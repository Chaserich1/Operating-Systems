/* Author: Chase Richards
   Project: Homework 1 CS4760
   Date: January 31, 2020  
   Filename: CommandOptions.c  */

#include "CommandOptions.h"
#include "SearchFileSystem.h"

//Initialize all flags as false until they are entered
int helpMessageFlg = 0;    // -h
int symbolicLinkFlg = 0;   // -L
int fileTypeInfoFlg = 0;   // -t
int permissionFlg = 0;     // -p
int linksToFileFlg = 0;    // -i
int fileUIDFlg = 0;        // -u
int fileGIDFlg = 0;        // -g
int fileByteSizeFlg = 0;   // -s
int lastModTimeFlg = 0;    // -d
int tpiugsFlg = 0;         // -l

void flgsPassedIn(int argc, char **argv) {

    // Used getopt to go through the parameter flgs that the user passes  
    int c = 0;
    int opterr = 0;

    while((c = getopt(argc, argv, "hLdgipstul")) != -1) 
    {
        switch(c) 
        {
            case 'h':
                helpMessageFlg = 1;
                break;
            case 'L':
                symbolicLinkFlg = 1;
                break;
            case 'd':
                lastModTimeFlg = 1;
                break;
            case 'g':
                fileGIDFlg = 1;
                break;
            case 'i':
                linksToFileFlg = 1;
                break;
            case 'p':
                permissionFlg = 1;
                break;
            case 's':
                fileByteSizeFlg = 1;
                break;
            case 't':
                fileTypeInfoFlg = 1;
                break;
            case 'u':
                fileUIDFlg = 1;
                break;
            case '?':
                fprintf(stderr, "%s: Error: Invalid option, use -h to see the available options.\n", argv[0]);
                perror("");
                exit(1);
            case 'l':
                fileTypeInfoFlg = 1, permissionFlg = 1, linksToFileFlg = 1, fileUIDFlg = 1, fileGIDFlg = 1, fileByteSizeFlg = 1;
                break; 
        }
    }
    
}

void displayHelpMessage() 
{
    printf("\n---------------------------------------------------------\n");
    printf("See below for the options:\n\n");
    printf("-h   : Print help message.\n");
    printf("-L   : Follow symbolic links, if any.\n"); 
    printf("-d   : Show the time of the last modification.\n");
    printf("-g   : Print the GID associated with the file.\n");
    printf("-i   : Print the number of links to file in inode table.\n");
    printf("-p   : Print permission bits.\n");
    printf("-s   : Print the size of file in bytes.\n");
    printf("-t   : Print information on file type.\n");
    printf("-u   : Print the UID associated with the file.\n");
    printf("-l   : Print information for -t -p -i -u -g -s\n");
    printf("\n---------------------------------------------------------\n"); 
}

void printOptions(char *path, char **argv)
{  
    //-t: Print information on the file type
    if(fileTypeInfoFlg)
    {
        switch(typeStats.st_mode & S_IFMT)
        {
            case S_IFREG:
                printf("%-12s", "RegularFile");
                break;
            case S_IFDIR:
                printf("%-12s", "Directory");
                break;
            case S_IFLNK:
                printf("%-12s", "SymbolicLink");
                break;
            case S_IFCHR:
                printf("%12s", "CharSpecFile");
                break;
            case S_IFBLK:
                printf("%12s", "BlckSpecFile");
                break;
            case S_IFIFO:
                printf("%-12s", "PipeOrFIFO");
                break;
            case S_IFSOCK:
                printf("%-12s", "Socket");
                break;
            default:
                printf("%-12s", "Unknown");
                break;
        }
        printf(" ");
    }

    //-p: Print permission bits
    if(permissionFlg)
    {
        if(S_ISLNK(typeStats.st_mode)) 
        {
            printf("l");
        }
        else if(S_ISDIR(typeStats.st_mode))
        {
            printf("d");
        }
        else
        {
            printf("-");
        }
        
        printf((typeStats.st_mode & S_IRUSR) ? "r" : "-");
        printf((typeStats.st_mode & S_IWUSR) ? "w" : "-");
        printf((typeStats.st_mode & S_IXUSR) ? "x" : "-");
        printf((typeStats.st_mode & S_IRGRP) ? "r" : "-");
        printf((typeStats.st_mode & S_IWGRP) ? "w" : "-");
        printf((typeStats.st_mode & S_IXGRP) ? "x" : "-");
        printf((typeStats.st_mode & S_IROTH) ? "r" : "-");
        printf((typeStats.st_mode & S_IWOTH) ? "w" : "-");
        printf((typeStats.st_mode & S_IXOTH) ? "x" : "-");
        printf(" ");
    }

    //-i: Print the number of links to file in inode table
    if(linksToFileFlg)
    {
        printf("%3d", typeStats.st_nlink);
        printf(" ");
    }

    //-u: Print the UID associated with the file
    if(fileUIDFlg)
    {
        if((pwd = getpwuid(typeStats.st_uid)) != NULL)
            printf("%8s", pwd-> pw_name);
        else
            printf("%8d", typeStats.st_uid);
    
        printf(" ");
    }

    //-g: Print the GID associated with the file
    if(fileGIDFlg)
    {
        if((grp = getgrgid(typeStats.st_gid)) != NULL)
            printf("%8s", grp-> gr_name);
        else
            printf("%8d", typeStats.st_gid);

        printf(" ");
    }

    //-s: Print the size of the file in bytes
    if(fileByteSizeFlg) 
    {

        long int byte = (long)typeStats.st_size;
        if(byte >= 1073741824) 
        {
            byte = (long)(byte / 1073741824);
            printf("%4dG", byte);
        }
        else if(byte >= 1048576) 
        {
            byte = (long)(byte / 1048576);
            printf("%4dM", byte);
        }
        else if(byte >= 1024) 
        {
            byte = (long)(byte / 1024);
            printf("%4dK", byte);
        }
        else
            printf("%5d", byte);

        printf(" ");
    }

    //-d: Show the time of the last modification
    if(lastModTimeFlg)
    {
        if(stat(path, &typeStats) == -1)
        {
            fprintf(stderr, "%s: Error: Unable to get file last modification time", argv[0]);
            perror("");
        }
        else 
        {
            char formDate[16];
            time_t time = typeStats.st_mtime;
            struct tm lastModTime;
            localtime_r(&time, &lastModTime);
            //Format the date to be Month(3) day, year and print it
            strftime(formDate, sizeof(formDate),"%b %e, %Y", &lastModTime);
            printf("%s", formDate);
        }

        printf(" ");
    }

    //If -l is passed in then -t -p -i -u -g -s are enable
    //-L is handled in SearchFileSystem.c
}
