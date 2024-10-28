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

typedef struct volumeControlBlock {
    char volumeName[32];
    uint64_t signature;
    uint32_t totalBlocks;
    uint32_t blockSize;
    uint32_t freeBlocks;
    uint32_t rootDirectory;
} VCB;

int initFileSystem(uint64_t numberOfBlocks, uint64_t blockSize) {
    printf("Initializing File System with %ld blocks and block size of %ld bytes\n", numberOfBlocks, blockSize);

    if (startPartitionSystem("volume.dat", &numberOfBlocks, &blockSize) != 0) {
        printf("Failed to initialize partition system.\n");
        return -1;
    }

    VCB vcb = {0};
    strcpy(vcb.volumeName, "MyVolume");
    vcb.signature = 0x41504141;  // Example signature
    vcb.totalBlocks = numberOfBlocks;
    vcb.blockSize = blockSize;
    vcb.freeBlocks = numberOfBlocks - 1;
    vcb.rootDirectory = 1;

    printf("Writing VCB to the first block...\n");
    uint64_t blocksWritten = LBAwrite(&vcb, 1, 0);
    if (blocksWritten != 1) {
        printf("Error writing VCB to disk. Blocks written: %ld\n", blocksWritten);
        perror("LBAwrite failed");  // Print detailed error information
        return -1;
    }

    printf("VCB written successfully!\n");
    return 0;
}

void exitFileSystem() {
    printf("System exiting\n");
    closePartitionSystem();
}

int main() {
    uint64_t totalBlocks = 20000;
    uint64_t blockSize = 512;

    if (initFileSystem(totalBlocks, blockSize) == 0) {
        printf("Initialization completed successfully.\n");
    } else {
        printf("Initialization failed.\n");
    }

    exitFileSystem();
    return 0;
}