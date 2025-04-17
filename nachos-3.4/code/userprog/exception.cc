// exception.cc
//	Entry point into the Nachos kernel from user programs.
//	There are two kinds of things that can cause control to
//	transfer back to here from user code:
//
//	syscall -- The user code explicitly requests to call a procedure
//	in the Nachos kernel.  Right now, the only function we support is
//	"Halt".
//
//	exceptions -- The user code does something that the CPU can't handle.
//	For instance, accessing memory that doesn't exist, arithmetic errors,
//	etc.
//
//	Interrupts (which can also cause control to transfer from user
//	code into the Nachos kernel) are handled elsewhere.
//
// For now, this only handles the Halt() system call.
// Everything else core dumps.
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "system.h"
#include "syscall.h"
#include "system.h"
#include "addrspace.h"
#include "thread.h"
#define FileNameMaxLen 128
static OpenFile* openFileTable[20] = {NULL};
static bool openFileUsed[20] = {false};
// #include "synchconsole.h"
// SynchConsole *synchConsole;
//----------------------------------------------------------------------
// ExceptionHandler
// 	Entry point into the Nachos kernel.  Called when a user program
//	is executing, and either does a syscall, or generates an addressing
//	or arithmetic exception.
//
// 	For system calls, the following is the calling convention:
//
// 	system call code -- r2
//		arg1 -- r4
//		arg2 -- r5
//		arg3 -- r6
//		arg4 -- r7
//
//	The result of the system call, if any, must be put back into r2.
//
// And don't forget to increment the pc before returning. (Or else you'll
// loop making the same system call forever!
//
//	"which" is the kind of exception.  The list of possible exceptions
//	are in machine.h.
//----------------------------------------------------------------------

void doExit(int status)
{
    PCB *pcb = currentThread->space->pcb;
    int pid = pcb->pid;

    printf("System Call: [%d] invoked [Exit]\n", pid);
    printf("Process [%d] exits with [%d]\n", pid, status);

    // Mark process as exited
    pcb->exitStatus = status;

    // Remove from parent-child relationships
    // if (pcb->parent != NULL)
    // {
    //     pcb->parent->RemoveChild(pcb);
    // }

    // Clean up children: delete exited children, nullify parent in others
    pcb->DeleteExitedChildrenSetParentNull();

    // If it has no parent, free the PCB now
    // if (pcb->parent == NULL)
    // {
    //     pcbManager->DeallocatePCB(pcb);
    // }

    pcbManager->DeallocatePCB(pcb);

    // Clean up address space (which includes PCB pointer)
    delete currentThread->space;

    // Finish thread
    currentThread->Finish();
}


void incrementPC()
{
    int oldPCReg = machine->ReadRegister(PCReg);

    machine->WriteRegister(PrevPCReg, oldPCReg);
    machine->WriteRegister(PCReg, oldPCReg + 4);
    machine->WriteRegister(NextPCReg, oldPCReg + 8);
}

void childFunction(int pid)
{
    currentThread->RestoreUserState();
    currentThread->space->RestoreState();
    machine->Run(); // Never returns
    ASSERT(FALSE);
}

int doFork(int functionAddr)
{
    static int forkAttemptCount = 0;
    forkAttemptCount++;

    // First print that Fork was invoked
    printf("System Call: [%d] invoked Fork.\n", currentThread->space->pcb->pid);

    // Step 1: Check if enough memory
    if (currentThread->space->GetNumPages() > mm->GetFreePageCount()) {
        printf("Not Enough Memory for Child Process %d\n", forkAttemptCount);
        return -1;
    }

    // Step 2: Save parent's user register state
    currentThread->SaveUserState();

    // Step 3: Deep copy of the parent address space
    AddrSpace* childAddrSpace = new AddrSpace(currentThread->space);
    if (!childAddrSpace->valid) {
        delete childAddrSpace;
        return -1;
    }

    // Step 4: Create child thread
    Thread* childThread = new Thread("childThread");
    childThread->space = childAddrSpace;

    // Step 5: Allocate PCB and link to parent
    PCB* childPCB = pcbManager->AllocatePCB();
    if (childPCB == nullptr) {
        delete childThread;
        delete childAddrSpace;
        return -1;
    }

    childPCB->thread = childThread;
    childPCB->parent = currentThread->space->pcb;
    currentThread->space->pcb->AddChild(childPCB);
    childAddrSpace->pcb = childPCB;

    // Step 6: Copy parent's registers into child
    childThread->CopyUserRegistersFrom(currentThread);
    childThread->SetUserRegister(2, 0); // r2 = 0 in child
    childThread->SetUserRegister(PCReg, functionAddr);
    childThread->SetUserRegister(NextPCReg, functionAddr + 4);
    childThread->SetUserRegister(PrevPCReg, functionAddr - 4);

    // Step 7: Print info about the fork
    // Note: Removed the duplicate Fork invocation message that was here
    printf("Process [%d] Fork: start at address [0x%x] with [%d] pages memory\n",
        currentThread->space->pcb->pid, functionAddr, childAddrSpace->GetNumPages());

    // Step 8: Fork the child thread to jump into user mode
    childThread->Fork([](int) {
        currentThread->space->RestoreState();
        currentThread->RestoreUserState();
        machine->Run(); // never returns
        ASSERT(FALSE);
    }, 0);

    // Step 9: Restore parent's state and return child PID
    currentThread->space->RestoreState();
    currentThread->RestoreUserState();
    return childPCB->pid; // r2 = PID in parent
}


int doExec(char *filename)
{
    printf("System Call: [%d] invoked Exec\n", currentThread->space->pcb->pid);

    // 1. Open the executable file
    OpenFile *executable = fileSystem->Open(filename);
    AddrSpace *space;

    if (executable == NULL)
    {
        printf("Exec Error: Unable to open file %s\n", filename);
        return -1;
    }

    // 2. Save the existing PCB before replacing the address space
    PCB *pcb = currentThread->space->pcb;
    delete currentThread->space;

    // 3. Create a new address space
    space = new AddrSpace(executable);

    printf("Exec Program: [%d] loading [%s]\n",pcb->pid, filename);

    // 4. Close the executable file
    delete executable;

    // 5. Check if address space creation succeeded
    if (!space->valid)
    {
        printf("Exec Error: Could not create address space for file %s\n", filename);
        return -1;
    }

    // 6. Reuse the existing PCB
    space->pcb = pcb;

    // 7. Set the new address space for the current thread
    currentThread->space = space;

    // 8. Initialize registers for the new program
    space->InitRegisters();

    // 9. Load the page table into the MMU
    space->RestoreState();

    // 10. Begin execution of the new program
    machine->Run();

    // This line should never be reached unless something fails inside Run()
    ASSERT(FALSE);
    return 0;
}

OpenFileId doOpen(char *fileName)
{
    printf("System Call: [%d] invoked Open\n", currentThread->space->pcb->pid);
    
    // Open the file
    OpenFile *file = fileSystem->Open(fileName);
    if (file == NULL) {
        return -1; // Could not open the file
    }
    
    // For this simple implementation, we'll just use a static array to store files
    // This is not ideal for a real OS but works for testing purposes
    // static OpenFile* openFileTable[20] = {NULL};
    // static bool openFileUsed[20] = {false};
    
    // Find an available slot
    for (int i = 0; i < 20; i++) {
        if (!openFileUsed[i]) {
            openFileTable[i] = file;
            openFileUsed[i] = true;
            return i; // Return file descriptor
        }
    }
    
    // If we get here, no slots are available
    delete file; // Clean up
    return -1;
}

bool doClose(int fileId)
{
    printf("System Call: [%d] invoked Close.\n", currentThread->space->pcb->pid);
    
    // Check if fileId is valid
    if (fileId < 0 || fileId >= 20) {
        return false;
    }
    
    // Check if the file is actually open
    if (openFileTable[fileId] == NULL || !openFileUsed[fileId]) {
        return false;
    }
    
    // Close the file and mark the slot as available
    delete openFileTable[fileId];
    openFileTable[fileId] = NULL;
    openFileUsed[fileId] = false;
    
    return true;
}

void doCreate(char *fileName)
{
    int pid = currentThread->space->pcb->pid;
    printf("Syscall Call: [%d] invoked Create.\n", pid);
    
    // No need to read the string - it's already been read
    bool success = fileSystem->Create(fileName, 0);
    
    if (success) {
        printf("File Creation Successful: File [%s] created by PID [%d]\n", fileName, pid);
        // Remove List() call if it doesn't exist in your FileSystem class
    } else {
        printf("File Creation Failure: File [%s] not created\n", fileName);
    }
    
    machine->WriteRegister(2, success ? 1 : -1);
    
    // Clean up memory allocated by readString()
    delete[] fileName;
}

int doJoin(int pid) {
    printf("System Call: [%d] invoked Join.\n", currentThread->space->pcb->pid);

    PCB* joinPCB = pcbManager->GetPCB(pid);
    if (joinPCB == NULL) {
        return -1;
    }

    PCB* pcb = currentThread->space->pcb;
    if (pcb != joinPCB->parent) {
        return -1;
    }

    // Wait until the child has exited.
    while(!joinPCB->HasExited()) {
        currentThread->Yield();
    }

    // Retrieve child's exit status.
    int status = joinPCB->exitStatus;

    // Now, deallocate the child's PCB.
    pcbManager->DeallocatePCB(joinPCB);

    return status;
}

int doRead(int fileId, char *buffer, int size)
{
    printf("System Call: [%d] invoked Read.\n", currentThread->space->pcb->pid);
    
    // Check if fileId is valid (between 0 and 19)
    if (fileId < 0 || fileId >= 20) {
        return -1;
    }
    
    // Get the file from the open file table
    // You'll need to define the openFileTable in your class or as a global
    OpenFile* file = openFileTable[fileId];
    if (file == NULL) {
        return -1; // File not open
    }
    
    // Read from the file
    int bytesRead = file->Read(buffer, size);
    
    // Return the number of bytes read
    return bytesRead;
}

int doWrite(int fileId, char *buffer, int size)
{
    printf("System Call: [%d] invoked Write.\n", currentThread->space->pcb->pid);
    
    // Check if fileId is valid
    if (fileId < 0 || fileId >= 20) {
        return -1;
    }
    
    // Get the file from the open file table
    OpenFile* file = openFileTable[fileId];
    if (file == NULL || !openFileUsed[fileId]) {
        return -1; // File not open
    }
    
    // Write to the file
    int bytesWritten = file->Write(buffer, size);
    
    return bytesWritten;
}


int doKill(int pid)
{
    PCB* victimPCB = pcbManager->GetPCB(pid);

    // Step 1: Validate PID
    if (victimPCB == NULL) {
        printf("Kill Error: Invalid PID [%d]\n", pid);
        return -1;
    }

    // Step 2: If the current thread is being killed, just call doExit
    if (victimPCB == currentThread->space->pcb) {
        printf("Kill Info: Process [%d] is self; calling doExit(0)\n", pid);
        doExit(0);
        return 0;
    }

    // FIXED FORMATTING TO MATCH EXPECTED OUTPUT
    printf("System Call: [%d] invoked Kill.\n", currentThread->space->pcb->pid);
    printf("Process [%d] killed process [%d]\n",
           currentThread->space->pcb->pid, pid);

    // Step 3: Remove from parent's children list if parent exists
    if (victimPCB->parent != NULL) {
        victimPCB->parent->RemoveChild(victimPCB);
    }

    // Step 4: Set children's parent to null
    victimPCB->DeleteExitedChildrenSetParentNull();

    // Step 5: Remove the address space and memory
    delete victimPCB->thread->space;

    // Step 6: Remove thread from ready list or mark to be destroyed
    if (victimPCB->thread == currentThread) {
        threadToBeDestroyed = currentThread;
    } else {
        scheduler->RemoveThread(victimPCB->thread);
        delete victimPCB->thread;
    }

    // Step 7: Deallocate PCB
    pcbManager->DeallocatePCB(victimPCB);

    // Step 8: Return success
    return 0;
}

void doYield()
{
    DEBUG('t', "System Call: [%d] invoked Yield.\n", currentThread->space->pcb->pid);
    printf("System Call: [%d] invoked Yield.\n", currentThread->space->pcb->pid);
    currentThread->Yield();
}

char *readString(int virtualAddr)
{
    int i = 0;
    char *str = new char[256];
    unsigned int physicalAddr = currentThread->space->Translate(virtualAddr);

    // Need to get one byte at a time since the string may straddle multiple pages that are not guaranteed to be contiguous in the physicalAddr space
    bcopy(&(machine->mainMemory[physicalAddr]), &str[i], 1);
    while (str[i] != '\0' && i != 256 - 1)
    {
        virtualAddr++;
        i++;
        physicalAddr = currentThread->space->Translate(virtualAddr);
        bcopy(&(machine->mainMemory[physicalAddr]), &str[i], 1);
    }
    if (i == 256 - 1 && str[i] != '\0')
    {
        str[i] = '\0';
    }

    return str;
}


void ExceptionHandler(ExceptionType which)
{
    int type = machine->ReadRegister(2);

    if ((which == SyscallException) && (type == SC_Halt))
    {
        DEBUG('a', "Shutdown, initiated by user program.\n");
        interrupt->Halt();
    }
    else if ((which == SyscallException) && (type == SC_Exit))
    {
        // Implement Exit system call
        doExit(machine->ReadRegister(4));
    }
    else if ((which == SyscallException) && (type == SC_Fork))
    {
        int ret = doFork(machine->ReadRegister(4));
        machine->WriteRegister(2, ret);
        incrementPC();
    }
    else if ((which == SyscallException) && (type == SC_Exec))
    {
        int virtAddr = machine->ReadRegister(4);
        char *fileName = readString(virtAddr);
        int ret = doExec(fileName);
        machine->WriteRegister(2, ret);
        incrementPC();
    }
    else if ((which == SyscallException) && (type == SC_Join))
    {
        int ret = doJoin(machine->ReadRegister(4));
        machine->WriteRegister(2, ret);
        incrementPC();
    }
    else if ((which == SyscallException) && (type == SC_Kill))
    {
        int ret = doKill(machine->ReadRegister(4));
        machine->WriteRegister(2, ret);
        incrementPC();
    }
    else if ((which == SyscallException) && (type == SC_Yield))
    {
        doYield();
        incrementPC();
    }
    else if ((which == SyscallException) && (type == SC_Create))
    {
        int virtAddr = machine->ReadRegister(4);
        char *fileName = readString(virtAddr);
        doCreate(fileName);
        incrementPC();
    }
        else if ((which == SyscallException) && (type == SC_Open))
    {
        int virtAddr = machine->ReadRegister(4);
        char *fileName = readString(virtAddr);
        OpenFileId ret = doOpen(fileName);
        machine->WriteRegister(2, ret);
        delete[] fileName;
        incrementPC();
    }
    else if ((which == SyscallException) && (type == SC_Read))
    {
    int fileId = machine->ReadRegister(4);    // File descriptor
    int virtAddr = machine->ReadRegister(5);  // Buffer address
    int size = machine->ReadRegister(6);      // Size to read
    
    // Allocate buffer for data
    char *buffer = new char[size];
    
    // Call the doRead function
    int bytesRead = doRead(fileId, buffer, size);
    
    // Copy data from kernel space to user space
    if (bytesRead > 0) {
        // Copy each byte to user memory
        for (int i = 0; i < bytesRead; i++) {
            machine->WriteMem(virtAddr + i, 1, buffer[i]);
        }
    }
    
    // Clean up
    delete[] buffer;
    
    // Return bytes read
    machine->WriteRegister(2, bytesRead);
    incrementPC();
    }
    else if ((which == SyscallException) && (type == SC_Close))
    {
        int fileId = machine->ReadRegister(4);
        bool success = doClose(fileId);
        machine->WriteRegister(2, success ? 0 : -1);
        incrementPC();
    }
    else if ((which == SyscallException) && (type == SC_Write))
    {
        int fileId = machine->ReadRegister(4);    // File descriptor
        int virtAddr = machine->ReadRegister(5);  // Buffer address
        int size = machine->ReadRegister(6);      // Size to write
        
        // Allocate buffer for data
        char *buffer = new char[size];
        
        // Copy data from user space to kernel space
        for (int i = 0; i < size; i++) {
            int value;
            if (!machine->ReadMem(virtAddr + i, 1, &value)) {
                printf("Error reading from user memory\n");
                size = i;  // Only write what we've successfully read
                break;
            }
            buffer[i] = (char)value;
        }
        
        // Call the doWrite function
        int bytesWritten = doWrite(fileId, buffer, size);
        
        // Clean up
        delete[] buffer;
        
        // Return bytes written
        machine->WriteRegister(2, bytesWritten);
        incrementPC();
    }
    else
    {
        printf("Unexpected user mode exception %d %d\n", which, type);
        ASSERT(FALSE);
    }
}