/* Author: Chase Richards
   Project: Homework 1 CS4760
   Date: January 30, 2020  
   Filename: main.c  */

#include "SearchFileSystem.h"
#include "DirectoryCheck.h"
#include "CommandOptions.h"
#include "Queue.h"

int main(int argc, char* argv[]) 
{
 
    int i;
    char* dirname = NULL;
   
    //If there are option(s) and directory or just a directory (ex: bt -p -i testDir)
    for(i = optind; i < argc; i++) 
    { 
        if(argv[i] != NULL)
            dirname = argv[i];        
    }
    
    //If there is nothing passed in (ex: bt)
    if(dirname == NULL) 
    { 
        dirname = getCWD(".", argv);
    }

    //Need to make sure the directory passed in is a directory (ex: bt -p)
    if(!(isDirectory(dirname)))
        dirname = getCWD(".", argv);

    //printf("%i\n", isdirectory(argv[1]));
    
    //Call function with getopt switch
    flgsPassedIn(argc, argv);
    
    //Print information on the options available and exit
    if(helpMessageFlg) 
    {
        displayHelpMessage();
        exit(1);
    }
    
    //Pass the directory whether specified or current default to searchFileSystem (in GetCurrentDirectory.c)
    breadthFirstTraversal(dirname, argv);
   
    return 0;
}
