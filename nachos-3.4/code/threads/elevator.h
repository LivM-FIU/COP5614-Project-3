#ifndef ELEVATOR_H
#define ELEVATOR_H

#include "copyright.h"
#include "synch.h"

// Function Declarations
void Elevator(int numFloors);
void ArrivingGoingFromTo(int atFloor, int toFloor);


// Person Structure
struct Person {
    int id;
    int atFloor;
    int toFloor;
};

// Elevator Class
class ELEVATOR {
public:
    ELEVATOR(int numFloors);
    ~ELEVATOR();

    void hailElevator(Person *p);
    void start();
    bool anyPersonsWaiting();

private:
    int numFloors;
    int currentFloor;
    int direction;
    int occupancy;
    int maxOccupancy;
    int activePassengers; // Track active passengers
    Lock *elevatorLock;

    Condition *elevatorCondition;
    Condition **entering;
    Condition **leaving;
    int *personsWaiting;
};

// Ensure Global Elevator Object is Declared Correctly
extern ELEVATOR *e;

#endif // ELEVATOR_H