/**************************************************************
* Class::  CSC-415-04 Spring 2024
* GitHub-Name:: jeshwanthsingh
* Project:: Basic File System
*
* File:: fsFreespace.c
*
* Description:: Freesspace management of the filesystem
**************************************************************/

#include <stdio.h>
#include <stdint.h>
#include "mfs.h"
#include "fsFreespace.h"
#include "fsLow.h"
#include <stdlib.h> // for malloc and free
#include <string.h> // for memset
#define CHARSIZE 8 		// Number for the 8 bits in an unsigned char
#define TOTAL_FREE_SPACE 2560 	//Total bytes for free space
#define FREE_SPACE_LIMIT CHARSIZE*TOTAL_FREE_SPACE
#define FREE_SPACE_DEBUG 0
#define FREE_SPACE_BLOCKS 5

int freeSpaceLocation = 0;	// Number that signifies beginning of free space
int numSpace = 0;
uint8_t *freeSpaceMap = NULL;

void initFreeSpace() {
    // Allocate 5 blocks for the bitmap
    freeSpaceMap = malloc(TOTAL_FREE_SPACE);
    if (!freeSpaceMap) {
        printf("Failed to allocate bitmap\n");
        return;
    }

    // Clear bitmap and mark every block free
    memset(freeSpaceMap, 0, TOTAL_FREE_SPACE);
   
    // Mark the first 6 blocks as used (VCB + free space map)
    for (int i = 0; i < 6; i++) {
        int byteIndex = i / 8;
        int bitIndex = 7 - i % 8;
        freeSpaceMap[byteIndex] |= (1 << bitIndex);  // Set the bit
    }

    printf("First byte of bitmap: %02X\n", freeSpaceMap[0]); // Should print FC

    // Write bitmap to disk (5 blocks starting from block 1)
    printf("Writing free space bitmap to block %d...\n", 1);
    if (LBAwrite(freeSpaceMap, FREE_SPACE_BLOCKS, 1) != FREE_SPACE_BLOCKS) {
        printf("Error writing bitmap\n");
        free(freeSpaceMap);
        return;
    }

    //free(freeSpaceMap);
    printf("Free space bitmap initialized\n");
}

void loadFreeSpace(){
    freeSpaceMap = malloc(TOTAL_FREE_SPACE);
    if (LBAread(freeSpaceMap, FREE_SPACE_BLOCKS, 1) != FREE_SPACE_BLOCKS){
        printf("Error loading bitmap\n");
        free(freeSpaceMap);
        return;
    }
    printf("Freespace loaded successfully\n");
}

// Function to find n consecutive free blocks
int findFreeBlocks(int numOfBlocks) {
    int consecutive = 0;  // Tracks consecutive free blocks
    int startBlock = 0;   // Tracks the starting block number of free sequence

    for (int block = 0; block < TOTAL_FREE_SPACE; block++) {
        int byteIndex = block / 8;  // Find which byte this block is in
        int bitIndex = block % 8;  // Find which bit within the byte
        //printf("Checking block %d\n",block);

        // Check if the block is free (bit is 0)
        if ((freeSpaceMap[byteIndex] & (1 << (7 - bitIndex))) == 0) {
            // Increment the consecutive count
            if (consecutive == 0) {
                startBlock = block; // Potential start of free sequence
            }
            consecutive++;

            // If we have found n free blocks, mark them used and return the start
            if (consecutive == numOfBlocks) {
                // Mark the blocks as used
                for (int i = 0; i < numOfBlocks; i++) {
                    int setBlock = startBlock + i;
                    int setByteIndex = setBlock / 8;
                    int setBitIndex = setBlock % 8;
                    freeSpaceMap[setByteIndex] |= (1 << (7 - setBitIndex));
                }
                return startBlock; // Return the starting block number
            }
        } else {
            // Reset the consecutive count if a used block is encountered
            consecutive = 0;
        }
    }

    // If no sequence of n free blocks is found, return -1
    return -1;
}

// Function to release space in the bitmap
void releaseSpace(int blockNumber, int numBlocks) {
    // Check for invalid input
    if (blockNumber + numBlocks > FREE_SPACE_LIMIT) {
        fprintf(stderr, "Error: Block range exceeds total blocks.\n");
        return;
    }

    // Mark the specified blocks as free
    for (int i = 0; i < numBlocks; i++) {
        int currentBlock = blockNumber + i;
        int byteIndex = currentBlock / 8;  // Locate the byte
        int bitIndex = currentBlock % 8;  // Locate the bit within the byte

        freeSpaceMap[byteIndex] &= ~(1 << (7 - bitIndex));  // Set the bit to 0
    }

    printf("Released %d blocks starting from block %d.\n", numBlocks, blockNumber);
}

int checkFree(int blockNumber){

    int byteIndex = blockNumber / 8;
    int bitIndex = blockNumber % 8;

    return freeSpaceMap[byteIndex] & (1 << (7-bitIndex));
}