/* Author: Chase Richards
   Project: Homework 1 CS4760
   Date: January 31, 2020  
   Filename: CommandOptions.h  */

#ifndef COMMANDOPTIONS_H
#define COMMANDOPTIONS_H

#include "SearchFileSystem.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>
#include <ctype.h>

extern int helpMessageFlg;    // -h
extern int symbolicLinkFlg;   // -L
extern int fileTypeInfoFlg;   // -t
extern int permissionFlg;     // -p
extern int linksToFileFlg;    // -i
extern int fileUIDFlg;        // -u
extern int fileGIDFlg;        // -g
extern int fileByteSizeFlg;   // -s
extern int lastModTimeFlg;    // -d
extern int tpiugsFlg;         // -l

void flgsPassedIn(int argc, char **argv);
void printOptions(char *path, char **argv);
void displayHelpMessage();

#endif
