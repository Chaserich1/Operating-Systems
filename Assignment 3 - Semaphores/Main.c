/* Author: Chase Richards
   Project: Homework 3 CS4760
   Date March 3, 2020
   Filename: Main.c  */

#include "Main.h"

int main(int argc, char* argv[])
{
    int c;
    char *inFile = "input.dat"; //Default inputfile name
    int n = 64; //Max Children in system at once
    int totalInts = 64; //Default 64 integers in input the file
    int t = 100; //Default alarm(100)

    while((c = getopt(argc, argv, "hf:n:t:")) != -1)
    {
        switch(c)
        {
            case 'h':
                displayHelpMessage();
                return (EXIT_SUCCESS);
            case 'f':
                inFile = optarg;
                break;
            case 'n':
                n = atoi(optarg);
                break;
            case 't':
                t = atoi(optarg);
                break;
            default:
                printf("Using default values");
                break;               
        }
    }
  
    /* Signal for terminating, freeing up shared mem, killing all children 
       if the program goes for more than two seconds of real clock */
    signal(SIGALRM, sigHandler);
    alarm(t);

    /* Signal for terminating, freeing up shared mem, killing all 
       children if the user enters ctrl-c */
    signal(SIGINT, sigHandler);  

    sharedMemoryWork(totalInts, n, inFile);

}

void sharedMemoryWork(int totalInts, int n, char *inFile)
{
    key_t key;
    int sharedMemSegment, sharedMemDetach, sharedMemDetach1;
    int totalInts1;

    //Key returns a key based on the path and id
    key = ftok(".",'a');
    if(key == -1)
    {
        perror("master: Error: getting the shared memory key struct int array");
        exit(EXIT_FAILURE);
    }

    //Allocate the shared memory using shmget
    sharedMemSegment = shmget(key, sizeof(struct sharedMemory), IPC_CREAT | 0666);
    
    //If shmget is unsuccessful it retruns -1 so check for this
    if(sharedMemSegment == -1) 
    {
        perror("master: Error: shmget failed to allocate shared memory struct int array");
        exit(EXIT_FAILURE);
    }

    //Attach to shared memory segment
    smPtr = (struct sharedMemory *)shmat(sharedMemSegment, (void *)0, 0);
    
    //If shmat returns -1 then it failed to attach
    if(smPtr == (void *)-1)
    {
        perror("master: Error: Failed to attach shared memory for struct int array");
        exit(EXIT_FAILURE);
    }
    
    //ftok the key
    key_t key1 = ftok(".", 'b');
    //If key returns -1 then it failed so check
    if(key1 == -1)
    {
        perror("bin_adder: Error: Failed to get shared memory key calc array");
        exit(EXIT_FAILURE);
    }
    //Get the segment for the array
    int seg = shmget(key1, sizeof(int), IPC_CREAT | 0777);
    //If shmget returns -1 it failed
    if(seg == -1)
    {
        perror("bin_adder: Error: Failed to get shared memory segment calc array");
        exit(EXIT_FAILURE);
    }

    //Attach to shared memory and check if it fails
    int *calculationFlg = (int*)shmat(seg, (void*)0, 0);
    if(calculationFlg == (void *) -1)
    {
        perror("bin_adder: Error: Failed to attach to shared memory calc array");
        exit(EXIT_FAILURE);
    }   

    //Clock variable declarations
    int milliSecCalc1 = 0, milliSecCalc2 = 0;
    clock_t startCalc1, startCalc2, totalCalc1, totalCalc2;
 
    //Open the input file using the function that checks for failure
    INFILE = openFile(inFile, "a", n); 
    //FILE* filePtr = fopen("adder_log","a");    

    /*Write the headers to the file, read the input file to 
      shared memory, fork exec the children for the calculations*/
    calculationFlg[0] = 0;
    int calcFlg = calculationFlg[0];
    if(calculationFlg[0] == 0)
    {
        writeLogHeaders(calcFlg); //Write the headers to adder_log
        totalInts = readFileOne(inFile, sharedMemSegment, n); //Get the total numbers of ints in input file
        startCalc1 = clock(); //get the start time of the clock
        int newCalcFlg = calculationOne(totalInts, calcFlg); //set the calculation flag and do calc n/2
        totalCalc1 = clock() - startCalc1; //Get the end time and subtract the starting clock time for total
        milliSecCalc1 = totalCalc1 * 1000 / CLOCKS_PER_SEC;
        //Open adder_log and write the total time that calculation n/2 took
        FILE* filePtr = fopen("adder_log", "a");
        fprintf(filePtr, "\nTotal Time for n/2 Calculations: %d seconds %d milliseconds\n", milliSecCalc1/1000, milliSecCalc1%1000);
        fclose(filePtr);
        calculationFlg[0] = newCalcFlg; //Set the flag to 1 so we go into calculation 2
    }
    //n/logn calculation
    if(calculationFlg[0] == 1)
    {
        int newCalcFlg = 1;
        writeLogHeaders(newCalcFlg); //Write the headers
        totalInts = readFileTwo(inFile, sharedMemSegment, n); //Get the total numbers of integers in the file
        startCalc2 = clock(); //Get the start time of the clock
        calculationTwo(totalInts, newCalcFlg); //Do nlogn calculation
        totalCalc2 = clock() - startCalc2; //Subtract start time from end time for total
        milliSecCalc2 = totalCalc2 * 1000 / CLOCKS_PER_SEC;
        //Open the file and write the total time that calculation nlogn took
        FILE* filePtr = fopen("adder_log", "a");
        fprintf(filePtr, "\nTotal Time for n/2 Calculations: %d seconds %d milliseconds\n", milliSecCalc2/1000, milliSecCalc2%1000); 
        fclose(filePtr);
    }

    //Detach and Remove Shared Memory
    sharedMemDetach = deallocateMem(sharedMemSegment, (void*) smPtr);     
    sharedMemDetach1 = deallocateMem(seg, (void*) calculationFlg);   
    //Check for shared memory detach to return -1
    if(sharedMemDetach == -1 || sharedMemDetach1 == -1)
    {
        perror("master: Error: shared memory detach and removal failed");
        exit(EXIT_FAILURE);
    }
}

// n/2 calculation
int calculationOne(int totalInts, int calcFlg)
{
    printf("\nn/2 pairs calculation:\n");
    int completedChildren = 0; //Completed child processes
    int runningChildren = 0; //Children in system
    int childCounter = 0;
    int index = 0;
    int numIntsToAdd = 2;
    pid_t pid, waitingID;
    int status;
    int childExec; //For checking exec failure
    char xx[20]; //Convert index to char string for exec
    char yy[20];
    //Semaphore Declarations
    sem_t* sem;
    char *semaphoreName = "semChild";

    //Create the semaphore with starting value of 1 and check for success
    sem = sem_open(semaphoreName, O_CREAT, 0644, 1);
    if(sem == SEM_FAILED)
    {
        perror("master: Error: Unable to open n/2 semaphore");
        exit(EXIT_FAILURE);
    } 
    
    //if the number of integers to add is greater than the total and there are no more running children 
    while(calcFlg == 0)
    {
        //If there are less than 20 children, index & children & finished children are less than the total ints in the file
        if(runningChildren < 20 && childCounter < totalInts && index < totalInts)
        {
            //Fork
            pid = fork();
            //Check for fork failure if it returns -1
            if(pid == -1)
            {
                perror("master: Error: Failed to fork");
                exit(EXIT_FAILURE);
            }
            //Fork returns 0 if it is successful
            else if(pid == 0)
            {
                //Pass the index and the number of ints to add as a string through exec
                sprintf(xx, "%d", index);
                sprintf(yy, "%d", numIntsToAdd);
                char *args[] = {"./bin_adder", xx, yy, NULL};
                childExec = execvp(args[0], args);

                //If exec fails it returns -1 so perror and exit
                if(childExec == -1)
                {
                    perror("master: Error: Child failed to exec");
                    exit(EXIT_FAILURE);
                } 
                                                 
            }
            runningChildren++; //Add to number of running children
            childCounter++; //We've done another child
            index += numIntsToAdd; //index is index plus the number of ints to add
            //printf("Index: %d\n", index);
        }
        
        //Wait for the child to finish and make sure it's greater than 0
        waitingID = waitpid(-1, &status, WNOHANG);

        if(waitingID > 0)
        {
            completedChildren++; //A child has completed
            runningChildren--; //One less child in the system
            
            //if the index is greater than or equal to the total ints in the file
            if(index >= totalInts)
            {
                /*if the number of integers we execed to add is more than total
                  ints in file and active children is 0 then we are done and can
                  set the calculation flg to 1 to move onto nlog(n) */
                if(numIntsToAdd >= totalInts && runningChildren == 0)
                {    
                    calcFlg = 1;
                }
                /*if there are no more active children then set index
                  to zero and numbers to add doubles */
                if(runningChildren == 0)
                {
                    index = 0;
                    numIntsToAdd *= 2;
                }
            }
        }
    }
    //Destroy the semaphore
    sem_unlink(semaphoreName);
    //Return the calculation flg so the program moves onto n/logn
    return calcFlg;
}

// nlog(n) calculation
int calculationTwo(int totalInts, int newCalcFlg)
{
    printf("\nn/logn groups calculation:\n");
    int completedChildren = 0; //Completed child processes
    int runningChildren = 0; //Children in system
    int childCounter = 0;
    int index = 0;
    int numIntsToAdd = ceil(log2(totalInts)); //Number of ints per group
    int temp = totalInts/(ceil(log(totalInts)));
    pid_t pid, waitingID;
    int status;
    int childExec; //For checking exec failure
    char xx[20]; //Convert index to char string for exec
    char yy[20];
    //Semaphore Declarations
    sem_t* sem1;
    char *semaphoreName1 = "semLogChild";

    //Create the semaphore with starting value of 1 and check for success
    sem1 = sem_open(semaphoreName1, O_CREAT, 0744, 1);
    if(sem1 == SEM_FAILED)
    {
        perror("master: Error: Unable to open nlogn semaphore");
        exit(EXIT_FAILURE);
    } 
       
    //if the number of integers to add is greater than the total and there are no more running children 
    while(newCalcFlg == 1)
    {
        /*If there are less than 20 children, index & children 
          & finished children are less than the total ints in the file*/
        if(runningChildren < 20 && childCounter < totalInts && index < totalInts)
        {
            //Fork
            pid = fork();
            //Check for fork failure if it returns -1
            if(pid == -1)
            {
                perror("master: Error: Failed to fork");
                exit(EXIT_FAILURE);
            }
            //Fork returns 0 if it is successful
            else if(pid == 0)
            {
                /*If there is a remainder then pass the remaining integers
                  in. So for example if it is 64 there are 10 groups of 6
                  and one remaining group of 4 integers */
                int logIntsToAdd;
                if((index + numIntsToAdd) > totalInts)
                    logIntsToAdd = totalInts - index;
                else
                    logIntsToAdd = numIntsToAdd;
                //Pass the index and the number of ints to add as a string through exec
                sprintf(xx, "%d", index);
                sprintf(yy, "%d", logIntsToAdd);
                char *args[] = {"./bin_adder", xx, yy, NULL};
                childExec = execvp(args[0], args);

                //If exec fails it returns -1 so perror and exit
                if(childExec == -1)
                {
                    perror("master: Error: Child failed to exec");
                    exit(EXIT_FAILURE);
                } 
                                                 
            }
            runningChildren++; //Add to number of running children
            childCounter++; //We've done another child
            index += numIntsToAdd; //index is index plus the number of ints to add
        }
        
        //Wait for the child to finish and make sure it's greater than 0
        waitingID = waitpid(-1, &status, WNOHANG);

        if(waitingID > 0)
        {
            completedChildren++; //A child has completed
            runningChildren--; //One less child in the system
            
            //if the index is greater than or equal to the total ints in the file
            if(index >= totalInts)
            {
                /*if the number of integers we execed to add is more than total
                  ints in file and active children is 0 then we are done and ca
                  set the calculation flg to 1 to move onto nlog(n) */
                if(numIntsToAdd >= totalInts && runningChildren == 0)
                {    
                    newCalcFlg = 2;
                }
                /*if there are no more active children then set index
                  to zero and numbers to add doubles */
                if(runningChildren == 0)
                {
                    index = 0;
                    numIntsToAdd *= 2;
                }
            }
        }
    }
    //Destroy the semaphore
    sem_unlink(semaphoreName1);
    return newCalcFlg;
}    
    
//Open the input file and generate the random values
FILE* openFile(char *fileName, char *mode, int n)
{
    FILE* filePtr = fopen(fileName, "w");
    if(filePtr == NULL)
    {
        perror("master: Error: Failed to open input file");
        exit(EXIT_FAILURE);
    }
   
    //Generate the random values for the input file between 0-256
    srand(time(0));
    int i;
    for(i = 0; i < n; i++)
    {
        int randValue;
        randValue = (rand() % 256);
        fprintf(filePtr, "%d\n", randValue);
    }
    fclose(filePtr);
 
    return filePtr;
}

/*Read the integer values from the file and save to shared 
  memory array, also returns total num of ints in file */
int readFileOne(char *fileName, int shmid, int n)
{
    //Open the file and check for failure
    FILE* filePtr = fopen(fileName, "r");
    if(filePtr == NULL)
    {
        perror("master: Error: Failed to open input file");
        exit(EXIT_FAILURE);
    }

    int count = 0;
    //Array for the file integers attached to shared memory
    int *integersOne;
    integersOne = shmat(shmid, NULL, 0);
    //Check if it returns -1
    if(integersOne < 0)
    {
        perror("master: Error: Failed to attach to sharedmemory");
        exit(EXIT_FAILURE);
    } 

    //Loop through the file and read the integers to the intergers array
    int i; 
    for(i = 0; i < n; i++)
    {
        fscanf(filePtr, "%d", &smPtr-> integersOne[i]);
        count++;
    } 

    fclose(filePtr);
    //Detach from shared memory
    shmdt((void*) integersOne);
    return count;
}

/* Read the integer values from the file and save to 
   shared memory array, also returns total num of ints in file */
int readFileTwo(char *fileName, int shmid, int n)
{
    //Open the file and check for failure
    FILE* filePtr = fopen(fileName, "r");
    if(filePtr == NULL)
    {
        perror("master: Error: Failed to open input file");
        exit(EXIT_FAILURE);
    }

    int count = 0;
    //Array for the file integers attached to shared memory
    int *integersTwo;
    integersTwo = shmat(shmid, NULL, 0);
    //Check if it returns -1
    if(integersTwo < 0)
    {
        perror("master: Error: Failed to attach to sharedmemory");
        exit(EXIT_FAILURE);
    } 

    //Loop through the file and read the integers to the intergers array
    int i; 
    for(i = 0; i < n; i++)
    {
        fscanf(filePtr, "%d", &smPtr-> integersTwo[i]);
        //printf("%d\n", smPtr-> integers[i]);
        count++;
    } 

    fclose(filePtr);
    //Detach from shared memory
    shmdt((void*) integersTwo);
    return count;
}

//This simply write the headers to adder_log
void writeLogHeaders(calcFlg)
{
    FILE* logFile = fopen("adder_log", "a");
    if(logFile == NULL)
    {
        perror("master: Error: failed to open adder_log file");
        exit(EXIT_FAILURE);
    }

    if(calcFlg == 0)
         fprintf(logFile, " n/2 pairs");
    else
         fprintf(logFile, "\n\n n/logn groups");
     
    fprintf(logFile, "\n----------------------------------------------------------------------------------\n");
    fprintf(logFile, "\tPID\t\tIndex\t\tSize\t\tResult\t\tTime\n");
    fclose(logFile); 
} 

//Shared Memory Deallocation and removal - checks for failure in sharedmemwork
int deallocateMem(int shmid, void *shmaddr) 
{
    //Detach and remove shared memory
    shmdt(shmaddr);
    shmctl(shmid, IPC_RMID, NULL);
    return 0;
}

/* Signal handler, that looks to see if the signal is for 2 seconds being up or ctrl-c being entered.
   In both cases, I connect to shared memory so that I can write the time that it is killed to the file
   and so that I can disconnect and remove the shared memory. */
void sigHandler(int sig)
{
    if(sig == SIGALRM)
    {
        key_t key = ftok(".",'a');
        int sharedMemSegment;
        sharedMemSegment = shmget(key, sizeof(struct sharedMemory), IPC_CREAT | 0666);
        key_t key1 = ftok(".", 'b');
        int seg;
        seg = shmget(key1, sizeof(int), IPC_CREAT | 0777);
        printf("Timer is up.\n"); 
        printf("Killing children, removing shared memory and unlinking semaphore.\n");
        shmctl(sharedMemSegment, IPC_RMID, NULL);
        shmctl(seg, IPC_RMID, NULL);
        sem_unlink("semChild");
        sem_unlink("semLogChild");
        kill(0, SIGKILL);
        exit(EXIT_SUCCESS);
    }
    
    if(sig == SIGINT)
    {
        key_t key = ftok(".",'a');
        int sharedMemSegment;
        sharedMemSegment = shmget(key, sizeof(struct sharedMemory), IPC_CREAT | 0666);
        key_t key1 = ftok(".", 'b');
        int seg;
        seg = shmget(key1, sizeof(int), IPC_CREAT | 0777);
        printf("Ctrl-c was entered\n");
        printf("Killing children, removing shared memory and unlinking semaphore\n");
        shmctl(sharedMemSegment, IPC_RMID, NULL);
        shmctl(seg, IPC_RMID, NULL);
        sem_unlink("semChild");
        sem_unlink("semLogChild");
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
    printf("-f filename  : Input file (Default: input.dat).\n"); 
    printf("-n x  : Number of integers to be randomly generated in the input file (Default: 64).\n");
    printf("-t x  : Number of seconds for the program timer that goes into the alarm (Default: 100).\n");
    printf("\n---------------------------------------------------------\n");
    printf("Examples of how to run program(default and with options):\n\n");
    printf("$ make\n");
    printf("$ ./master\n");
    printf("$ ./master -f input.dat -n 32 -t 200\n");
    printf("$ make clean\n");
    printf("\n---------------------------------------------------------\n"); 
    exit(0);
}
