/* Author: Chase Richards
   Project: Homework 3 CS4760
   Date March 3, 2020
   Filename: ChildProcess.c  */

#include "ChildProcess.h"

int main(int argc, char* argv[])
{
    key_t key;
    int sharedMemSegment, sharedMemDetach, sharedMemDetach1;
 
    //ftok gives me the key based path and 'm' id
    key = ftok(".",'a');

    //If key returns -1 then it failed so check
    if(key == -1)
    {
        perror("bin_adder: Error: Failed to get shared memory key stuct of ints");
        exit(EXIT_FAILURE);
    }
    
    //Allocate the shared memory using shmget
    sharedMemSegment = shmget(key, sizeof(struct sharedMemory), IPC_CREAT | 0666);
    
    //If shmget returns -1 then it failed
    if(sharedMemSegment == -1)
    {
        perror("bin_adder: Error: Failed to get shared memory segment struct of ints");
        exit(EXIT_FAILURE);
    }
    
    //Attach to the shared memory segment
    smPtr = shmat(sharedMemSegment, (void *)0, 0);

    //Check if it returns -1
    if(smPtr == (void *) -1)
    {
        perror("bin_adder: Error: Failed to attach to shared memory struct of ints");
        exit(EXIT_FAILURE);
    }   
    //ftok the key and check for failure
    key_t key1 = ftok(".", 'b');
    if(key1 == -1)
    {
        perror("bin_adder: Error: Failed to get shared memory key calc array");
        exit(EXIT_FAILURE);
    }
    
    //Allocate and get the shared memory segment for the array
    int seg = shmget(key1, sizeof(int), IPC_CREAT | 0777);
    if(seg == -1)
    {
        perror("bin_adder: Error: Failed to get shared memory segment calc array");
        exit(EXIT_FAILURE);
    }

    //Attach to shared mem seg and check for failure
    int *calculationFlg = (int*)shmat(seg, (void*)0, 0);
    if(calculationFlg == (void *) -1)
    {
        perror("bin_adder: Error: Failed to attach to shared memory calc array");
        exit(EXIT_FAILURE);
    } 

    //Assign index and numIntsToAdd to the exec arguments 
    int index = atoi(argv[1]);
    int numIntsToAdd = atoi(argv[2]);

    //printf("%d, %d, %d\n", index, numIntsToAdd, smPtr-> integers[index]);
    
    //If calculation flg is 0 do n/2 calculation and if it is 1 do n/logn calculation
    if(calculationFlg[0] == 0) 
        firstCriticalSection(index, numIntsToAdd);  
    else if(calculationFlg[0] == 1)
        secondCriticalSection(index, numIntsToAdd);

    //Detach from shared memory and check for it returning -1
    sharedMemDetach = deallocateMem(sharedMemSegment, (void*) smPtr);
    sharedMemDetach1 = deallocateMem(seg, (void*) calculationFlg);
    if(sharedMemDetach == -1 || sharedMemDetach1 == -1)
    {
        perror("bin_adder: Error: Failed to detach shared memory");
        exit(EXIT_FAILURE);
    }
    return 0;
}

// n/2 partition and critical section
void firstCriticalSection(int index, int numIntsToAdd)
{
    FILE* file;
    file = fopen("adder_log", "a");
    
    //Semaphore Declarations
    sem_t *sem;
    char *semaphoreName = "semChild";
    //Open semaphore and check for failure
    sem = sem_open(semaphoreName, 0);
    if(sem == SEM_FAILED)
    {
        perror("bin_adder: Error: Failed to open n/2 semaphore");
        exit(EXIT_FAILURE);
    }

    /* numIntsToAdd is 2 for this calculation, so add an index
       and the following index, store it in the first index, and
       zero out the second index */
    int i;
    for(i = 1; i < numIntsToAdd; i++)
    {    
        smPtr-> integersOne[index] += smPtr-> integersOne[index + i];
        smPtr-> integersOne[index + i] = 0;
    }
 
    //Critical Section One
    char time[10];
 
    sleep(rand() % 4); //sleep for somewhere between 0-3 seconds
    timeSetter(time);
    fprintf(stderr, "%d waiting to enter the critical section at %s\n", getpid(), time);
    sem_wait(sem); //waiting for its time to enter critcal section
    //Beginning Critical Section
    timeSetter(time);
    fprintf(stderr, "%d is inside the critical section at %s\n", getpid(), time);
    sleep(1);
    //Write to output log file
    fprintf(file, "\t%d\t\t%d\t\t%d\t\t%d\t\t%s\n", getpid(), index, numIntsToAdd, smPtr-> integersOne[index], time);       
    sleep(1); //Sleep another 1 second before leaving critical section
    timeSetter(time);
    fprintf(stderr, "%d is exiting the critical section at %s\n", getpid(), time);
    sem_post(sem); //Critical section finished, signal semaphore   
}

// n/logn calculation critical second
void secondCriticalSection(int index, int numIntsToAdd)
{
    FILE* file;
    file = fopen("adder_log", "a");
    //Semaphore declarations
    sem_t *sem1;
    char *semaphoreName1 = "semLogChild"; 
    //Open the smaphore and check for failure
    sem1 = sem_open(semaphoreName1, 0);
    if(sem1 == SEM_FAILED)
    {
        perror("bin_adder: Error: Failed to open nlogn semaphore");
        exit(EXIT_FAILURE);
    }

    /* numIntsToAdd is 2 for this calculation, so add an index
       and the following index, store it in the first index, and
       zero out the second index */   
    int i;
    for(i = 1; i < numIntsToAdd; i++)
    {
        smPtr-> integersTwo[index] += smPtr-> integersTwo[index + i];
        smPtr-> integersTwo[index + i] = 0;
    }
    
    //Critical section for writing to file
    char time[10];

    sleep(rand() % 4); //sleep for somewhere between 0-3 seconds
    timeSetter(time);
    fprintf(stderr, "%d waiting to enter the critical section at %s\n", getpid(), time);
    sem_wait(sem1); //waiting for its time to enter critcal section
    //Beginning Critical Section
    timeSetter(time);
    fprintf(stderr, "%d is inside the critical section at %s\n", getpid(), time);
    sleep(1); //Sleep another 1 second before writing to file
    //Write to file
    fprintf(file, "\t%d\t\t%d\t\t%d\t\t%d\t\t%s\n", getpid(), index, numIntsToAdd, smPtr-> integersTwo[index], time);       
    sleep(1); //Sleep another 1 second before leaving critical section
    timeSetter(time);
    fprintf(stderr, "%d is exiting the critical section at %s\n", getpid(), time);
    sem_post(sem1); //Critical section finished, signal semaphore   

}

//Deallocate shared memory but don't remove it
int deallocateMem(int shmid, void *shmaddr) 
{
    //If detaching fails it will return -1 so return -1 for deallocation call
    if(shmdt(shmaddr) == -1)
        return -1;
    //shmctl(shmid, IPC_RMID, NULL); Don't need to remove the shared memory
    return 0;
}

//Set the values for the current time to be indicated on stderr
void timeSetter(char *tStr)
{
    time_t ttime;
    struct tm *timeStruct;
    time(&ttime);
    timeStruct = localtime(&ttime);
    //Format the time accordingly
    sprintf(tStr, "%02d:%02d:%02d", timeStruct-> tm_hour, timeStruct-> tm_min, timeStruct-> tm_sec);
         
}
