/* Author: Chase Richards
   Project: Homework 1 CS4760
   Date: January 29, 2020  
   Filename: SearchFileSystem.c  */

#include "SearchFileSystem.h"
#include "CommandOptions.h"
#include "DirectoryCheck.h"
#include "Queue.h"

void breadthFirstTraversal(char *path, char **argv) 
{
    char buf[512];
    char *tempBuf;
    char *dqPath;
    struct dirent *direntp;
    DIR *dirp;       

    //Create the queue
    struct Queue *queuePtr = createQueue();

    //This makes sure the directory and stats for the one passed in is included in output
    lstat(path, &typeStats);
    printOptions(path, argv);
    printf("%s\n", path); 
    
    //Enqueue the orginal directory that is passed in
    enqueue(queuePtr, path);

    //While the queue is not empty
    while(!emptyQueue(queuePtr)) 
    {  
        //Dequeue first in and assign it to dequeued path
        dqPath = dequeue(queuePtr);
        //Return if the directory does not open
        if(!(dirp = opendir(dqPath))) 
        {
            perror("bt: Error: Unable to open directory");
            return; 
        }
    
        //Read the files and directories in the current directory        
        while((direntp = readdir(dirp)) != NULL) 
        {
            //Don't process current or parent
            if(strcmp(direntp-> d_name, ".") != 0 && strcmp(direntp-> d_name, "..") != 0 && strcmp(direntp-> d_name, ".git") != 0) 
            {              
                /*Store the formatted (next/direntp-> d_name) c string in the buffer 
                  pointed to by buf, sizeof(buf) is the max size to fill */ 
                snprintf(buf, sizeof(buf), "%s/%s", dqPath, direntp-> d_name); 
                lstat(buf, &typeStats);                               
 
                //If -L is specified and it is a symbolic link
                if((symbolicLinkFlg) && (typeStats.st_mode & S_IFMT) == S_IFLNK) 
                {
                    char tempPath[256];
                    char* linkPath = buf;
                    //Read the value of the symbolic link
                    int symValue = readlink(linkPath, tempPath, sizeof(tempPath));
                    if(symValue == -1)
                    {
                        fprintf(stderr, "%s: Error: Failed to read symbolic link", argv[0]);
                        perror("");
                    }
                    else
                    {
                        //Null terminate the file that the link leads to
                        tempPath[symValue] = '\0';
                        //Check the options passed in
                        printOptions(buf, argv);
                        //Print the path followed by -> symlinkfile/dir
                        printf("%s", buf);
                        printf(" -> %s\n", tempPath);
                    }
                } 
                //If it is not a symbolic link or -L is not passed in
                else
                {
                    //Pass buf to the options to see which were specified
                    printOptions(buf, argv);
                    //Print the path to the dir, file, symlink, etc
                    printf("%s\n", buf);

                    //If buf is a directory we need to enqueue it
                    if(isDirectory(buf)) 
                    {
                        /*tempBuf is a ptr to a newly allocated string which 
                          is a duplicate of the string pointed to by buf  */ 
                        tempBuf = strdup(buf);
                        //Enqueue the tempBuf
                        enqueue(queuePtr, tempBuf);
                    }   
                }  
            }
            else
                continue;            
        }
    }

    //Pass queuePtr to free to avoid memory leak
    free(queuePtr);     
}
