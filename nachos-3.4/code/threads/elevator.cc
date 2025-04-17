#include "copyright.h"
#include "system.h"
#include "synch.h"
#include "elevator.h"

int nextPersonID = 1;
Lock *personIDLock = new Lock("PersonIDLock");

ELEVATOR *e;
Semaphore *elevatorReady = new Semaphore("ElevatorReady", 0);

void ELEVATOR::start() {
    while (1) {
        // A. Wait until hailed
        elevatorLock->Acquire();
        bool hasWaiting = false;
        for (int i = 0; i < numFloors; i++) {
            if (personsWaiting[i] > 0) {
                hasWaiting = true;
                break;
            }
        }
        if (!hasWaiting && occupancy == 0) {
            elevatorCondition->Wait(elevatorLock); // Lock is held here
        }

        // B. While there are active persons, loop doing the following
        // 0. Acquire elevatorLock (Already acquired above)
        // 1. Signal persons inside elevator to get off (leaving->broadcast(elevatorLock))
        leaving[currentFloor - 1]->Broadcast(elevatorLock); // Lock is held here

        // 2. Signal persons atFloor to get in, one at a time, checking occupancyLimit each time
        while (personsWaiting[currentFloor - 1] > 0 && occupancy < maxOccupancy) {
            entering[currentFloor - 1]->Signal(elevatorLock); // Lock is held here
            personsWaiting[currentFloor - 1]--;
        }

        // 2.5 Release elevatorLock
        elevatorLock->Release();

        // 3. Spin for some time
        for (int j = 0; j < 1000000; j++) {
            currentThread->Yield();
        }

        // 4. Go to next floor
        elevatorLock->Acquire();
        if (direction == 1 && currentFloor < numFloors) {
            currentFloor++;
        } else if (direction == -1 && currentFloor > 1) {
            currentFloor--;
        } else if (direction == 1 && currentFloor == numFloors) {
            direction = -1;
            currentFloor--;
        } else if (direction == -1 && currentFloor == 1) {
            direction = 1;
            currentFloor++;
        }
        if(occupancy != 0 ){
        printf("Elevator arrives on floor %d\n", currentFloor);
        elevatorLock->Release();
        }
        elevatorLock->Release();
    }
}

void ElevatorThread(int numFloors) {
    printf("Elevator with %d floors was created!\n", numFloors);
    e = new ELEVATOR(numFloors);
    elevatorReady->V();
    e->start();
}

ELEVATOR::ELEVATOR(int floors) {
    this->numFloors = floors;
    currentFloor = 1;
    direction = 1;
    occupancy = 0;
    maxOccupancy = 5;
    activePassengers = 0; // Initialize active passengers

    entering = new Condition *[numFloors];
    leaving = new Condition *[numFloors];
    personsWaiting = new int[numFloors];
    elevatorLock = new Lock("ElevatorLock");
    elevatorCondition = new Condition("ElevatorCondition");

    for (int i = 0; i < numFloors; i++) {
        char enteringName[50];
        char leavingName[50];
        sprintf(enteringName, "Entering %d", i + 1);
        sprintf(leavingName, "Leaving %d", i + 1);
        entering[i] = new Condition(enteringName);
        leaving[i] = new Condition(leavingName);
        personsWaiting[i] = 0;
    }
}

ELEVATOR::~ELEVATOR() {
    delete elevatorLock;
    delete elevatorCondition;
    for (int i = 0; i < numFloors; i++) {
        delete entering[i];
        delete leaving[i];
    }
    delete[] entering;
    delete[] leaving;
    delete[] personsWaiting;
}

void Elevator(int numFloors) {
    Thread *t = new Thread("Elevator");
    t->Fork(ElevatorThread, numFloors);
    elevatorReady->P();
}

void ELEVATOR::hailElevator(Person *p) {
    elevatorLock->Acquire();
    personsWaiting[p->atFloor - 1]++;

    if ((occupancy == 0 && personsWaiting[p->atFloor - 1] == 1) || occupancy > 0) {
        elevatorCondition->Signal(elevatorLock);
    }

    entering[p->atFloor - 1]->Wait(elevatorLock); // Lock is held here

    printf("Person %d got into the elevator.\n", p->id);
    occupancy++;

    leaving[p->toFloor - 1]->Wait(elevatorLock); // Lock is held here

    printf("Person %d got out of the elevator.\n", p->id);
    occupancy--;
    if(occupancy == 0){
        printf("Elevator Stops.\n");
        elevatorCondition->Wait(elevatorLock);
    }

    elevatorLock->Release();
}
void PersonThread(int person) {
    Person *p = (Person *)person;
    printf("Person %d wants to go from floor %d to %d\n", p->id, p->atFloor, p->toFloor);
    e->hailElevator(p);
}

int getNextPersonID() {
    personIDLock->Acquire();
    int personID = nextPersonID;
    nextPersonID++;
    personIDLock->Release();
    return personID;
}

void ArrivingGoingFromTo(int atFloor, int toFloor) {
    Person *p = new Person;
    p->id = getNextPersonID();
    p->atFloor = atFloor;
    p->toFloor = toFloor;
    Thread *t = new Thread("Person " + p->id);
    t->Fork(PersonThread, (int)p);
}