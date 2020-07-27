/* Author: Chase Richards
   Project: Homework 1 CS4760
   Date: February 7, 2020  
   Filename: Queue.h  */

#ifndef QUEUE_H
#define QUEUE_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#define QBUF 256

struct Queue 
{
    char *directory[QBUF];
    int head, tail;    
};

struct Queue *createQueue();
void enqueue(struct Queue *queuePtr, char *path);
char* dequeue(struct Queue *queuePtr);
int emptyQueue(struct Queue *queuePtr);

#endif
