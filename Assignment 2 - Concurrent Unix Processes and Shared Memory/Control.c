/* Author: Chase Richards
   Project: Homework 2 CS4760
   Date February 13, 2020  
   Filename: Control.c  */

#include "Control.h"

int main(int argc, char* argv[]) 
{   
    int c;
    //Setting the default option values
    int maxChildren = 4, childLimit = 2, startOfSeq = 101, incrementVal = 4;
    char *outFile = "output.dat";

    /* GetOpt switch for the options being passed in */
    while((c = getopt(argc, argv, "hn:s:b:i:o:")) != -1) 
    {
        switch(c) 
        {
            case 'h':
                displayHelpMessage();
                break;
            case 'n':
                maxChildren = atoi(optarg);
                //printf("%d\n", maxChildren);
                break;
            case 's':
                childLimit = atoi(optarg);
                //printf("%d\n", childLimit);
                if(childLimit > 20)
                {
                    printf("The max number of children allowed in the system is 20, setting to -s 20\n");
                    childLimit = 20;
                }
                break;
            case 'b':
                startOfSeq = atoi(optarg);
                //printf("%d\n", startOfSeq);
                break;
            case 'i':
                incrementVal = atoi(optarg);
                //printf("%d\n", incrementVal);
                break;
            case 'o':
                outFile = optarg;
                //printf("%s\n", outFile);
                break;
            case '?':
                perror("oss: Error: Invalid option, use -h to see the available options.\n");
                exit(EXIT_FAILURE); 
        }
    }
    
    /* Signal for terminating, freeing up shared mem, killing all children 
       if the program goes for more than two seconds of real clock */
    signal(SIGALRM, sigHandler);
    alarm(2);

    /* Signal for terminating, freeing up shared mem, killing all 
       children if the user enters ctrl-c */
    signal(SIGINT, sigHandler);   
    
    /* Function that creates and connects to the shared memory segment 
       then launches the children launching function */
    sharedMemoryWork(maxChildren, childLimit, startOfSeq, incrementVal, outFile); 
   
    return 0;   
}

void sharedMemoryWork(int maxChildren, int childLimit, int startOfSeq, int incrementVal, char* outFile) 
{

    int sharedMemSegment, sharedMemDetach;
    char *sharedMemAttach;
    key_t key;

    //Key returns a key based on the path and id
    key = ftok(".",'m');
    //printf("%d", key);

    if(key == -1)
    {
        perror("oss: Error: getting the shared memory key");
        exit(EXIT_FAILURE);
    }    

    //Allocate the shared memory using shmget
    sharedMemSegment = shmget(key, sizeof(struct sharedMemory), IPC_CREAT | 0644);

    //If shmget is unsuccessful it retruns -1 so check for this
    if(sharedMemSegment == -1) 
    {
        perror("oss: Error: shmget failed to allocate shared memory");
        exit(EXIT_FAILURE);
    }

    //Attach the memory to our space
    smPtr = (struct sharedMemory *)shmat(sharedMemSegment, NULL, 0);
    //int *sharedMemAttachInt = (int *)sharedMemAttach;    

    //If shmat is unsuccessful it returns -1 so check for this
    if(smPtr == (void *)-1)
    {
        perror("oss: Error: shmat failed to attach shared memory");
        //if(shmctl(sharedMemSegment, IPC_RMID, NULL) == -1)
          //  perror("exe: Error: shmctl failed to remove the shared memory segment");
        exit(EXIT_FAILURE);
    }

    /* FOR INTIAL TESTING: Use the shared memory or read it 
    if(argc >= 2) 
        strncpy(sharedMemAttach, argv[1], sizeof(int));
    printf("The segment has the following: %s\n", sharedMemAttach); */

    //Initialize the seconds and nanoSeconds to 0     
    smPtr-> nanoSeconds = 0;
    smPtr-> seconds = 0;    

    //Initialize the shared memory array to all 0s
    int i;
    for(i = 0; i < maxChildren; i++)
    {
        smPtr-> childProcArr[i] = 0;
    }    
   
    //Function that lauches the children and writes to the file
    launchChildren(maxChildren, childLimit, startOfSeq, incrementVal, outFile);
    
    //Close the outfile
    fclose(OUTFILE);

    //Detach and remove the segment of shared memory
    sharedMemDetach = deallocateMem(sharedMemSegment, (void *) smPtr);
    //If shmdt is unsuccessful it returns -1 so check for this
    if(sharedMemDetach == -1)
    {
        perror("oss: Error: shmdt failed to detach shared memory");
        exit(EXIT_FAILURE);
    }
}

void launchChildren(int maxChildren, int childLimit, int startOfSeq, int incrementVal, char *outFile)
{
    pid_t pid;
    pid_t waitingID;
    int incrementClock; 
    int childrenInSystem = 0;
    int completedChildren = 0;
    int childCounter = 0;
    int childExec;
    int status;
    int i;
    int initNotPrimeNum = 0;
    int initPrimeNum = 0;
    int initOutOfTime = 0; 
    int initChildID = 0;
    int notPrimeNum[maxChildren];
    int primeNum[maxChildren];
    int outOfTime[maxChildren];  
    char childID[maxChildren];
    char stringArray[20];

    /* Fill the numbers to check array according to the 
       starting (-b) and increment (-i) values */
    int numsToCheck[maxChildren];
    numsToCheck[0] = startOfSeq;
    for(i = 1; i < maxChildren; i++)
    {
        numsToCheck[i] = numsToCheck[i - 1] + incrementVal;
    }
    
    //Open the outfile
    OUTFILE = fopen(outFile, "a");

    //Loop through until the completed children equals the max children specified by -n x
    while(maxChildren > completedChildren)
    {   
        //Increment the time of by 10000
        timeIncrementation(); 
        
        /* If the the number of children is less than the max number 
           (specified by -n) it should run and the children 
           running is less than the limit of children that should 
           run at once (specified by -s) */
        if(childCounter < maxChildren && childrenInSystem < childLimit)
        {    

            /* Increment the number of children currently running - 
               this will be decremented after waiting on the child to finish */
            childrenInSystem++;
            initChildID++;
            sprintf(childID, "%d", initChildID);
           
            //exec needs the array as a character string 
            sprintf(stringArray, "%d", numsToCheck[childCounter]);
            
            //Fork then print the starting time
            pid = fork(); 
            fprintf(OUTFILE, "Child ID: %d lauched.    | Time: %d seconds, %u nanoseconds\n", pid, smPtr-> seconds, smPtr-> nanoSeconds);
            
            //Fork returns -1 if it fails
            if(pid == -1)
            {
                perror("oss: Error: Child process fork failed");
                exit(EXIT_FAILURE);
            }
            //Fork returns 0 to the child if successful
            else if(pid == 0) /* Child pid */
            { 
                //Use execvp to run the oss executable
                char *args[] = {"./prime", stringArray, childID, NULL};
                childExec = execvp(args[0], args);

                //If the exec fails it returns -1 so perror and exit
                if(childExec == -1)
                {
                    perror("oss: Error: Child failed to exec ls\n");
                    exit(EXIT_FAILURE);
                }
                exit(EXIT_SUCCESS);                       
            }
            childCounter++; //Increment the number of children      
        }
     
        //Wait for the child to finish
        waitingID = waitpid(-1, &status, WNOHANG);      
        
        if(waitingID > 0)
        { 
            fprintf(OUTFILE, "Child ID: %d terminated. | Time: %d seconds, %u nanoseconds\n", waitingID, smPtr-> seconds, smPtr-> nanoSeconds); 
     
            completedChildren++; //A child has completed
            childrenInSystem--; //There is one less child currently running, so we will be able to launch another now
        }
    }

    //Output the completion time to the output file when a children finish
    fprintf(OUTFILE, "The program completed at: %d seconds %u nanoseconds\n", smPtr-> seconds, smPtr-> nanoSeconds);

    //Loop through the shared memory array and add the values to the correct array to be printed at end
    for(i = 1; i < maxChildren + 1; i++)
    {
        //If the value in the sharedmemory array is positive it is prime
        if(smPtr-> childProcArr[i] > 0)
        { 
            //fprintf(OUTFILE, "%d is prime\n", childProcArr[i]);
            primeNum[initPrimeNum] = smPtr-> childProcArr[i];
            initPrimeNum++;
        }
        //If the value in the sharedmemory array is negative it is not prime
        else if(smPtr-> childProcArr[i] < -1)
        {
            //Change the negative value to positive then store in the not prime array so it prints positive
            int posVal = ((smPtr-> childProcArr[i]) * -1);
            //fprintf(OUTFILE, "%d is not prime\n", posVal);
            notPrimeNum[initNotPrimeNum] = posVal;
            initNotPrimeNum++;
        }
        //If the value is -1 then it didn't have time for the determination
    }

    //Loop through the array that stored the numbers that took too long to determine  
    for(i = 1; i < maxChildren + 1; i++)
    {
        if(smPtr-> tooMuchTime[i] != 0)
        {
            outOfTime[initOutOfTime] = smPtr-> tooMuchTime[i];
            initOutOfTime++;
        }
    }

    //Print the prime array numbers to the output file 
    fprintf(OUTFILE, "The prime numbers are:");
    for(i = 0; i < initPrimeNum; i++)
    {
        fprintf(OUTFILE, " %d", primeNum[i]);
    }
    //Print the not prime arrays numbers to the outfile
    fprintf(OUTFILE, "\nThe numbers that are not prime are:");
    for(i = 0; i < initNotPrimeNum; i++)
    {
        fprintf(OUTFILE, " %d", notPrimeNum[i]);
    }
    //Print the out of time array number to the outfile
    fprintf(OUTFILE, "\nThe numbers that didn't have enough time to make determination:");
    for(i = 0; i < initOutOfTime; i++)
    {
        fprintf(OUTFILE, " %d", outOfTime[i]);
    }
}

int deallocateMem(int shmid, void *shmaddr) 
{
    //If detaching fails it will return -1 so return -1 for deallocation call
    if(shmdt(shmaddr) == -1)
        return -1;
    shmctl(shmid, IPC_RMID, NULL);
    return 0;
}

/* Increment the time by 10000 at the start of each loop, if we reach a second then add a second */
void timeIncrementation()
{
    smPtr-> nanoSeconds = smPtr-> nanoSeconds + 10000;
    if(smPtr-> nanoSeconds % 1000000000 == 0)
    {
        smPtr-> seconds = smPtr-> seconds + 1;
        smPtr-> nanoSeconds = 0;
    }
}

/* Signal handler, that looks to see if the signal is for 2 seconds being up or ctrl-c being entered.
   In both cases, I connect to shared memory so that I can write the time that it is killed to the file
   and so that I can disconnect and remove the shared memory. */
void sigHandler(int sig)
{
    if(sig == SIGALRM)
    {
        key_t key = ftok(".",'m');
        int sharedMemSegment;
        sharedMemSegment = shmget(key, sizeof(struct sharedMemory), IPC_CREAT | 0644);
        smPtr = (struct sharedMemory *)shmat(sharedMemSegment, NULL, 0);
        printf("Two Seconds is up.\n");
        fprintf(OUTFILE, "Killed at %d seconds and %u nanoseconds\n", smPtr-> seconds, smPtr-> nanoSeconds);
        fclose(OUTFILE);
        shmctl(sharedMemSegment, IPC_RMID, NULL);
        kill(0, SIGKILL);
        exit(EXIT_SUCCESS);
    }
    
    if(sig == SIGINT)
    {
        key_t key = ftok(".",'m');
        int sharedMemSegment;
        sharedMemSegment = shmget(key, sizeof(struct sharedMemory), IPC_CREAT | 0644);
        smPtr = (struct sharedMemory *)shmat(sharedMemSegment, NULL, 0);
        printf("Ctrl-c was entered\n");
        fprintf(OUTFILE, "Killed at %d seconds and %u nanoseconds\n", smPtr-> seconds, smPtr-> nanoSeconds);
        fclose(OUTFILE);
        shmctl(sharedMemSegment, IPC_RMID, NULL);
        kill(0, SIGKILL);
        exit(EXIT_SUCCESS);
    }
}

/* For the -h option that can be entered */
void displayHelpMessage() 
{
    printf("\n---------------------------------------------------------\n");
    printf("See below for the options:\n\n");
    printf("-h    : Instructions for running the project and terminate.\n");
    printf("-n x  : Maximum number of child processes oss will ever create (Default 4).\n"); 
    printf("-s x  : Number of children allowed to exist in the system at same time (Default: 2).\n");
    printf("-b B  : Start of the sequence of numbers to be tested for primality (Default: 101).\n");
    printf("-i I  : Increment between numbers that we test (Default: 4).\n");
    printf("-o filename  : Output file (Default: output.dat).\n");
    printf("\n---------------------------------------------------------\n");
    printf("Examples of how to run program(default and with options):\n\n");
    printf("$ make\n");
    printf("$ ./oss\n");
    printf("$ ./oss -n 10 -s 2 -b 101 -i 4 -o output.dat\n");
    printf("$ make clean\n");
    printf("\n---------------------------------------------------------\n"); 
    exit(0);
}
