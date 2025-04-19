#include "copyright.h"
#include "system.h"
#include "synch.h"

Lock *Testl = new Lock("MyLock");
int Counter = 0;

void LockT(int number)
{
    int num, val;

    for (num = 0; num < 3; num++)
    {
        Testl->Acquire();

        val = Counter;
        printf("*** Thread %d sees counter value %d\n", number, val);
        Counter++;

        Testl->Release();

        currentThread->Yield(); // Allow other threads to execute
    }

    // Final counter check
    while (Counter<12)
    currentThread->Yield();

    val = Counter;

    printf("Thread %d sees final value %d\n", number, val);

    currentThread->Finish();
}

void LockTest()
{       printf(" Lock test with threads\n");

    // Fork other threads
    for (int i = 1; i <= 3; i++)
    {
        Thread *Testt = new Thread("forked thread");
        Testt->Fork(LockT, i);
    }
    LockT(0);
        // Yield so that Thread 0 gets CPU before others
        // currentThread->Yield();
}
