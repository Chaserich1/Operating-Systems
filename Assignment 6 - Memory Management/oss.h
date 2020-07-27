/*  Author: Chase Richards
    Project: Homework 6 CS4760
    Date April 23, 2020
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
#include <semaphore.h>
#include <fcntl.h>

#define MAXPROCESSES 18

void displayHelpMessage(); //-h getopt option
void sigHandler(int sig); //Signal Handle(ctrl c and timeout)
void removeAllMem(); //Removes all sharedmemory
FILE* openLogFile(char *file); //Opens the output log file
FILE* filePtr; //output file
void manager(int, int); //resource manager
int generateProcPid(int *pidArr, int totalPids); //Generates the pid (0,1,2,3,4,..17) 
void printStats(); //printing the final statistics
int firstScheme(); //Random value between 0 and 32k

//Shared memory keys and shared memory segment ids
const key_t clockKey = 202123;
const key_t messageKey = 493343;
int clockSegment, msgqSegment;

/* ---------------------------------Messaging Setup-------------------------------------- */

/* Message structure that includes the type of message, the pid, the address, and message details 
   which says whether it is read, write or terminating */
typedef struct
{
    long typeofMsg;
    int process;
    int address;
    int msgDetails;
} msg;

//Prototypes for different messages to and from oss and user
void messageToProcess(int, int);
void messageToOss(int, int, int);

/* ------------------------------Simulated Clock Setup----------------------------------- */

//Shared memory clock
typedef struct
{
    unsigned int sec;
    unsigned int nanosec;
    float arrOfWeights[32];
} clksim;

//Increment the clock so if it reaches a second in nanoseconds it changes accordingly and increment time
void clockIncrementor(clksim *simTime, int incrementor)
{
    simTime-> nanosec += incrementor;
    if(simTime-> nanosec >= 1000000000)
    {
        simTime-> nanosec -= 1000000000;
        simTime-> sec += 1;
    }
}

//For subtracting the virtual clock times in user to determine if a process has run for at least 1 second
clksim subTime(clksim time1, clksim time2)
{
    clksim sub = {.sec = time1.sec - time2.sec, .nanosec = time1.nanosec - time2.nanosec};
    if(sub.nanosec < 0)
    {
        sub.nanosec += 1000000000;
        sub.sec -= 1;
    }
    return sub;
}

/* ------------------------------------Queue Setup----------------------------------------- */

/* Queue struct and prototypes */
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

//Start time for the next process (between 1 and 500 milliseconds)
clksim nextProcessStartTime(clksim maxTime, clksim curTime);

/* ------------------------------------Paging Setup----------------------------------------- */

//Page struct
typedef struct 
{
    int locationOfFrame;
} page;

//Frame Struct
typedef struct
{
    int process;
    unsigned referenceBit: 1;
    unsigned dirtyBit: 1;
    clksim arrivalTime;
} frameTable;

//Page table struct
typedef struct
{
    page pageArr[32]; //32 pages in the page array
} pageTable;

//Prototypes for logging frame allocation and paging functions
int findAvailFrame(frameTable *frameT);
void logFrameAllocation(frameTable *frameT, clksim curTime); 
int clockReplacementPolicy(frameTable *frameT, clksim curTime);

#endif
