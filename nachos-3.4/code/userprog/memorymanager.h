#ifndef MEMORY_H
#define MEMORY_H

#include "bitmap.h"

class MemoryManager{

    public: 
        MemoryManager();
        ~MemoryManager();

        int AllocatePage();
        int DeAllocatePage(int which); // Page number
        unsigned int GetFreePageCount();

    private:
        BitMap *bitmap;
};

#endif // MEMORY_H
