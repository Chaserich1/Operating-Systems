/*  Author: Chase Richards
    Project: Homework 5 CS4760
    Date April 4, 2020
    Filename: user.c  */

#include "oss.h"

int main(int argc, char *argv[])
{
    msg message;
   
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
    
    //Semaphore declarations 
    sem_t* sem;
    char *semaphoreName = "semUser";

    //Used for determining if a process has run for at least one second
    clksim startClock;
    clksim totalClock;
    //Counting the resources
    int rCounter;
    //For the response from oss - granted or denied
    int msgFromOss;
    //Used to determine whether a process should request or release a resource
    int boundB;   
    //Will be used for messaging with oss
    int procPid, resource, process;
    //Passed the message queue segment id through execl
    msgqSegment = atoi(argv[1]);
    //Passed the generated process id through execl
    procPid = atoi(argv[2]);
    //The real processes pid
    process = getpid();
    //Resources that the process has - initially all zeros
    int procsResources[20];
    int i; //loops
    for(i = 0; i < 20; i ++)
        procsResources[i] = 0;    

    //Random in case the random value generated in oss.c is close to 0, so add the getpid() value
    srand(time(0) + process);
    //Starting clock value for the process is now
    startClock = *clockPtr;

    //Continuous loop until it's time to terminate
    while(1)
    {
        //Get the totaltime for the process and check to ensure it's run for at least a second
        totalClock = subTime((*clockPtr), startClock);
        boundB = (rand() % 100) + 1;
        //If it has run for at least 1 second and it is divisible by 1000000 it can terminate normally
        if((clockPtr-> nanosec % 1000000 == 0) && (totalClock.sec >= 1))
        {
            //Determine if the process is to terminate
            boundB = (rand() % 100) + 1;
            //If it is time to terminate (>45% chance), then send the termination message and terminate with return 0
            if(boundB >= 45)
            {
                terminateToOss(process, procPid);
                return 0;               
            }
        }
        //Random chance of requesting a resource (give it a 50% chance to request, otherwise release)
        boundB = (rand() % 100) + 1;
        if(boundB >= 50)
        {
            //Get a random resource and request it from oss
            resource = rand() % 20;
            msgFromOss = requestToOss(process, procPid, resource);
            //If oss grants the request
            if(msgFromOss == 3)
            {
                //increment the process resources
                rCounter++;
                procsResources[resource]++;
            }
            //If the reuqest is denied wait to receive a message
            else if(msgFromOss == 4)
            {
                int receivemessage;
                //Wait for the resource and check for a failure
                receivemessage = msgrcv(msgqSegment, &message, sizeof(msg), process, 0);
                if(receivemessage == -1)
                {
                    perror("user: Error: Failed to receive message (msgrcv)\n");
                    exit(EXIT_FAILURE);
                }
                //Increment the process resources
                rCounter++;
                procsResources[resource]++;
            }
        }
        /* If it was in the 50% that it releases then check if it has any resources and release if it does */
        else if(procsResources > 0)
        {
            for(i = 0; i < 20; i++)
            {
                if(procsResources[i] > 0)
                {
                    resource = i;
                    break;
                }
            }
            //Send the release message to oss and if granted then release the resource
            msgFromOss = releaseToOss(process, procPid, resource);
            //Release is granted so decrement the resource from the processes resources
            if(msgFromOss == 3)
            {
                rCounter--;
                procsResources[resource]--;
            }
        }
 
    
        //Open the semaphore and increment the shared memory clock
        sem = sem_open(semaphoreName, O_CREAT, 0700, 1);    
        if(sem == SEM_FAILED)
        {
            perror("user: Error: Failed to open semaphore\n");
            exit(EXIT_FAILURE);
        }
        //Increment clock
        clockIncrementor(clockPtr, 1000000);
        //Signal semaphore
        sem_unlink(semaphoreName);
    }
    return 0;
}

/* Message being sent to oss requesting resources */
int requestToOss(int process, int procPid, int resource)
{
    int sendmessage, receivemessage;
    /* Send the message to oss with type 1 and it's a request for the specified resource for the process */
    msg message = {.typeofMsg = 1, .msgDetails = 0, .resource = resource, .process = procPid, .processesPid = process};
    
    /* Send the message and check for failure */
    sendmessage = msgsnd(msgqSegment, &message, sizeof(msg), 0);
    if(sendmessage == -1)
    {
        perror("user: Error: Failed to send message (msgsnd)\n");
        exit(EXIT_FAILURE);
    }
    /* Receive the message from oss */
    receivemessage = msgrcv(msgqSegment, &message, sizeof(msg), process, 0);
    if(receivemessage == -1)
    {
        perror("user: Error: Failed to receive message (msgrcv)\n");
        exit(EXIT_FAILURE);
    }
    //Return whether oss is granting or denying   
    return message.msgDetails;
}

/* Message being sent to oss releasing resources */
int releaseToOss(int process, int procPid, int resource)
{
    int sendmessage, receivemessage;
    /* Send the message to oss with type 1 and it's a release for the specific resource and process */
    msg message = {.typeofMsg = 1, .msgDetails = 1, .resource = resource, .process = procPid, .processesPid = process};
    
    /* Send the message and check for failure */
    sendmessage = msgsnd(msgqSegment, &message, sizeof(msg), 0);
    if(sendmessage == -1)
    {
        perror("user: Error: Failed to send message (msgsnd)\n");
        exit(EXIT_FAILURE);
    }
    /* Receive the message from oss */
    receivemessage = msgrcv(msgqSegment, &message, sizeof(msg), process, 0);
    if(receivemessage == -1)
    {
        perror("user: Error: Failed to receive message (msgrcv)\n");
        exit(EXIT_FAILURE);
    }
    //Return whether oss is granting or denying   
    return message.msgDetails;
}

/* Message being sent to oss terminating process and it's a termination for the specific process with no resource */
void terminateToOss(int process, int procPid)
{
    int sendmessage;
    /* Send the message to oss with type 1 and it's a request */
    msg message = {.typeofMsg = 1, .msgDetails = 2, .resource = -1, .process = procPid, .processesPid = process};
    
    /* Send the message and check for failure */
    sendmessage = msgsnd(msgqSegment, &message, sizeof(msg), 0);
    if(sendmessage == -1)
    {
        perror("user: Error: Failed to send message (msgsnd)\n");
        exit(EXIT_FAILURE);
    }
        
    return;
}

