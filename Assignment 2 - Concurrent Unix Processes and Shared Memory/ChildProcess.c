/* Author: Chase Richards
   Project: Homework 2 CS4760
   Date: February 16, 2020  
   Filename: ChildProcess.c  */

#include "ChildProcess.h"

int main(int argc, char* argv[]) 
{

    int sharedMemSegment, sharedMemDetach;
    char *sharedMemAttach; 
    //ftok gives us the key based on curent path and 'm' id
    key_t key = ftok(".",'m');
    
    //Error Getting key
    if(key == -1)
    {
        perror("prime: Error: Getting struct key");
        exit(EXIT_FAILURE);
    }

    //Allocate the shared memory with shmget
    sharedMemSegment = shmget(key, sizeof(struct sharedMemory), IPC_CREAT | 0644);

    //If shmget returns -1 then it was unsuccessful
    if(sharedMemSegment == -1)
    {
        perror("prime: Error: shmget failed to allocate shared memory\n");
        exit(EXIT_FAILURE);
    }
    
    //Attach the memory to our space
    smPtr = (struct sharedMemory *)shmat(sharedMemSegment, NULL, 0);

    //If shmat returns -1 then it was unsuccessful
    if(smPtr == (void *)-1)
    {
        perror("prime: Error: shmat failed to attach shared memory");
        exit(EXIT_FAILURE);
    }
    
    /*Used for testing signals 
    for(;;)
        ;
    */

    //Save the starting time to a variable
    int childStartTime = smPtr-> nanoSeconds;
   
    int i; 
    int childID = atoi(argv[2]);
    int numToCheck = atoi(argv[1]);
    int primeFlg = 1;
    
    //Check from 2 to the number we are checking divided by 2 
    for(i = 2; i <= sqrt(numToCheck); i++)
    {
        //Check if this child has passed the 1 millisecond time limit
        if(smPtr-> nanoSeconds >= (childStartTime + 1000000))
        {
            smPtr-> childProcArr[childID] = -1;
            smPtr-> tooMuchTime[childID] = numToCheck;
            sharedMemDetach = deallocateMem(sharedMemSegment, smPtr);
            //If shmdt returns -1 then it was unsuccessful
            if(sharedMemDetach == -1)
            {
                perror("prime: Error: shmdt failed to detach shared memory\n");
                exit(EXIT_FAILURE);
            }       
            return (-1);          
        }

        //If n is divided by any to the number from 2 to numToCheck/2 it isn't prime
        if(numToCheck % i == 0)
            primeFlg = 0;
    }

    /* If the number is not prime add it to the shared mem array 
       as a negative if it is prime then add it to the array as is */
    if(primeFlg == 0)
        smPtr-> childProcArr[childID] = (numToCheck * -1);
    else if(primeFlg == 1)
        smPtr-> childProcArr[childID] = numToCheck;     
    
    //Detach from shared memory and check for -1 failure to detach  
    sharedMemDetach = deallocateMem(sharedMemSegment, smPtr);
    if(sharedMemDetach == -1)
    {
        perror("prime: Error: failed to detach shared memory");
        exit(EXIT_FAILURE);
    }
 
    return 0;
}

int deallocateMem(int shmid, void *shmaddr) 
{
    //If detaching fails it will return -1 so return -1 for deallocation call
    if(shmdt(shmaddr) == -1)
        return -1;
    //shmctl(shmid, IPC_RMID, NULL); Don't need to remove the shared memory
    return 0;
}
