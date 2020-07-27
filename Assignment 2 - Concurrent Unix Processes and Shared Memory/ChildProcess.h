/* Author: Chase Richards
   Project: Homework 2 CS4760
   Date: February 16, 2020  
   Filename: ChildProcess.h  */

#ifndef CHILDPROCESS_H
#define CHILDPROCESS_H

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/shm.h>
#include <sys/ipc.h>
#include <math.h>

int deallocateMem(int shmid, void *shmaddr);

struct sharedMemory
{
    int nanoSeconds;
    int seconds;
    int childProcArr[1024];
    int tooMuchTime[1024];
};

struct sharedMemory *smPtr;



#endif
