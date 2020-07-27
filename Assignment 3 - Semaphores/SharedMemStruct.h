/* Author: Chase Richards
   Project: Homework 3 CS4760
   Date March 11, 2020
   Filename: SharedMemStruct.h  */

//Struct for the shared memory arrays for both n/2 and n/logn
struct sharedMemory
{
    int integersOne[1024];
    int integersTwo[1024];
};

struct sharedMemory *smPtr;
