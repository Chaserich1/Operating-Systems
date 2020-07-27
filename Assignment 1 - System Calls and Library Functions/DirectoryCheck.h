/* Author: Chase Richards
   Project: Homework 1 CS4760
   Date: January 29, 2020  
   Filename: DirectoryCheck.h  */ 

#ifndef DIRECTORYCHECK_H
#define DIRECTORYCHECK_H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <sys/stat.h>

char *getCWD(char *path, char **argv);
int isDirectory(char* path);

#endif
