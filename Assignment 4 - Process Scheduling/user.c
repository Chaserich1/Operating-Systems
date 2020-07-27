/*  Author: Chase Richards
    Project: Homework 4 CS4760
    Date March 18, 2020
    Filename: user.c  */

#include "oss.h"

int main(int argc, char *argv[])
{
    int procPid;
    msg message;
    int receiveMessage, sendMessage;
    int quantum;
    int status = 0;
    procPid = atoi(argv[1]);
    msgqSegment = atoi(argv[2]);
    quantum = atoi(argv[3]);
    srand(time(0) + (procPid + 1));

    pcbt *pcbtPtr;
    clksim *clockPtr;
    clksim timeBlocked;
    clksim event;
    clksim procBlkedTime;
    int burstTime;
    
    /* Get and attach to the process control block table shared memory */
    pcbtSegment = shmget(pcbtKey, sizeof(pcbt) * (procPid + 1), IPC_CREAT | 0666);
    if(pcbtSegment < 0)
    {
        perror("user: Error: Failed to get pcb table segment (shmget)");
        exit(EXIT_FAILURE);
    }
    pcbtPtr = shmat(pcbtSegment, NULL, 0);
    if(pcbtPtr < 0)
    {
        perror("user: Error: Failed to attach pcb table (shmat)");
        exit(EXIT_FAILURE);
    }
    
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

    /* While it is not to be terminated */
    while (status != 1)
    {
        /* Receive the message and check for failure */
        receiveMessage = msgrcv(msgqSegment, &message, sizeof(message.valueofMsg), (procPid + 1), 0);
        if(receiveMessage == -1)
        {
            perror("user: Error: Failed to recieve message (msgrcv)");
            exit(EXIT_FAILURE);
        }
  
        /* Determine if it is blocked or terminating or use its entire timeslice */
        int timeToTerminate = ((rand() % 100) + 1) <= 3 ? 1 : 0;
        int timeToBlock = ((rand() % 100) + 1) <= 3 ? 1 : 0;
        if(timeToTerminate)
            status = 1;
        else if(timeToBlock)
            status = 2;
        else
            status = 0;

        /* Set valueofMsg based on whether it is to be blocked terminated or use its timeslice */
        if(status == 0)
            message.valueofMsg = 100;
        //Ready to terminate
        else if(status == 1)
            message.valueofMsg = (rand() % 99) + 1;
        //Blocked process
        else if(status == 2)
        {
            //If it is blocked give it a negative random value for the message to oss
            message.valueofMsg = ((rand() % 99) + 1) * -1;
            //Assign the time it is being blocked
            timeBlocked.nanosec = clockPtr-> nanosec; 
            timeBlocked.sec = clockPtr-> sec;
            //Value for the amount of quantum to be used depending on the queue and mvalue
            burstTime = message.valueofMsg * (quantum / 100) / (pcbtPtr[procPid].priority + 1);
            //Random values for the event r.s random numbers
            event.nanosec = (rand() % 1000) * 1000000;
            event.sec = (rand() % 2) + 1;
            //The event that the time is to happen is added to waitingTime for the process
            pcbtPtr[procPid].waitingTime = addTime(pcbtPtr[procPid].waitingTime, event);
            //Determine when the event occurs based on the clock
            event = addTime(event, timeBlocked);
            clockIncrementor(&event, (burstTime * -1));
            //pcbtPtr[procPid].blockedTime = addTime(pcbtPtr[procPid].blockedTime, event); 
            pcbtPtr[procPid].readyToGo = 0;
        }

        clksim startblked;
        startblked.nanosec = 10000000;
        startblked.sec = 1;

        message.typeofMsg = procPid + 100;
        
        /* Send the message back and check for failure */
        sendMessage = msgsnd(msgqSegment, &message, sizeof(message.valueofMsg), 0);
        if(sendMessage == -1)
        {
            perror("user: Error: Failed to send message (msgsnd)");
            exit(EXIT_FAILURE);
        }

        /* If it is blocked then while it's not ready check the clock
           and once the event is to happen, then unblock the process */
        if(status == 2)
        {
            while(pcbtPtr[procPid].readyToGo == 0)
            {
                //clockIncrementor(&startblked, 1000000);
                if(event.sec > clockPtr-> sec)
                {
                    pcbtPtr[procPid].readyToGo = 1;
                    //Get the time blocked for the pid 
                    pcbtPtr[procPid].blkedTime = addTime(pcbtPtr[procPid].blkedTime, startblked);
                }
                else if(event.nanosec >= clockPtr-> nanosec && event.sec >= clockPtr-> sec)
                {
                    pcbtPtr[procPid].readyToGo = 1;
                    //Get the time blocked for the pid
                    pcbtPtr[procPid].blkedTime = addTime(pcbtPtr[procPid].blkedTime, startblked);
                }
            }
        } 

    }

    return 0;
}

