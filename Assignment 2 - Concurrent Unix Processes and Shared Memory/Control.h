/* Author: Chase Richards
   Project: Homework 2 CS4760
   Date: February 15, 2020
   Filename: Control.h  */

#ifndef CONTROL_H
#define CONTROL_H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <string.h>
#include <time.h>
#include <signal.h>

struct sharedMemory
{
    int nanoSeconds;
    int seconds;  
    int childProcArr[1024];
    int tooMuchTime[1024]; 
};

struct sharedMemory* smPtr;

FILE *OUTFILE;

//Prototype for deallocating the shared memory
int deallocateMem(int shmid, void *shmaddr);

//Prototype for working with the shared memory from master
void sharedMemoryWork();

//Prototype for forking, execing, and writing to the output file the numbers from the shared mem array
void launchChildren(int maxChildren, int childLimit, int startOfSeq, int incrementVal, char *outFile);

//Signal handler
void sigHandler(int sig);

//Each time it loops we add 10000 nanoSeconds and handles adding to seconds 
void timeIncrementation();

//-h option
void displayHelpMessage();

#endif
