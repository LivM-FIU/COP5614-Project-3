#include "syscall.h"

#define BUFFER_SIZE 256

const char writeBuffer[BUFFER_SIZE] = "This is a test of the Nachos file system. If you can read this message, write() is working!";
char readBuffer[BUFFER_SIZE];

int main()
{
    int fileId;
    
    // Create a test file
    Create("test_write.txt");
    
    // Open the file
    fileId = Open("test_write.txt");
    if (fileId < 0) {
        Halt();
    }
    
    // Write to the file
    Write(writeBuffer, 80, fileId);
    
    // Close the file
    Close(fileId);
    
    // Re-open the file for reading
    fileId = Open("test_write.txt");
    if (fileId < 0) {
        Halt();
    }
    
    // Read from the file
    Read(readBuffer, BUFFER_SIZE, fileId);
    
    // Close the file
    Close(fileId);
    
    Halt();
    return 0;
}
