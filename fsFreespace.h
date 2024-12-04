/**************************************************************
* Class::  CSC-415-04 Spring 2024
* GitHub-Name:: jeshwanthsingh
* Project:: Basic File System
*
* File:: fsFreespace.h
*
* Description:: Header file for freesspace.c of the filesystem
**************************************************************/
#ifndef FS_FREESPACE_H
#define FS_FREESPACE_H

extern int freeSpaceLocation;   // free space location
extern int numSpace;    // number of free blocks
extern uint8_t *freeSpaceMap;   // free space map

void initFreeSpace();   // initialize free space
void loadFreeSpace();   // load free space
int findFreeBlocks(int numOfBlocks);    // find n consecutive free blocks
void releaseSpace(int entryLocation, int numBlocks);    // release space in the bitmap
int checkFree(int blockNumber); // check free space

#endif // FS_FREESPACE_H