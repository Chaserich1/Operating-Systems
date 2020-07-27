/*  Author: Chase Richards
    Project: Homework 5 CS4760
    Date April 4, 2020
    Filename: oss.c  */

#include "oss.h"

//Stats
int granted = 0;
int normalTerminations = 0;
int deadlockTerminations = 0;
int deadlockAlgRuns = 0;
int totalCounter = 0;
char *outputLog = "logOutput.dat";

int main(int argc, char* argv[])
{
    int c;
    int n = 18; //Max Children in system at once
    int verbose = 2;
    srand(time(0));
    while((c = getopt(argc, argv, "hn:v")) != -1)
    {
        switch(c)
        {
            case 'h':
                displayHelpMessage();
                return (EXIT_SUCCESS);
            case 'n':
                n = atoi(optarg);
                break;
            case 'v':
                verbose = 1;
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
    
    manager(n, verbose); //Call scheduler function
    removeAllMem(); //Remove all shared memory, message queue, kill children, close file

    return 0;
}

/* Does the fork, exec and handles the messaging to and from user */
void manager(int maxProcsInSys, int verbose)
{
    filePtr = openLogFile(outputLog); //open the output file
    
    int receivedMsg; //Recieved from child telling scheduler what to do
    
    /* Create resource descriptor shared memory */ 
    resDesc *resDescPtr;
    resDescSegment = shmget(resDescKey, sizeof(int) * 18, IPC_CREAT | 0666);
    if(resDescSegment < 0)
    {
        perror("oss: Error: Failed to get resource desriptor segment (shmget)");
        removeAllMem();
    }
    resDescPtr = shmat(resDescSegment, NULL, 0);
    if(resDescPtr < 0)
    {
        perror("oss: Error: Failed to attach to resource descriptor segment (shmat)");
        removeAllMem();
    }
    resDescConstruct(resDescPtr);
 
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

    //Statistics
    int granted1;
    int totalProcs = 0;

    int i; //For loops
    int processExec; //exec  nd check for failurei
    int deadlockDetector = 0; //deadlock flag
    int procPid; //generated pid
    int pid; //actual pid
    char msgqSegmentStr[10]; //for execing to the child
    char procPidStr[3]; //for execing the generated pid to child
    sprintf(msgqSegmentStr, "%d", msgqSegment);
    //Array of the pids in the generated pids index, initialized to -1 for available
    int *pidArr;
    pidArr = (int *)malloc(sizeof(int) * maxProcsInSys);
    for(i = 0; i < maxProcsInSys; i++)
        pidArr[i] = -1;

    //Printing the inital allocated matrix showing that nothing is allocated
    fprintf(filePtr, "Program Starting - No Resources Allocated to Processes: \n");
    printTable((*resDescPtr), maxProcsInSys, 20);
    outputLines += 20;

    //Loop runs constantly until it has to terminate
    while(1)
    {
        //Only 18 processes in the system at once and spawn random between 0 and 5000000000
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
                processExec = execl("./user", "user", msgqSegmentStr, procPidStr, (char *)NULL);
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
   
            if(outputLines < 100000 && verbose == 1)
            {
                fprintf(filePtr, "OSS has spawned process P%d at time %d:%09d\n", procPid, clockPtr-> sec, clockPtr-> nanosec);
                outputLines++;
                
            }
        }
        else if((msgrcv(msgqSegment, &message, sizeof(msg), 1, IPC_NOWAIT)) > 0) 
        {
            //printf("Message Handling");
            //Process is requesting a resource
            if(message.msgDetails == 0)
            {
                //Write the request to the log file for verbose
                if(outputLines < 100000 && verbose == 1)
                {
                    fprintf(filePtr, "Oss has detected P%d requesting R%d at time %d:%09d\n", message.process, message.resource, clockPtr-> sec, clockPtr-> nanosec);
                    outputLines++;
                   
                }
                //If the process is requesting a resource that is shared
                if(resDescPtr-> resSharedOrNot[message.resource] == 1)
                {
                    //Make sure it doesn't have more than available of the shared resource
                    if(resDescPtr-> allocated2D[message.process][message.resource] < 7)
                    {
                        //Grant the request for resource and send message to process that it was granted
                        resDescPtr-> allocated2D[message.process][message.resource] += 1;
                        granted++;
                        messageToProcess(message.processesPid, 3);
                        if(outputLines < 100000 && verbose == 1)
                        {
                            fprintf(filePtr, "Oss granting P%d request for R%d at time %d:%09d\n", message.process, message.resource, clockPtr-> sec, clockPtr-> nanosec);
                            outputLines++;
                        }                
                    }
                    //Otherwise deny the request and send denied message to process
                    else
                    {
                        resDescPtr-> requesting2D[message.process][message.resource] += 1;
                        messageToProcess(message.processesPid, 4);
                        if(outputLines < 100000 && verbose == 1)
                        {
                            fprintf(filePtr, "Oss denying P%d request for R%d at time %d:%09d\n", message.process, message.resource, clockPtr-> sec, clockPtr-> nanosec);
                            outputLines++;
                        }   
                    }
                }
                /* Requesting a nonshareable resource, check if there are any left */
                else if(resDescPtr-> availableResources[message.resource] > 0)
                {
                    //Remove it from the free resources and add it to the 2D array for the process
                    resDescPtr-> availableResources[message.resource] -= 1;
                    resDescPtr-> allocated2D[message.process][message.resource] += 1;
                    //Let child know the request was granted
                    granted++;
                    messageToProcess(message.processesPid, 3);
                    if(outputLines < 100000 && verbose == 1)
                    {
                        fprintf(filePtr, "Oss granting P%d request for R%d at time %d:%09d\n", message.process, message.resource, clockPtr-> sec, clockPtr-> nanosec);
                        outputLines++;
                    }
                }
                /* Otherwise there are no instances of the nonshareable resource available */
                else
                {
                    //Add one to the request table for the process and send deny message to child
                    resDescPtr-> requesting2D[message.process][message.resource] += 1;
                    messageToProcess(message.processesPid, 4);
                    if(outputLines < 100000 && verbose == 1)
                    {
                        fprintf(filePtr, "Oss denying P%d request for R%d at time %d:%09d\n", message.process, message.resource, clockPtr-> sec, clockPtr-> nanosec);
                        outputLines++;
                    }
                }     
   
            }
            /* If the message is for releasing resources */
            else if(message.msgDetails == 1)
            {
                //If there are resources allocated to the process
                if(resDescPtr-> allocated2D[message.process][message.resource] > 0)
                {
                    //If it's not shared, release, remove from allocated table and let the process know
                    if(resDescPtr-> resSharedOrNot[message.resource] == 0)
                        resDescPtr-> availableResources[message.resource] += 1;
                    resDescPtr-> allocated2D[message.process][message.resource] -= 1;
                    messageToProcess(message.processesPid, 3);
                    if(outputLines < 100000 && verbose == 1)
                    {
                        fprintf(filePtr, "Oss has acknowledged P%d releasing R%d at time %d:%09d\n", message.process, message.resource, clockPtr-> sec, clockPtr-> nanosec);
                        outputLines++;
                    }
                }
                //Otherwise deny because the child doesn't have the resource
                else
                    messageToProcess(message.processesPid, 4);
            }
            /* If it is time to terminate the process */
            else if(message.msgDetails == 2)
            {
                int pidWaiting;
                //Resources are available once again
                for(i = 0; i < 20; i++)
                {
                    /* Removce all of the requests and allocated (release all resources)
                       Add all of the allocated resources back to the available */
                    if(resDescPtr-> resSharedOrNot[i] == 0)
                        resDescPtr-> availableResources[i] += resDescPtr-> allocated2D[message.process][i];
                    resDescPtr-> allocated2D[message.process][i] = 0;
                    resDescPtr-> requesting2D[message.process][i] = 0;
                }
                //The simulated pid is available now, wait for process completion
                pidArr[message.process] = -1;
                procCounter -= 1;
                normalTerminations++;
                pidWaiting = waitpid(message.processesPid, NULL, 0);
                if(outputLines < 100000 && verbose == 1)
                {
                    fprintf(filePtr, "Oss has acknowledged P%d terminating normally at time %d:%09d\n", message.process, clockPtr-> sec, clockPtr-> nanosec);
                    outputLines++;
                }
            }
        }
       
        /* Every 20 granted requests, print the allocated grid */ 
        if(granted % 20 == 0 && granted != 0 && granted != granted1 && verbose == 1)
        {
            granted1 = granted;
            printTable((*resDescPtr), maxProcsInSys, 20);
            outputLines += 19;
        }
        /*
        for(i = 0; i < 18; i++)
        {
            for(j = 0; j < 20; j++)
            {
                granted += resDescPtr-> allocated2D[i][j];
                if(granted >= 20)
                {
                    printTable((*resDescPtr), maxProcsInSys, 20);
                    outputLines += 19;
                    break;   
                }
            }
        }*/        

        //printf("%d\n", clockPtr-> nanosec);
        //Check for a deadlock every second
        if(clockPtr-> nanosec == 0)
        {
            //printf("Check for deadlock");
            //Call the deadlock function from notes (which will also terminate the first detected deadlock)
            deadlockDetector = deadlock(resDescPtr, maxProcsInSys, clockPtr, pidArr, &procCounter, &outputLines); 
         
            if(deadlockAlgRuns <= clockPtr-> sec)
            {
                deadlockAlgRuns++;
            } 
            /* If there was a deadlock on the first check above then run the deadlock detecting algorithm again
               repeatedly until the deadlock algorithm function returns 0 meaning there is no longer a deadlock */
            while(deadlockDetector == 1 && outputLines < 100000)
            { 
                fprintf(filePtr, "   Oss running deadlock detector after process kill\n");
                outputLines++;
                deadlockDetector = deadlock(resDescPtr, maxProcsInSys, clockPtr, pidArr, &procCounter, &outputLines);
                if(deadlockDetector == 0)
                {
                    fprintf(filePtr, "   System is no longer in deadlock\n");   
                    outputLines++;
                }
            }
            
        }        
        
        //Open the semaphore for writing to shared memory clock
        sem = sem_open(semaphoreName, O_CREAT, 0644, 1);
        if(sem == SEM_FAILED)
        {
            perror("oss: Error: Failed to open semaphore (sem_open)\n");
            exit(EXIT_FAILURE);
        }
        //Increment the clock
        clockIncrementor(clockPtr, 1000000);
        //Signal the semaphore we are finished
        sem_post(sem);      
    }    
    
    //Will never actually get here because of the infinite loop, so moving to alarm signal handler
    //printf("%d", clockPtr-> sec); 
    //printf("Total Granted Requests: %d\n", granted);
    //printf("Total Normal Terminations: %d\n", normalTerminations);
    //printf("Total Deadlock Algorithm Runs: %d\n", deadlockAlgRuns);
    //printf("Total Deadlock Terminations: %d\n", deadlockTerminations);
    //printf("Pecentage of processes in deadlock that had to terminate on avg: \n");
    
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
void messageToProcess(int receiver, int response)
{
    int sendmessage;
    //Process is -1 because we didn't generate one, no resource, oss is sending process of 1
    msg message = {.typeofMsg = receiver, .msgDetails = response, .process = -1, .resource = -1, .processesPid = 1};
    
    //Send the message and check for failure
    sendmessage = msgsnd(msgqSegment, &message, sizeof(msg), 0);
    if(sendmessage == -1)
    {
        perror("oss: Error: Failed to send message (msgsnd)\n");
        removeAllMem();
    }
    return;
}

/* req_lt_avail from notes */
int req_lt_avail(int req[], int avail[], int shared[], int held[])
{
    int i;
    //Loop over the process requests
    for(i = 0; i < 20; i++)
    {
        //If it is more than the available shared requests break (returning false 0)
        if(shared[i] == 1 && req[i] > 0 && held[i] == 7)
            break;
        //If the request is greater than available break (returning false 0)
        else if(req[i] > avail[i])
            break;
    }
    //If you go over complete loop then you have enough to satisfy the request
    return (i == 20);
}

/* Deadlock algorithm from notes */
int deadlock(resDesc *resDescPtr, int nProcs, clksim *clockPtr, int *pidArr, int *procCounter, int *outputLines)
{
    int i, p; //loops
    int work[20]; //avail from avail vector
    int finish[nProcs]; //whether the process is going to finish
    int deadlockedProcs[nProcs]; //deadlocked procs
    int counter = 0; //counter for deadlocked procs
    int releasedValues[20]; //Released resources
    //Initialize work from available resources
    for(i = 0; i < 20; i++)
        work[i] = resDescPtr-> availableResources[i];
    //Initialize as they won't finish
    for(i = 0; i < nProcs; i++)
        finish[i] = 0;
    //For each process
    for(p = 0; p < nProcs; p++)
    {
        //If we know this process is done, continue
        if(finish[p])
            continue;
        //Check the function above whether it can have a shared resource or whether request < available
        if((req_lt_avail(resDescPtr-> requesting2D[p], resDescPtr-> availableResources, resDescPtr-> resSharedOrNot, resDescPtr-> allocated2D[p])))
        {
            //It will finish
            finish[p] = 1;
            //Take everything allocated to the process and add to the work vector 
            for(i = 0; i < 20; i++)
                work[i] += resDescPtr-> allocated2D[p][i];
            //set process index to -1
            p = -1;
        }
    }
    //If request > available reset the p, if it is continue to the next one
    for(p = 0; p < nProcs; p++)
    {
        if(!finish[p])
            deadlockedProcs[counter++] = p;
    }
  
    totalCounter += counter; 
    /* Deadlock resolution: Kill and release deadlocked process with lowest generated process id, then check 
       for deadlock again, if there is still a deadlock, kill and repeat. If no deadlock continue on with 
       the program. */
    if(counter > 0)
    {
        //For the deadlocked processes
        for(i = 0; i < counter; i++)
        {
            if((*outputLines) < 100000)
            {
                fprintf(filePtr, "   Oss detected that P%d is deadlocked at time %d:%09d\n", deadlockedProcs[i], clockPtr-> sec, clockPtr-> nanosec);
                fprintf(filePtr, "   Attempting to resolve deadlock\n");
                (*outputLines) += 2;
            }
            
            int j; 
            //Terminate by killing the process
            kill(pidArr[deadlockedProcs[i]], SIGKILL);
            waitpid(pidArr[deadlockedProcs[i]], NULL, 0);
            deadlockTerminations++;
            //Release the resources
            for(j = 0; j < 20; j++)
            {
                if(resDescPtr-> resSharedOrNot[j] == 0)
                {
                    //Add the allocated resources back to the available vector
                    resDescPtr-> availableResources[j] += resDescPtr-> allocated2D[deadlockedProcs[i]][j];
                    //Add the allocated resources to the released vector
                    releasedValues[j] += resDescPtr-> allocated2D[deadlockedProcs[i]][j];
                }
                //If there is a resource released, print the process, resource, and amount being released
                if(resDescPtr-> allocated2D[deadlockedProcs[i]][j])
                    fprintf(filePtr, "   P%d released resource: R%d:%d\n", deadlockedProcs[i], j, resDescPtr-> allocated2D[deadlockedProcs[i]][j]);
                //Clear the allocated and requests for this processes resources
                resDescPtr-> allocated2D[deadlockedProcs[i]][j] = 0;
                resDescPtr-> requesting2D[deadlockedProcs[i]][j] = 0;
            }

            //Add the pid back to the available pids and decrement the procs in the system so a new one can launch
            pidArr[deadlockedProcs[i]] = -1;
            (*procCounter)--;
            
            if((*outputLines) < 100000)
            {
                fprintf(filePtr, "   Oss killing process P%d\n", deadlockedProcs[i]);
                (*outputLines)++;
            }
            return 1; //return 1 because there is still another deadlock
        }
        return 1; //return 1 because there is still another deadlock
    }
    return 0; //return 0 because there is no deadlocks in the counter
}

/* Print Allocation Matrix according to the assignment sheet */
void printAllocatedTable(int allocated2D[18][20], int processes, int resources)
{
    int mRow, mColumn;
    fprintf(filePtr, "Allocated Matrix\n   ");
    //Print the resource column names R0-R19
    for(mColumn = 0; mColumn < resources; mColumn++)
    {
        fprintf(filePtr, "R%-2d ", mColumn);
    }
    fprintf(filePtr, "\n");
    //Print the process row names P0-P17
    for(mRow = 0; mRow < processes; mRow++)
    {
        fprintf(filePtr, "P%-2d ", mRow);
        //Loop through each spot in the table
        for(mColumn = 0; mColumn < resources; mColumn++)
        {
            //If the spot is not allocated, print 0
            if(allocated2D[mRow][mColumn] == 0)
                fprintf(filePtr, "0   ");
            //Otherwise print the allocated value
            else
                fprintf(filePtr, "%-3d ", allocated2D[mRow][mColumn]);           
        }
        fprintf(filePtr, "\n");
    }
    return;
}

/* Function for printing the allocated values (will be called roughly every 20 granted requests */
void printTable(resDesc resDescPtr, int processes, int resources)
{
    printAllocatedTable(resDescPtr.allocated2D, processes, resources);
    return;
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
    shmctl(resDescSegment, IPC_RMID, NULL);   
    shmctl(clockSegment, IPC_RMID, NULL);
    msgctl(msgqSegment, IPC_RMID, NULL);
    sem_unlink("semOss");
    sem_unlink("semUser");
    kill(0, SIGTERM);
    fclose(filePtr);   
    exit(EXIT_SUCCESS);
} 
//Used for the average deadlock termiantion calculation
int divideNums(int a, int b)
{
    if(a == 0)
        return 0;

    float answer = (float)a / b;   
 
    return answer * 100;
}

/* Print the final statistics to the console and the end of the file - will be called in signal handler */
void printStats()
{
    fprintf(filePtr, "Total Granted Requests: %d\n", granted);
    fprintf(filePtr, "Total Normal Terminations: %d\n", normalTerminations);
    fprintf(filePtr, "Total Deadlock Algorithm Runs: %d\n", deadlockAlgRuns);
    fprintf(filePtr, "Total Deadlock Terminations: %d\n", deadlockTerminations);
    fprintf(filePtr, "Pecentage of processes in deadlock that had to terminate on avg: %d%\n", divideNums(deadlockTerminations, totalCounter));
    printf("Total Granted Requests: %d\n", granted);
    printf("Total Normal Terminations: %d\n", normalTerminations);
    printf("Total Deadlock Algorithm Runs: %d\n", deadlockAlgRuns);
    printf("Total Deadlock Terminations: %d\n", deadlockTerminations);
    printf("Pecentage of processes in deadlock that had to terminate on avg: %d%\n", divideNums(deadlockTerminations, totalCounter));
}

/* Signal handler, that looks to see if the signal is for 2 seconds being up or ctrl-c being entered.
   In both cases, print the final statistics and remove all of the memory, semaphores, and message queues */
void sigHandler(int sig)
{
    printStats();
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
    printf("-v    : Option to turn verbose on: indicates requests and releases of resources (Default: off).\n");
    printf("\n---------------------------------------------------------\n");
    printf("Examples of how to run program(default and with options):\n\n");
    printf("$ make\n");
    printf("$ ./oss\n");
    printf("$ ./oss -n 10 -v\n");
    printf("$ make clean\n");
    printf("\n---------------------------------------------------------\n"); 
    exit(0);
}
