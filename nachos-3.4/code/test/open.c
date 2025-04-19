#include "syscall.h"

int main()
{
    SpaceId fileId;
    
    // Create a test file first (if needed)
    Create("testfile.txt");
    
    // Try to open the file
    fileId = Open("testfile.txt");
    
    // Simple test to see if it worked
    if (fileId == -1) {
        // File couldn't be opened
        Halt();
    } else {
        // File opened successfully - just exit with the file ID
        Exit(fileId);
    }
    
    return 0;
}