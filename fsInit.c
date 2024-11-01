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
   uint64_t signature;          // Put signature first (8-byte alignment)
   char volumeName[32];         // Then the volume name
   uint32_t totalBlocks;
   uint32_t blockSize;
   uint32_t freeBlocks;
   uint32_t rootDirectory;
   uint32_t freeSpaceStart;
   uint8_t padding[452];        // Pad to exactly 512 bytes
} __attribute__((packed)) VCB;

// Directory entry structure
typedef struct {
   char name[32];
   uint32_t size;
   uint32_t block;
   uint8_t isDirectory;
   uint8_t padding[23];
} DirEntry;

// Define constants
#define ROOT_DIR_BLOCK 2
#define FREE_SPACE_BLOCK 1
#define VCB_BLOCK 0
#define MY_SIGNATURE 0x415415415ULL  // Note the ULL suffix

// Function prototypes
int initFileSystem(uint64_t numberOfBlocks, uint64_t blockSize);
void exitFileSystem(void);
static void initRootDirectory(void);
static void initFreeSpace(uint64_t numberOfBlocks);

// Global VCB pointer
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

   // Initialize VCB
   vcbPtr->signature = MY_SIGNATURE;
   strncpy(vcbPtr->volumeName, "MyVolume", 31);
   vcbPtr->totalBlocks = numberOfBlocks;
   vcbPtr->blockSize = blockSize;
   vcbPtr->freeBlocks = numberOfBlocks - 3;  // Subtract VCB, bitmap, and root blocks
   vcbPtr->rootDirectory = ROOT_DIR_BLOCK;
   vcbPtr->freeSpaceStart = FREE_SPACE_BLOCK;

   // Write VCB
   printf("Writing VCB to block %d...\n", VCB_BLOCK);
   if (LBAwrite(vcbPtr, 1, VCB_BLOCK) != 1) {
       printf("Error writing VCB\n");
       free(vcbPtr);
       return -1;
   }

   printf("VCB Verification:\n");
   printf("- Signature: 0x%llx\n", (unsigned long long)vcbPtr->signature);

   // Initialize free space bitmap
   initFreeSpace(numberOfBlocks);

   // Initialize root directory
   initRootDirectory();

   printf("File system initialized successfully!\n");
   return 0;
}

static void initFreeSpace(uint64_t numberOfBlocks) {
   // Allocate bitmap block
   uint8_t* bitmap = (uint8_t*)malloc(vcbPtr->blockSize);
   if (!bitmap) {
       printf("Failed to allocate bitmap\n");
       return;
   }

   // Clear bitmap
   memset(bitmap, 0, vcbPtr->blockSize);

   // Mark first 3 blocks as used (VCB, bitmap, root)
   bitmap[0] = 0x07;  // Binary: 00000111

   // Write bitmap to disk
   printf("Writing free space bitmap to block %d...\n", FREE_SPACE_BLOCK);
   if (LBAwrite(bitmap, 1, FREE_SPACE_BLOCK) != 1) {
       printf("Error writing bitmap\n");
       free(bitmap);
       return;
   }

   // Verify bitmap
   uint8_t verifyBitmap[512];
   if (LBAread(verifyBitmap, 1, FREE_SPACE_BLOCK) == 1) {
       printf("Bitmap verification: first byte = 0x%02X (should be 0x07)\n", verifyBitmap[0]);
   }

   free(bitmap);
   printf("Free space bitmap initialized\n");
}

static void initRootDirectory(void) {
   // Allocate directory block
   DirEntry* rootDir = (DirEntry*)malloc(vcbPtr->blockSize);
   if (!rootDir) {
       printf("Failed to allocate root directory\n");
       return;
   }

   // Clear directory block
   memset(rootDir, 0, vcbPtr->blockSize);

   // Create "." entry
   strncpy(rootDir[0].name, ".", 1);
   rootDir[0].size = vcbPtr->blockSize;
   rootDir[0].block = ROOT_DIR_BLOCK;
   rootDir[0].isDirectory = 1;

   // Create ".." entry (same as "." for root)
   strncpy(rootDir[1].name, "..", 2);
   rootDir[1].size = vcbPtr->blockSize;
   rootDir[1].block = ROOT_DIR_BLOCK;
   rootDir[1].isDirectory = 1;

   // Write root directory
   printf("Writing root directory to block %d...\n", ROOT_DIR_BLOCK);
   if (LBAwrite(rootDir, 1, ROOT_DIR_BLOCK) != 1) {
       printf("Error writing root directory\n");
       free(rootDir);
       return;
   }

   // Verify root directory
   DirEntry verifyRoot[vcbPtr->blockSize / sizeof(DirEntry)];
   if (LBAread(verifyRoot, 1, ROOT_DIR_BLOCK) == 1) {
       printf("Root directory verification:\n");
       printf("- First entry: %s (isDirectory=%d)\n", verifyRoot[0].name, verifyRoot[0].isDirectory);
       printf("- Second entry: %s (isDirectory=%d)\n", verifyRoot[1].name, verifyRoot[1].isDirectory);
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