/**************************************************************
* Class::  CSC-415-0# Spring 2024
* Name::
* Student IDs::
* GitHub-Name::
* Group-Name::
* Project:: Basic File System
*
* File:: b_io.h
*
* Description:: Interface of basic I/O Operations
*
**************************************************************/

#ifndef _B_IO_H
#define _B_IO_H
#include <fcntl.h>
#define B_CHUNK_SIZE 512

typedef int b_io_fd;        // File descriptor

b_io_fd b_open (char * filename, int flags);  //open file
int b_read (b_io_fd fd, char * buffer, int count);  //read from file
int b_write (b_io_fd fd, char * buffer, int count); //write from file
int b_seek (b_io_fd fd, off_t offset, int whence);  //seek from file
int b_close (b_io_fd fd);   //close file

#endif

