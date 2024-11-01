/**************************************************************
* Class::  CSC-415-0# Spring 2024
* GitHub-Name:: jeshwanthsingh
* Project:: Basic File System
*
* File:: fsInit.c
*
* Description:: Main driver for file system assignment.
**************************************************************/

#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include "fsLow.h"
#include "mfs.h"

// Define the VCB structure
typedef struct volumeControlBlock {
   uint64_t signature;          
   char volumeName[32];         
   uint32_t totalBlocks;
   uint32_t blockSize;
   uint32_t freeBlocks;
   uint32_t rootDirectory;
   uint32_t freeSpaceStart;
   uint8_t padding[452];        
} __attribute__((packed)) VCB;

typedef struct {
   char name[32];
   uint32_t size;
   uint32_t block;
   uint8_t isDirectory;
   uint8_t padding[23];
} DirEntry;

#define ROOT_DIR_BLOCK 2
#define FREE_SPACE_BLOCK 1
#define VCB_BLOCK 0
#define MY_SIGNATURE 0x415415415ULL  

// Function prototypes
/* initFileSystem: initializes file system, given 2 arguments: it allocates space for the file system to 
 be stored by creating and iniitializing the Volume Control Block (VCB), writing it to block 0 on disk.
 Also initializes free-space bitmap. Finally, creates the root directories '.' and '..'
 2 arguments: numberOfBlocks and blockSize (size of each block)
 numberOfBlocks: number of 'cells' in memory.
 blockSize: size of each block (in bytes, defined as an unsigned 64 bit int)
  Returns: int (success or failure?)
*/

int initFileSystem(uint64_t numberOfBlocks, uint64_t blockSize);
//exit file system
void exitFileSystem(void);
static void initRootDirectory(void);
static void initFreeSpace(uint64_t numberOfBlocks);

//Global VCB pointer
static VCB* vcbPtr = NULL;

int initFileSystem(uint64_t numberOfBlocks, uint64_t blockSize) {
   printf("Initializing File System with %ld blocks and block size of %ld bytes\n", 
          numberOfBlocks, blockSize);

   // Allocate and clear VCB
   vcbPtr = (VCB*)malloc(blockSize);
   if (!vcbPtr) {
       printf("Failed to allocate VCB\n");
       return -1;
   }
   memset(vcbPtr, 0, blockSize);

   // Check if the Volume is already Initialized
   if (LBAread(vcbPtr, 1, VCB_BLOCK) != 1) {
       printf("Failed to read block 0\n");
   }

   // Check if signature matches
   if (vcbPtr->signature == MY_SIGNATURE) {
       printf("Signature match found, Volume already initialized\n");
       return 0; // Volume is already initialized
   }

   // Initialize VCB
   vcbPtr->signature = MY_SIGNATURE;
   strncpy(vcbPtr->volumeName, "MyVolume", 31);
   vcbPtr->totalBlocks = numberOfBlocks;
   vcbPtr->blockSize = blockSize;
   vcbPtr->freeBlocks = numberOfBlocks - 12;  // Subtract VCB, 5 bitmap blocks, and 6 root directory blocks
   vcbPtr->freeSpaceStart = FREE_SPACE_BLOCK;

   // Initialize and write the free space bitmap (5 blocks)
   initFreeSpace(numberOfBlocks);

   // Dynamically request 6 blocks for root directory and update VCB
   vcbPtr->rootDirectory = 6;  // Request 6 blocks from free space system
   if (vcbPtr->rootDirectory == 0) {
       printf("Failed to allocate blocks for root directory\n");
       free(vcbPtr);
       return -1;
   }

   // Write VCB to block 0
   printf("Writing VCB to block %d...\n", VCB_BLOCK);
   if (LBAwrite(vcbPtr, 1, VCB_BLOCK) != 1) {
       printf("Error writing VCB\n");
       free(vcbPtr);
       return -1;
   }

   printf("VCB Verification:\n");
   printf("- Signature: 0x%llx\n", (unsigned long long)vcbPtr->signature);

   // Initialize root directory in the allocated blocks
   initRootDirectory();

   printf("File system initialized successfully!\n");
   return 0;
}

static void initFreeSpace(uint64_t numberOfBlocks) {
   // Allocate 5 blocks for the bitmap
   uint8_t* bitmap = (uint8_t*)malloc(5 * vcbPtr->blockSize);
   if (!bitmap) {
       printf("Failed to allocate bitmap\n");
       return;
   }

   // Clear bitmap and mark first 8 blocks as used (VCB, 5 bitmap, 6 root)
   memset(bitmap, 0, 5 * vcbPtr->blockSize);
   for (int i = 0; i < 12; i++) {
       bitmap[i / 8] |= (1 << (i % 8));
   }

   // Write bitmap to disk (5 blocks starting from block 1)
   printf("Writing free space bitmap to block %d...\n", FREE_SPACE_BLOCK);
   if (LBAwrite(bitmap, 5, FREE_SPACE_BLOCK) != 5) {
       printf("Error writing bitmap\n");
       free(bitmap);
       return;
   }

   free(bitmap);
   printf("Free space bitmap initialized\n");
}

static void initRootDirectory(void) {
   // Allocate memory for the root directory (6 blocks)
   DirEntry* rootDir = (DirEntry*)malloc(6 * vcbPtr->blockSize);
   if (!rootDir) {
       printf("Failed to allocate root directory\n");
       return;
   }

   // Clear directory block
   memset(rootDir, 0, 6 * vcbPtr->blockSize);

   // Create "." entry
   strncpy(rootDir[0].name, ".", 1);
   rootDir[0].size = 6 * vcbPtr->blockSize;  // Set to total directory block size
   rootDir[0].block = vcbPtr->rootDirectory;
   rootDir[0].isDirectory = 1;

   // Create ".." entry (same as "." for root)
   strncpy(rootDir[1].name, "..", 2);
   rootDir[1].size = 6 * vcbPtr->blockSize;
   rootDir[1].block = vcbPtr->rootDirectory;
   rootDir[1].isDirectory = 1;

   // Write root directory (6 blocks starting from vcbPtr->rootDirectory)
   printf("Writing root directory to block %d...\n", vcbPtr->rootDirectory);
   if (LBAwrite(rootDir, 6, vcbPtr->rootDirectory) != 6) {
       printf("Error writing root directory\n");
       free(rootDir);
       return;
   }

   free(rootDir);
   printf("Root directory initialized\n");
}

void exitFileSystem() {
   // Write any cached data
   if (vcbPtr) {
       printf("Writing final VCB state...\n");
       LBAwrite(vcbPtr, 1, VCB_BLOCK);
       free(vcbPtr);
       vcbPtr = NULL;
   }

   printf("File system exited cleanly\n");
}