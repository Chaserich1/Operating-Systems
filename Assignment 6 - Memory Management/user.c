/*  Author: Chase Richards
    Project: Homework 6 CS4760
    Date April 23, 2020
    Filename: user.c  */

#include "oss.h"

int msgqSegment;

int main(int argc, char *argv[])
{
    clksim *clockPtr;    
 
    /* Get and attach to the clock simulation shared memory */
    clockSegment = shmget(clockKey, sizeof(clksim), IPC_CREAT | 0666);
    if(clockSegment < 0)
    {
        perror("user: Error: Failed to get clock segment (shmget)");
        exit(EXIT_FAILURE);
    }
    clockPtr = shmat(clockSegment, NULL, 0);
    if(clockPtr < 0)
    {
        perror("user: Error: Failed to attach clock (shmat)");
        exit(EXIT_FAILURE);
    }      

    //Semaphore Declarations
    sem_t* sem;
    char *semaphoreName = "semUser";
 
    //Will be used for messaging with oss
    int procPid, scheme;
    //Array of weights - one value for each page
    float lastValue;
    int pageToRequest, memoryAddress;
    //Passed the message queue segment id through execl
    msgqSegment = atoi(argv[1]);
    //Passed the generated process id through execl
    procPid = atoi(argv[2]);
    //Passed the scheme type through execl
    scheme = atoi(argv[3]);
    //Memeory Reference counter
    int memReferences = 0; 
 
    //Random 
    srand(time(0));
    int randMemRefCheck = (rand() % (1100 - 900 + 1)) + 900;
    
    //First memory request scheme
    if(scheme == 0)
    {
        //Continuous loop until it's time to terminate
        while(1)
        {
            memReferences++;  //Increment the number of memory references
            //Every 1000 +- 100 memory references check if it should terminate
            if(memReferences % randMemRefCheck == 0)
            {
                //Random value to check if it should terminate
                if(rand() % 100 > 50)
                {
                    //Send termination message to oss and actually terminate
                    messageToOss(procPid, 0, 2);
                    return 0;               
                }
            }   
     
            //Read or write with bias towards read
            int readOrWrite = rand() % 5 > 0 ? 0 : 1;
        
            //Send the message to Oss with the first scheme 
            messageToOss(procPid, firstScheme(), readOrWrite);

            //Open the semaphore and increment the shared clock
            sem = sem_open(semaphoreName, O_CREAT, 0700, 1);    
            if(sem == SEM_FAILED)
            {
                perror("user: Error: Failed to open semaphore\n");
                exit(EXIT_FAILURE);
            }
            //Increment clock
            clockIncrementor(clockPtr, 1000);
            //Signal semaphore
            sem_unlink(semaphoreName);
        }
    }
    //Second memory request scheme (1/n)
    else
    {
        //Continuous loop until it's time to terminate
        while(1)
        {
            memReferences++; //Increment the number of memory references
            
            //Add the each index of the array to the preceding value
            int i;
            for(i = 0; i < 31; i++)
            {
                clockPtr-> arrOfWeights[i + 1] += clockPtr-> arrOfWeights[i];
            }
            //Store the lastValue in the array of weights
            lastValue = clockPtr-> arrOfWeights[i];
            //Generate a random number from 0 to the last value
            int randomValue = (rand() % (int)lastValue + 1);
            //Travel down the array until a value greater than randomValue is found
            int j;
            for(j = 0; j < 32; j++)
            {
                //If randomValue is less than a weight
                if(randomValue < clockPtr-> arrOfWeights[j])
                {
                    //The page to request is the index of the greater value
                    pageToRequest = j;
                    break;
                }
            }
            //Now have a page to request but need offset
            //Multiply the page by 1024
            int newValue = pageToRequest * 1024;
            //Generate a random offset between 0 and 1023
            int offset = rand() % 1023;
            //To get the memory address add the two values
            memoryAddress = newValue + offset;
            
            //Every 1000 +- 100 memory references check if it should terminate
            if(memReferences % randMemRefCheck == 0)
            {
                //Random value to check if it should terminate
                if(rand() % 10 > 4)
                {
                    //Send termination message to oss and actually terminate
                    messageToOss(procPid, 0, 2);
                    return 0;               
                }
            }   
     
            //Read or write with bias towards read
            int readOrWrite = rand() % 5 > 0 ? 0 : 1; 
            //Send the message to Oss with the second scheme 
            messageToOss(procPid, memoryAddress, readOrWrite);     

            //Open the semaphore and increment the shared memory clock
            sem = sem_open(semaphoreName, O_CREAT, 0700, 1);    
            if(sem == SEM_FAILED)
            {
                perror("user: Error: Failed to open semaphore\n");
                exit(EXIT_FAILURE);
            }
            //Increment clock
            clockIncrementor(clockPtr, 10000);
            //Signal semaphore
            sem_unlink(semaphoreName);   
        }   
    } 
    

    return 0;
}

/* Message being sent to oss for read write or terminating */
void messageToOss(int curProcess, int address, int details)
{
    /* Send the message to oss with type 1 and it's a request */
    msg message = {.typeofMsg = 20, .process = curProcess, .address = address, .msgDetails = details};
    /* Send the message and check for failure */
    msgsnd(msgqSegment, &message, sizeof(msg), 0);
          
    return;
}

int firstScheme()
{
    return rand() % 32768;
}
