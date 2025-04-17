/* read.c */
#include "syscall.h"

#define BUFFER_SIZE 256

char buffer[BUFFER_SIZE];

int main()
{
    int fileId, bytesRead;
    
    // Create a test file
    Create("testfile.txt");
    
    // Open the file
    fileId = Open("testfile.txt");
    if (fileId < 0) {
        // Error handling without PrintString
        Halt();
    }
    
    // Read from the file
    bytesRead = Read(buffer, BUFFER_SIZE, fileId);
    
    // Close the file
    Close(fileId);
    
    return 0;
}