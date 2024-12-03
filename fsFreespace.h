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

extern int freeSpaceLocation;
extern int numSpace;
extern uint8_t *freeSpaceMap;

void initFreeSpace();
void loadFreeSpace();
int findFreeBlocks(int numOfBlocks);
void releaseSpace(int entryLocation, int numBlocks);
int checkFree(int blockNumber);

#endif // FS_FREESPACE_H