

#ifndef ORIGIN_H
#define ORIGIN_H

#include <stdio.h> 
#include <stdlib.h>   
#include <string.h>   
#include <stdbool.h>  
#include <sys/types.h> 
#include <sys/stat.h> 
#include <unistd.h>  
#include <dirent.h>   
#include <errno.h>   

#ifdef __linux__
#include <sys/xattr.h> 
#endif

// we define the name of the extended attribute to store "origin" metadata
#define ORIGIN_XATTR_NAME "user.origin"

void print_file_info(const char *filepath, bool show_origin_flag);

char *get_xattr_value(const char *filepath, const char *attr_name);

#endif // ORIGIN_H




