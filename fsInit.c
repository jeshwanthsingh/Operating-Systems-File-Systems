/**************************************************************
* Class::  CSC-415-0# Spring 2024
* Name::
* Student IDs::
* GitHub-Name::
* Group-Name::
* Project:: Basic File System
*
* File:: fsInit.c
*
* Description:: Main driver for file system assignment.
*
* This file is where you will start and initialize your system
*
**************************************************************/



 #include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <stdio.h>
#include <string.h>

#include "fsLow.h"
#include "mfs.h"

// Define the VCB structure
typedef struct volumeControlBlock {
    char volumeName[32];
    uint64_t signature;
    uint32_t totalBlocks;
    uint32_t blockSize;
    uint32_t freeBlocks;
    uint32_t rootDirectory;
    uint32_t freeSpaceStart;
} VCB;

// Define constants
#define ROOT_DIR_BLOCK 2
#define FREE_SPACE_BLOCK 1
#define VCB_BLOCK 0
#define MY_SIGNATURE 0x415415415  // Use your custom signature

int initFileSystem(uint64_t numberOfBlocks, uint64_t blockSize) {
    printf("Initializing File System with %ld blocks and block size of %ld bytes\n", 
           numberOfBlocks, blockSize);

    VCB vcb = {0};  // Initialize VCB structure
    strcpy(vcb.volumeName, "MyVolume");
    vcb.signature = MY_SIGNATURE;
    vcb.totalBlocks = numberOfBlocks;
    vcb.blockSize = blockSize;
    vcb.freeBlocks = numberOfBlocks - 3;
    vcb.rootDirectory = ROOT_DIR_BLOCK;
    vcb.freeSpaceStart = FREE_SPACE_BLOCK;

    printf("Writing VCB to the first block...\n");
    if (LBAwrite(&vcb, 1, VCB_BLOCK) != 1) {
        printf("Error writing VCB to disk. Blocks written: 0\n");
        return -1;
    }

    printf("VCB written successfully!\n");
    return 0;
}

void exitFileSystem() {
    printf("System exiting\n");
}