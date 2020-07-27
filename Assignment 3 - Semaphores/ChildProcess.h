/* Author: Chase Richards
   Project: Homework 3 CS4760
   Date March 3, 2020
   Filename: ChildProcess.h  */

#ifndef CHILDPROCESS_H
#define CHILDPROCESS_H

#include "SharedMemStruct.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/shm.h>
#include <string.h>
#include <unistd.h>
#include <semaphore.h>
#include <time.h>

//Deallocate shared memory
int deallocateMem(int shmid, void *shmaddr);

//For the current times
void timeSetter(char* curTime);

//n/2 critical and calc
void firstCriticalSection(int, int);
void secondCriticalSection(int, int);
#endif
