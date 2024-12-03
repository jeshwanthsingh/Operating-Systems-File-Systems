/**************************************************************
* Class::  CSC-415-04 Spring 2024
* GitHub-Name:: jeshwanthsingh
* Project:: Basic File System
*
* File:: fsFreespace.h
*
* Description:: Header file for freesspace.c of the filesystem
**************************************************************/

void initFreeSpace();
//int increaseSpace();
void loadFreeSpace();
int findFreeBlocks(int numOfBlocks);
void releaseSpace(int entryLocation, int numBlocks);
int checkFree(int blockNumber);