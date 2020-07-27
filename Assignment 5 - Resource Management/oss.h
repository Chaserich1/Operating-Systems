/*  Author: Chase Richards
    Project: Homework 5 CS4760
    Date April 4, 2020
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

void displayHelpMessage(); //-h getopt option
void sigHandler(int sig); //Signal Handle(ctrl c and timeout)
void removeAllMem(); //Removes all sharedmemory
FILE* openLogFile(char *file); //Opens the output log file
FILE* filePtr; //output file
void manager(int, int); //resource manager
int generateProcPid(int *pidArr, int totalPids); //Generates the pid (0,1,2,3,4,..17) 
void printStats(); //printing the final statistics
int divideNums(int, int); //dividing totalterminations by detected deadlocks

//Shared memory keys and shared memory segment ids
const key_t resDescKey = 122032;
const key_t clockKey = 202123;
const key_t messageKey = 493343;
int resDescSegment, clockSegment, msgqSegment;

/* ---------------------------------Messaging Setup-------------------------------------- */

/* Message structure that includes the type of message (1 for oss), the real and generated pids,
   the specific resource for release or request, and message details which says whether the process
   is requesting, releasing, or terminating. For oss it says whether it is granting or denying */
typedef struct
{
    long typeofMsg;
    int process;
    int processesPid;
    int resource;
    int msgDetails;
} msg;

//Prototypes for different messages to and from oss and user
void messageToProcess(int receiver, int response);
int requestToOss(int process, int procPid, int resource);
int releaseToOss(int process, int procPid, int resource);
void terminateToOss(int process, int procPid);

/* ------------------------------Simulated Clock Setup----------------------------------- */

//Shared memory clock
typedef struct
{
    unsigned int sec;
    unsigned int nanosec;
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


//Start time for the next process (between 1 and 500 milliseconds)
clksim nextProcessStartTime(clksim maxTime, clksim curTime);

/* ------------------------------Resource Descriptor Setup----------------------------------- */

/* Resource Decriptor is a fixed size struct containing info on managing the resources in oss */
typedef struct
{
    //Resources shared
    int sharedResources[20];
    //Available resources
    int availableResources[20];
    //Shared or not vector
    int resSharedOrNot[20];
    //Request matrix of resources and the processes doing the requesting
    int requesting2D[18][20];
    //Allocated matrix of resources and the processes who have the resources allocated to them
    int allocated2D[18][20];
} resDesc;

/* Constructor for the resource descriptor: initial instances of each resource class are a random num
   between 1 and 10 (inclusively). Create 20 resource descriptors out of which 20 +- 5% are shared */
void resDescConstruct(resDesc *descPtr)
{
    int i, j; //loops
    srand(time(NULL));
    //Initialize the spots in the tables to all zeros
    for(i = 0; i < 18; i++)
    {
        for(j = 0; j < 20; j++)
        {
            descPtr-> requesting2D[i][j] = 0;
            descPtr-> allocated2D[i][j] = 0;
        }
    }
    for(i = 0; i < 20; i++)
    {
        //20 +- 5% chance of it being a shared resource
        int randNum = rand() % ((6 + 1) - 4) + 4;
        int randNum1 = rand() % (randNum) + 1;
        if(randNum1 != 1)
        {
            //Random Number between 1-10 inclusive
            int randNum2 = rand() % 10 + 1;
            descPtr-> sharedResources[i] = randNum2;
            descPtr-> availableResources[i] = descPtr-> sharedResources[i];
            descPtr-> resSharedOrNot[i] = 0;
        }
        else
        {
            descPtr-> sharedResources[i] = 7;
            descPtr-> availableResources[i] = descPtr-> sharedResources[i];
            descPtr-> resSharedOrNot[i] = 1;
        }       
    }
    return;
}

//Prototypes for deadlock checking from notes and deadlock resolution and for printing allocated table
int deadlock(resDesc *resDescPtr, int nProcs, clksim *clockPtr, int *pidArr, int *procCounter, int *outputLines);
int req_lt_avail(int req[], int avail[], int shared[], int held[]);
void printAllocatedTable(int allocated2D[18][20], int processes, int resources);
void printTable(resDesc resDescPtr, int processes, int resources); 

#endif
