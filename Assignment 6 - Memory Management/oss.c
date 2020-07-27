/*  Author: Chase Richards
    Project: Homework 6 CS4760
    Date April 23, 2020
    Filename: oss.c  */

#include "oss.h"

char *outputLog = "logOutput.dat";

int main(int argc, char* argv[])
{
    int c;
    int n = MAXPROCESSES; //Max Children in system at once
    int m = 0;
    srand(time(0));
    while((c = getopt(argc, argv, "hn:m:")) != -1)
    {
        switch(c)
        {
            case 'h':
                displayHelpMessage();
                return (EXIT_SUCCESS);
            case 'n':
                n = atoi(optarg);
                break;
            case 'm':
                m = atoi(optarg);
                break;
            default:
                printf("Using default values");
                break;               
        }
    }
  
    /* Signal for terminating, freeing up shared mem, killing all children 
       if the program goes for more than two seconds of real clock */
    signal(SIGALRM, sigHandler);
    alarm(2);

    /* Signal for terminating, freeing up shared mem, killing all 
       children if the user enters ctrl-c */
    signal(SIGINT, sigHandler);  
    
    manager(n, m); //Call scheduler function
    removeAllMem(); //Remove all shared memory, message queue, kill children, close file

    return 0;
}

/* Does the fork, exec and handles the messaging to and from user */
void manager(int maxProcsInSys, int memoryScheme)
{
    filePtr = openLogFile(outputLog); //open the output file
    
    /* Create the simulated clock in shared memory */
    clksim *clockPtr;
    clockSegment = shmget(clockKey, sizeof(clksim), IPC_CREAT | 0666);
    if(clockSegment < 0)
    {
        perror("oss: Error: Failed to get clock segment (shmget)");
        removeAllMem();
    }
    clockPtr = shmat(clockSegment, NULL, 0);
    if(clockPtr < 0)
    {
        perror("oss: Error: Failed to attach clock segment (shmat)");
        removeAllMem();
    }
    clockPtr-> sec = 0;
    clockPtr-> nanosec = 0;   
 
    /* Constant for the time between each new process and the time for
       spawning the next process, initially spawning one process */   
    clksim maxTimeBetweenNewProcesses = {.sec = 0, .nanosec = 500000000};
    clksim spawnNextProc = {.sec = 0, .nanosec = 0};

    /* Create the message queue */
    msgqSegment = msgget(messageKey, IPC_CREAT | 0777);
    if(msgqSegment < 0)
    {
        perror("oss: Error: Failed to get message segment (msgget)");
        removeAllMem();
    }
    msg message;

    sem_t *sem;
    char *semaphoreName = "semOss";

    int outputLines = 0; //Counts the lines written to file to make sure we don't have an infinite loop
    int procCounter = 0; //Counts the processes
    int printCounter = 1; //For printing every second

    //Statistics
    int totalProcs = 0;
    int memAccesses = 0;
    int pageFaults = 0;

    int i, j, k; //For loops
    int processExec; //exec  nd check for failurei
    int procPid; //generated pid
    int pid; //actual pid
    char msgqSegmentStr[10]; //for execing to the child
    char procPidStr[3]; //for execing the generated pid to child
    char schemeStr[2]; //for execing the scheme to child
    sprintf(msgqSegmentStr, "%d", msgqSegment);
    sprintf(schemeStr, "%d", memoryScheme);
    //Array of the pids in the generated pids index, initialized to -1 for available
    int *pidArr;
    pidArr = (int *)malloc(sizeof(int) * maxProcsInSys);
    for(i = 0; i < maxProcsInSys; i++)
        pidArr[i] = -1;

    int pageSize = 1024;
 
    if(memoryScheme == 1)
    {
        //Initialize the array of weights
        float weightValue;
        for(k = 1; k <= 32; k++)
        {
            weightValue = 1 / (float)k;
            clockPtr-> arrOfWeights[k - 1] = weightValue;
        }
    }

    //Allocate the frame table as an array
    frameTable *frameT = (frameTable *)malloc(sizeof(frameTable) * 256);
    //Initialize the frame table
    for(i = 0; i < 256; i++)
    {
        //Not occupied yet
        frameT[i].referenceBit = 0x0;
        frameT[i].dirtyBit = 0x0;
        frameT[i].process = -1;
    }
    //Allocate the page tables as an array
    pageTable *pageT = (pageTable *)malloc(sizeof(pageTable) * maxProcsInSys);
    //Initialize the page tables
    for(i = 0; i < 18; i++)
    {
        for(j = 0; j < 32; j++)
            pageT[i].pageArr[j].locationOfFrame = -1;
    }
  
    //Create the queue wait queue for swapping out a process
    questrt *circQueue = queueCreation(maxProcsInSys); 

    //Printing starting
    printf("Program Starting\n");

    //printf("ProcCounter: %d", procCounter);
    //Loop runs constantly until it has to terminate
    while(totalProcs <= 100 && outputLines < 120000)
    {
        //Only 18 processes in the system at once and spawn random between 0 and 500000
        if((procCounter < maxProcsInSys) && ((clockPtr-> sec > spawnNextProc.sec) || (clockPtr-> sec == spawnNextProc.sec && clockPtr-> nanosec >= spawnNextProc.nanosec)))
        {
            procPid = generateProcPid(pidArr, maxProcsInSys);
            if(procPid < 0)
            {
                perror("oss: Error: Max number of pids in the system\n");
                removeAllMem();               
            }     
            /* Copy the generated pid into a string for exec, fork, check
               for failure, execl and check for failure */      
            sprintf(procPidStr, "%d", procPid);
            pid = fork();
            if(fork < 0)
            {
                perror("oss: Error: Failed to fork process\n");
                removeAllMem();
            }
            else if(pid == 0)
            {
                processExec = execl("./user", "user", msgqSegmentStr, procPidStr, schemeStr, (char *)NULL);
                if(processExec < 0)
                {
                    perror("oss: Error: Failed to execl\n");
                    removeAllMem();
                }
            }
            totalProcs++;
            procCounter++;
            //Put the pid in the pid array in the spot based on the generated pid
            pidArr[procPid] = pid;
     
            //Get the time for the next process to run
            spawnNextProc = nextProcessStartTime(maxTimeBetweenNewProcesses, (*clockPtr));
   
            clockIncrementor(clockPtr, 100000000);  
        }
        /* Receive a message from a process, if it is non zero, return immediately,
           if it is zero wait for a message of the oss type to be placed on the queue 
           and handle the message depending on if it is read, write or terminate */
        else if((msgrcv(msgqSegment, &message, sizeof(msg), 20, IPC_NOWAIT)) > 0) 
        {
            //printf("%d\n", message.msgDetails);
            //Increment the clock for the read/write operation
            clockIncrementor(clockPtr, 1000);
            memAccesses++;
            //Check to see if the frame is available in the page table
            int frameLocation = pageT[message.process].pageArr[message.address / pageSize].locationOfFrame;
            //The message will either be 0,1,2 (read,write,or terminate)
            int receivedMessage = message.msgDetails;   
            //If the message is to terminate
            if(receivedMessage == 2)
            {
                int k;
                //Since we are terminating make the pid available again for a new process
                int process = pidArr[message.process];
                pidArr[message.process] = -1;
                //Make the frames and page table available again for others
                for(k = 0; k < 256; k++)
                {
                    //Reset the reference bit, dirty bit, and make the frame available
                    if(frameT[k].process == message.process)
                    {
                        frameT[k].referenceBit = 0x0;
                        frameT[k].dirtyBit = 0x0;
                        frameT[k].process = -1;
                    }
                }
                for(k = 0; k < 32; k++)
                {
                    pageT[message.process].pageArr[k].locationOfFrame = -1;
                }
                //Decrement the procs in system
                procCounter--;
                //Wait for the process
                waitpid(process, NULL, 0);
                //Output to the file that the process terminated
                fprintf(filePtr, "P%d terminated at time %d:%09d\n", message.process, clockPtr-> sec, clockPtr-> nanosec);         
                outputLines++;      
            }
            //If it's not terminating, it's either read or write
            else
            {
                //If it's a read print to output file its requesting read
                if(receivedMessage == 0)
                {
                    fprintf(filePtr, "P%d requesting read of address %d at time %d:%09d\n", message.process, message.address, clockPtr-> sec, clockPtr-> nanosec);
                    outputLines++;
                }
                //Otherwise it's a write so print that to output file
                else
                {
                    fprintf(filePtr, "P%d requesting write of address %d at time %d:%09d\n", message.process, message.address, clockPtr-> sec, clockPtr-> nanosec);
                    outputLines++;
                }
                
                //Let's say there is no page fault (always granted for now)
                if(frameT[frameLocation].process == message.process && frameLocation != -1)
                {
                    //If it was a read skip the dirty bit change
                    if(receivedMessage == 0)
                    {
                        fprintf(filePtr, "Address %d in frame %d, giving data to P%d at time %d:%09d\n", message.address, frameLocation, message.process, clockPtr-> sec, clockPtr-> nanosec);
                        outputLines++;
                        //Set the reference bit to 1
                        frameT[frameLocation].referenceBit = 0x1;
                    }
                    //Otherwise it was a write, so also set the dirty bit
                    else
                    {
                        //Output file the address and frame being giving to which process
                        fprintf(filePtr, "Address %d in frame %d, writing data by P%d at time %d:%09d\n", message.address, frameLocation, message.process, clockPtr-> sec, clockPtr-> nanosec);
                        outputLines++;
                        //Set the reference bit to 1
                        frameT[frameLocation].referenceBit = 0x1;
                        //Since it was a write we also need to set the dirty bit
                        frameT[frameLocation].dirtyBit = 0x1;
                        //Log that we changed the dirty bit to the output file
                        fprintf(filePtr, "Dirty bit of frame %d set, adding additional time to the clock\n", frameLocation);        
                        outputLines++; 
                        //Open the semaphore
                        sem = sem_open(semaphoreName, O_CREAT, 0644, 1);
                        //Increment the clock for the dirty bit change
                        clockIncrementor(clockPtr, 100);
                        //Signal the semaphore
                        sem_post(sem);
                    }
                    //Open the semaphore
                    sem = sem_open(semaphoreName, O_CREAT, 0644, 1);
                    //Increment the clock for no page fault - less than page fault 
                    clockIncrementor(clockPtr, 10);
                    //Signal the semaphore
                    sem_post(sem);
                }
                //If there is a page fault
                else
                {
                    //increment the page fault stat
                    pageFaults++;
                    //Queue the request for device
                    enqueue(circQueue, message.process);
                    //Returns the available frame if there is one otherwise -1
                    int frameLocation = findAvailFrame(frameT);
                    //If there are none available we have to do second chance algorithm to replace
                    if(frameLocation == -1)
                    {
                        //printf("No Frames Available\n");
                        frameLocation = clockReplacementPolicy(frameT, (*clockPtr));
                        //Swap the frame into the location that was thrown out and set reference bit to 1
                        frameT[frameLocation].process = message.process;
                        frameT[frameLocation].referenceBit = 0x1;
                        fprintf(filePtr, "Clearing frame %d and swapping in process P%d\n", frameLocation, message.process);
                        outputLines++;
                        //Read set the dirty bit to zero
                        if(receivedMessage == 0)
                            frameT[frameLocation].dirtyBit = 0x0;
                        else //Write set it to 1
                            frameT[frameLocation].dirtyBit = 0x1;
                        //Log that the dirty bit was set and clock incremented
                        fprintf(filePtr, "Dirty bit of frame %d set, adding additional time to the clock\n", frameLocation);
                        outputLines++;
                        //Open the semaphore
                        sem = sem_open(semaphoreName, O_CREAT, 0644, 1);
                        //Increment the clock for setting the dirty bit
                        clockIncrementor(clockPtr, 1000);
                        //Signal the semaphore
                        sem_post(sem);
                        //Finish by updating the page table
                        pageT[message.process].pageArr[message.address / pageSize].locationOfFrame = frameLocation;
                    }
                    //Insert the frame in the available location
                    else
                    {
                        //Fullfil the request at the head of the queue
                        dequeue(circQueue);
                        fprintf(filePtr, "Inserting P%d page into frame %d\n", message.process, frameLocation);
                        outputLines++;
                        //Insert the frame and set the reference bit
                        frameT[frameLocation].process = message.process;
                        frameT[frameLocation].referenceBit = 0x1;
                        //Set the dirty bit according to read or write
                        if(receivedMessage == 0)
                            frameT[frameLocation].dirtyBit = 0x0; //read
                        else
                            frameT[frameLocation].dirtyBit = 0x1; //write
                        //Lastly update the page table getting the page from the address / 1024
                        pageT[message.process].pageArr[message.address / pageSize].locationOfFrame = frameLocation;
                    }
                    //Open the semaphore
                    sem = sem_open(semaphoreName, O_CREAT, 0644, 1);
                    //Increment the clock for a page fault
                    clockIncrementor(clockPtr, 1000);
                    //Signal the semaphore
                    sem_post(sem);
                }
            }
        } 
     
        //Print the memory map every second showing the allocation of frames
        if(clockPtr-> sec == printCounter)
        {
            if(outputLines < 100000)
            {
                logFrameAllocation(frameT, *clockPtr);
                outputLines += 258;
            }
            printCounter++;
        }
        //Open the semaphore and increment the clock
        sem = sem_open(semaphoreName, O_CREAT, 0644, 1);    
        if(sem == SEM_FAILED)
        {
            perror("user: Error: Failed to open semaphore\n");
            exit(EXIT_FAILURE);
        }
        //Increment clock
        clockIncrementor(clockPtr, 1400000);
        //Signal semaphore
        sem_post(sem);
    }  

    printf("\nStatistics of Interest:\n");
    fprintf(filePtr, "\nStatistics of Interest:\n");

    //Calculate and print the memory accesses per second to the console and the end of the output file
    float memAccessesPerSec = ((float)(memAccesses) / ((float)(clockPtr-> sec) + ((float)clockPtr-> nanosec / (float)(1000000000))));
    printf("Memory accesses per second: %f\n", memAccessesPerSec);
    fprintf(filePtr, "Memory accesses per second: %f\n", memAccessesPerSec);

    //Calculate and print the number of page faults per memory access to the console and the end of the output file
    float pageFaultsPerMemAccess = ((float)(pageFaults) / (float)memAccesses);
    printf("Page faults per memory access: %f\n", pageFaultsPerMemAccess);
    fprintf(filePtr, "Page faults per memory access: %f\n", pageFaultsPerMemAccess);

    //Calculate the average memory access speed
    float avgMemAccessSpeed = (((float)(clockPtr-> sec) + ((float)clockPtr-> nanosec / (float)(1000000000))) / ((float)memAccesses));
    printf("Average memory access speed: %f\n", avgMemAccessSpeed);
    fprintf(filePtr, "Average memory access speed: %f\n", avgMemAccessSpeed);

    removeAllMem();
    return;  
}

//Generate a simulated process pid between 0-17
int generateProcPid(int *pidArr, int totalPids)
{
    int i;
    for(i = 0; i < totalPids; i++)
    {
        if(pidArr[i] == -1)
            return i;
    }
    return -1;
}

/* Determines when to launch the next process based on a random value
   between 0 and maxTimeBetweenNewProcs and returns the time that it should launch */
clksim nextProcessStartTime(clksim maxTimeBetweenProcs, clksim curTime)
{
    clksim nextProcTime = {.sec = (rand() % (maxTimeBetweenProcs.sec + 1)) + curTime.sec, .nanosec = (rand() % (maxTimeBetweenProcs.nanosec + 1)) + curTime.nanosec};
    if(nextProcTime.nanosec >= 1000000000)
    {
        nextProcTime.sec += 1;
        nextProcTime.nanosec -= 1000000000;
    }
    return nextProcTime;
}

/* For sending the message to the processes */
void messageToProcess(int receivingProcess, int response)
{
    int sendmessage;
    //Process is -1 because we didn't generate one, no resource, oss is sending process of 1
    msg message = {.typeofMsg = receivingProcess, .msgDetails = response, .process = -1, .address = -1};
    
    //Send the message and check for failure
    sendmessage = msgsnd(msgqSegment, &message, sizeof(msg), 0);
    if(sendmessage == -1)
    {
        perror("oss: Error: Failed to send message (msgsnd)\n");
        removeAllMem();
    }
    return;
}

/* Return the first availble frame, if we are out return -1 */
int findAvailFrame(frameTable *frameT)
{
    int l;
    for(l = 0; l < 256; l++)
    {
        if(frameT[l].process == -1)
            return l;
    }
    return -1;
}

/* Frame replacement algorithm if the ref is 0 then it can be replaced, if ref bit is 1 reset it to 0 
   and continue to the next page, the reference bit would be 1 if it has been referred to in the past, 
   to give the second chance reset the ref bit and set the arrival time to the current time, the ref 
   bit would be 0 if it had not been ref in a while. Page is eligible for replacement if it has a ref bit
   of 0 either originally or due to the algorithm giving the page a second chance and resetting the ref bit */
int clockReplacementPolicy(frameTable *frameT, clksim curTime)
{
    int n = 0;
    int replacedFrameIndex = 0;
    while(1)
    {
        //If the reference bit is 1 reset the reference bit and continue to next page
        if(frameT[n].referenceBit == 1)
        {
            //reset the reference bit
            frameT[n].referenceBit = 0;
            //set the page arrival time to the current time
            frameT[n].arrivalTime = curTime;
            //Increment n to the next page and continue checking
            if(n != 255)
                n++;
            //Otherwise reset n to zero if it has gone through all pages
            else
                n = 0;       
        }
        //If it has a ref bit of 0 or is a second chance page then it can be replaced
        else
        {
            //Set n to the index of the replaceable page
            replacedFrameIndex = n;
            if(n != 255)
                n++;   
            else
                n = 0;
            //return the page to replace
            return replacedFrameIndex;
        }
    }
}

/* Print Allocation of Frames according to the assignment sheet */
void logFrameAllocation(frameTable *frameT, clksim curTime)
{
    int m;
    //Header for the current memory layout
    fprintf(filePtr, "Current memory layout at time %d:%09d is:\n", curTime.sec, curTime.nanosec);
    fprintf(filePtr, "           Occupied  RefByte  DirtyBit\n");
    //Loop through all of the frames
    for(m = 0; m < 256; m++)
    {
        //If the process is -1, it is unoccupied
        if(frameT[m].process < 0)
        {
            fprintf(filePtr, "Frame %3d: %-9s %-8d %-8d\n", m, ".", frameT[m].referenceBit, frameT[m].dirtyBit); 
        }
        //Otherwise it is occupied
        else
        {
            fprintf(filePtr, "Frame %3d: %-9s %-8d %-8d\n", m, "+", frameT[m].referenceBit, frameT[m].dirtyBit);
        }
    }   
    return;
}

/* Create the queue and set the capacity and the number of items*/
questrt *queueCreation(int capacity)
{
    int i;
    questrt *queuePtr = (questrt *)malloc(sizeof(questrt));
    queuePtr-> items = 0;
    queuePtr-> front = 0;
    queuePtr-> back = 0;
    queuePtr-> capacity = capacity;
    queuePtr-> arr = (int *)malloc(capacity * sizeof(int));
    for(i = 0; i < capacity; i++)
        queuePtr-> arr[i] = -1;
    return queuePtr;
}

/* Add a process to the queue and update the number of items in the queue*/
void enqueue(questrt *queuePtr, int pid)
{
    queuePtr-> items += 1;
    queuePtr-> arr[queuePtr-> back] = pid;
    queuePtr-> back = (queuePtr-> back + 1) % queuePtr-> capacity;
    return;
}

/* Remove a process from the queue and update the number of items in the queue*/
int dequeue(questrt *queuePtr)
{
    int pid;
    queuePtr-> items -= 1;
    pid = queuePtr-> arr[queuePtr-> front];
    queuePtr-> front = (queuePtr-> front + 1) % queuePtr-> capacity;
    return pid;
}

/* Open the log file that contains the output and check for failure */
FILE *openLogFile(char *file)
{
    filePtr = fopen(file, "a");
    if(filePtr == NULL)
    {
        perror("oss: Error: Failed to open output log file");
        exit(EXIT_FAILURE);
    }
    return filePtr;
}

/* When there is a failure, call this to make sure all memory is removed */
void removeAllMem()
{
    //printStats();   
    shmctl(clockSegment, IPC_RMID, NULL);
    msgctl(msgqSegment, IPC_RMID, NULL);
    sem_unlink("semOss");
    sem_unlink("semUser");
    kill(0, SIGTERM);
    fclose(filePtr);   
    exit(EXIT_SUCCESS);
} 

/* Print the final statistics to the console and the end of the file - will be called in signal handler
void printStats()
{
    //Calculate and print the memory accesses per second to the console and the end of the output file
    float memAccessesPerSec = ((float)(memAccesses) / ((float)(clockPtr-> sec) + ((float)clockPtr-> nanosec / (float)(1000000000))));
    printf("Memory accesses per second: %f\n", memAccessesPerSec);
    fprintf(filePtr, "Memory accesses per second: %f\n", memAccessesPerSec);
    //Calculate and print the number of page faults per memory access to the console and the end of the output file
    float pageFaultsPerMemAccess = ((float)(pageFaults) / (float)memAccesses);
    printf("Page faults per memory access: %f\n", pageFaultsPerMemAccess);
    fprintf(filePtr, "Page faults per memory access: %f\n", pageFaultsPerMemAccess);
    //Calculate the average memory access speed
    float avgMemAccessSpeed = (((float)(clockPtr-> sec) + ((float)clockPtr-> nanosec / (float)(1000000000))) / ((float)memAccesses));
    printf("Average memory access speed: %f\n", avgMemAccessSpeed);
    fprintf(filePtr, "Average memory access speed: %f\n", avgMemAccessSpeed);
}
*/

/* Signal handler, that looks to see if the signal is for 2 seconds being up or ctrl-c being entered.
   In both cases, print the final statistics and remove all of the memory, semaphores, and message queues */
void sigHandler(int sig)
{
    //printStats();
    if(sig == SIGALRM)
    {
        //printStats();
        printf("Timer is up.\n"); 
        printf("Killing children, removing shared memory, semaphore and message queue.\n");
        removeAllMem();
        exit(EXIT_SUCCESS);
    }
    
    if(sig == SIGINT)
    {
        printf("Ctrl-c was entered\n");
        printf("Killing children, removing shared memory, semaphore and message queue\n");
        removeAllMem();
        exit(EXIT_SUCCESS);
    }
}


/* For the -h option that can be entered */
void displayHelpMessage() 
{
    printf("\n---------------------------------------------------------\n");
    printf("See below for the options:\n\n");
    printf("-h    : Instructions for running the project and terminate.\n"); 
    printf("-n x  : Number of processes allowed in the system at once (Default: 18).\n");
    printf("-m x  : Determines how a child will perform their memory access (Default: 0).\n");
    printf("\n---------------------------------------------------------\n");
    printf("Examples of how to run program(default and with options):\n\n");
    printf("$ make\n");
    printf("$ ./oss\n");
    printf("$ ./oss -n 10 -m 1\n");
    printf("$ make clean\n");
    printf("\n---------------------------------------------------------\n"); 
    exit(0);
}
