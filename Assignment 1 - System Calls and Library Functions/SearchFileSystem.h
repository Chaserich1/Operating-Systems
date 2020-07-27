/* Author: Chase Richards
   Project: Homework 1 CS4760
   Date: January 29, 2020  
   Filename: SearchFileSystem.h  */

#ifndef SEARCHFILESYSTEM_H
#define SEARCHFILESYSTEM_H

#include "DirectoryCheck.h"
#include "CommandOptions.h"

#include <unistd.h>
#include <stddef.h>
#include <dirent.h>
#include <errno.h>
#include <string.h>
#include <pwd.h>
#include <grp.h>
#include <sys/types.h>
#include <time.h>

struct stat typeStats;
struct passwd *pwd;
struct group *grp;

void breadthFirstTraversal(char *path, char **argv);

#endif
