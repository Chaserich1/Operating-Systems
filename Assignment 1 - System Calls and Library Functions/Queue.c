/* Author: Chase Richards
   Project: Homework 1 CS4760
   Date: February 7, 2020  
   Filename: Queue.c  */

#include "Queue.h"

//Create the queue and dymically allocate size
struct Queue *createQueue() 
{
    //Allocate the size
    struct Queue *queuePtr = malloc(sizeof(struct Queue));
    //Initialize the head and tail
    queuePtr-> head = -1; 
    queuePtr-> tail = -1;
    return queuePtr; //Return the queue ptr
}

//Enqueue function to add a directory to the queue
void enqueue(struct Queue *queuePtr, char *path) 
{
    //If the tail is 255 then the queue is full
    if(queuePtr-> tail == QBUF - 1)
        perror("bt: Error: The queue is full");
    else 
    {
        //For the first directory passed in
        if(queuePtr-> head == -1)
            queuePtr-> head = 0;
        //Increment the tail when a new directory comes in
        queuePtr-> tail++;
        queuePtr-> directory[queuePtr-> tail] = path;
    }
} 

//Dequeue function to pull the first in, first out
char* dequeue(struct Queue *queuePtr) 
{
    char *tempPtr;
    //If tail is -1 then return null
    if(!(queuePtr-> tail))
        tempPtr = NULL;
    //Return the directory at the head of the queue (first in)
    tempPtr = queuePtr-> directory[queuePtr-> head];
    //Head is the next directory
    queuePtr-> head++;
    //If head is greater than tail reset the queue
    if(queuePtr-> head > queuePtr-> tail) 
        queuePtr-> head = queuePtr-> tail = -1;
       
    return tempPtr;
}

//Return true if the queue is empty, otherwise false
int emptyQueue(struct Queue *queuePtr) 
{
    if(queuePtr-> tail == -1)
        return true;
    else
        return false;
}

