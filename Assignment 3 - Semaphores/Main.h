/* Author: Chase Richards
   Project: Homework 3 CS4760
   Date March 3, 2020
   Filename: Main.h  */

#ifndef MAIN_H
#define MAIN_H

#include "SharedMemStruct.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <string.h>
#include <semaphore.h>
#include <sys/ipc.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <time.h>
#include <math.h>

//Input file
FILE* INFILE;

//Contains Help Message
void displayHelpMessage();

//Detaching and removing shared memory
int deallocateMem(int shmid, void *shmaddr);

//Getting and Attaching to shared Memory
void sharedMemoryWork(int totalInts, int n, char *inFile);

// n/2 and nlog(n) functions
int calculationOne(int, int);
int calculationTwo(int, int);

//Signal Handling (ctrl c and timeout)
void sigHandler(int sig);

//Open the file and check for failure
FILE* openFile(char *filename, char *mode, int n);

//Reading from input file and putting in shared memory
int readFileOne(char *fileName, int sharedMemSegment, int n);
int readFileTwo(char *fileName, int sharedMemSegment, int n);

//Write PID Index Size
void writeLogHeaders(int);

#endif
