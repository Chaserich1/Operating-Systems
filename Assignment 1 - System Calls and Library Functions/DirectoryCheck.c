/* Author: Chase Richards
   Project: Homework 1 CS4760
   Date: January 29, 2020  
   Filename: DirectoryCheck.c  */

#include "DirectoryCheck.h"

char *getCWD(char *path, char **argv)
{	
    long maxpath;
    char *mycwdp;
    //Find the maximum pathname length
    if ((maxpath = pathconf(".", _PC_PATH_MAX)) == -1) 
    {
        fprintf(stderr, "%s: Error: Failed to determine the length of the path", argv[0]);
        perror("");
        return NULL;
    }
    //Allocate space for the pathname dynamically
    if ((mycwdp = (char *) malloc(maxpath)) == NULL) 
    {
        fprintf(stderr, "%s: Error: Failed to allocate the space for the pathname", argv[0]);
        perror("");
        return NULL;
    }
    //If it fails, return NULL
    if (getcwd(mycwdp, maxpath) == NULL) 
    {
        fprintf(stderr, "%s: Error: Failed to get the current working directory", argv[0]);
        perror("");
        return NULL;
    }
    //Return the current working directory
    return mycwdp;
}

//Pass the path to determine if it's a directory
int isDirectory(char *path) 
{
    struct stat statBuf;
    //Return 0 if not a directory
    if(stat(path, &statBuf) == -1)
        return 0;
    else //Return true (nonzero) if it is a directory
        return S_ISDIR(statBuf.st_mode);
}

