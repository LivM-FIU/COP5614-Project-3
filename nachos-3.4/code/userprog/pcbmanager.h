#ifndef PCBMANAGER_H
#define PCBMANAGER_H

#include "bitmap.h"
#include "pcb.h"
#include "synch.h"

class PCBManager{

    public:
        PCBManager(int maxProcesses);
        ~PCBManager();

        PCB* AllocatePCB();
        int DeallocatePCB(PCB* pcb);
        PCB* GetPCB(int pid);

    private:
        BitMap* bitmap;
        PCB** pcbs;
        Lock* pcbManagerLock;
        int maxProcesses; 
};

#endif