/*  Author: Chase Richards
    Project: Homework 4 CS4760
    Date March 18, 2020
    Filename: oss.h  */

#ifndef OSS_H
#define OSS_H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <sys/msg.h>
#include <sys/wait.h>
#include <time.h>
#include <stdbool.h>
#include <signal.h>
#include <math.h>

void displayHelpMessage(); //-h getopt option
void sigHandler(int sig); //Signal Handle(ctrl c and timeout)
void removeAllMem(); //Removes all sharedmemory
FILE* openLogFile(char *file); //Opens the output log file
FILE* filePtr;

void scheduler(int); //Operating System simulator
int genProcPid(int *pidArr, int totalPids); //Generates the pid (1,2,3,4,...) 

//Shared memory keys and shared memory segment ids
const key_t pcbtKey = 122032;
const key_t clockKey = 202123;
const key_t messageKey = 493343;
int pcbtSegment, clockSegment, msgqSegment;

typedef struct
{
    long typeofMsg;
    int valueofMsg;
} msg;

//Shared memory clock
typedef struct
{
    unsigned int sec;
    unsigned int nanosec;
} clksim;

//Process control block table
typedef struct
{
    int fakePid; //0-18 pid
    int readyToGo; //user 1 or realtime 0
    int priority; //the processes priority
    clksim arrivalTime;
    clksim cpuTime; //CPU time used
    clksim blkedTime; //Time blocked
    clksim burstTime; //Time in last burst
    clksim waitingTime; //Time waiting
    clksim ageTime; //Aging alg   
} pcbt;

pcbt pcbCreation(int priority, int fakePid, clksim curTime)
{
    pcbt pcb = { .fakePid = fakePid, 
                 .priority = priority, 
                 .readyToGo = 1, 
                 .arrivalTime = {.sec = curTime.sec, .nanosec = curTime.nanosec},
                 .cpuTime = {.sec = 0, .nanosec = 0},
                 .blkedTime = {.sec = 0, .nanosec = 0},
                 .burstTime = {.sec = 0, .nanosec = 0},
                 .waitingTime = {.sec = 0, .nanosec = 0}, 
                 .ageTime = {.sec = 0, .nanosec = 0}};
    return pcb;
}

//Increment the clock so if it reaches a second in nanoseconds it changes accordingly
void clockIncrementor(clksim *simTime, int incrementor)
{
    simTime-> nanosec += incrementor;
    if(simTime-> nanosec >= 1000000000)
    {
        simTime-> sec += 1;
        simTime-> nanosec -= 1000000000;
    }
}

//Queue struct and prototypes
typedef struct
{
    unsigned int capacity;
    unsigned int items;
    unsigned int front;
    unsigned int back;
    int *arr;
} questrt;

questrt *queueCreation(int capacity);
void enqueue(questrt *queuePtr, int pid);
int dequeue(questrt *queuePtr);

//For adding and subtracting the virtual clock times
clksim addTime(clksim a, clksim b)
{
    clksim sum = {.sec = a.sec + b.sec, .nanosec = a.nanosec + b.nanosec};
    if(sum.nanosec >= 1000000000)
    {
        sum.nanosec -= 1000000000;
        sum.sec += 1;
    }
    return sum;
}

clksim subTime(clksim a, clksim b)
{
    clksim sub = {.sec = a.sec - b.sec, .nanosec = a.nanosec - b.nanosec};
    if(sub.nanosec < 0)
    {
        sub.nanosec += 1000000000;
        sub.sec -= 1;
    }
    return sub;
}

int blockedQueue(int *isBlocked, pcbt *pcbtPtr, clksim *clockPtr, clksim blkedTotal, int counter);
clksim nextProcessStartTime(clksim maxTime, clksim curTime);
int dispatcher(int fakePid, int priority, int msgId, clksim curTime, int quantum, int *outputLines);
int shouldCreateNewProc(int, int, clksim, clksim, int);
int agingCheck(int maxProcsInSys, pcbt *pcbtPtr);
//void clockIncrementor(clksim *simTime, int incrementor); 

#endif
