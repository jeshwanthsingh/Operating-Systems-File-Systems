/**************************************************************
* Class::  CSC-415-04 Spring 2024
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
#include "fsFreespace.c"
#include "parsePath.h"



#define ROOT_DIR_BLOCK 6
#define FREE_SPACE_BLOCK 1
#define VCB_BLOCK 0
#define DIR_ENTRY_BLOCKS 6
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

//Global VCB pointer
//static VCB* vcbPtr = NULL;
//Global cwd and root pointer
DirEntry * cwd = NULL;
DirEntry * root = NULL;

int initFileSystem(uint64_t numberOfBlocks, uint64_t blockSize) {
   printf("Initializing File System with %ld blocks and block size of %ld bytes\n", 
          numberOfBlocks, blockSize);

   printf("The size of Directory is %lu bytes\n", sizeof(DirEntry));
   
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
       loadFreeSpace();
       root = malloc(DIR_ENTRY_BLOCKS * vcbPtr->blockSize);
       LBAread(root, DIR_ENTRY_BLOCKS, ROOT_DIR_BLOCK);
       cwd = root;
       printf("The name of the root directory is %s \n", root[0].name);
       return 0; // Volume is already initialized
   }

   // Initialize VCB
   vcbPtr->signature = MY_SIGNATURE;
   strncpy(vcbPtr->volumeName, "MyVolume", 31);
   vcbPtr->totalBlocks = numberOfBlocks;
   vcbPtr->blockSize = blockSize;

   // Initialize and write the free space bitmap (5 blocks)
   initFreeSpace();
   vcbPtr->freeSpaceStart = FREE_SPACE_BLOCK;
   vcbPtr->freeBlocks = numberOfBlocks - 6;  // Subtract VCB, 5 bitmap blocks

   // Dynamically request 6 blocks for root directory and update VCB
   vcbPtr->rootDirectory = findFreeBlocks(6);  // Request 6 blocks from free space system
   printf("The starting of rootDirectory is %d\n",vcbPtr->rootDirectory);

   if (vcbPtr->rootDirectory == 0) {
       printf("Failed to allocate blocks for root directory\n");
       free(vcbPtr);
       return -1;
   }
   
   // Initialize root directory in the allocated blocks
   initRootDirectory();
   vcbPtr->freeBlocks = vcbPtr->freeBlocks - 6; //subtracting root directory blocks

   // Write VCB to block 0
   printf("Writing VCB to block %d...\n", VCB_BLOCK);
   if (LBAwrite(vcbPtr, 1, VCB_BLOCK) != 1) {
       printf("Error writing VCB\n");
       free(vcbPtr);
       return -1;
   }

   printf("VCB Verification:\n");
   printf("- Signature: 0x%llx\n", (unsigned long long)vcbPtr->signature);

   printf("File system initialized successfully!\n");
   return 0;
}

static void initRootDirectory(void) {
   // Allocate memory for the root directory (6 blocks)
   root = malloc(DIR_ENTRY_BLOCKS * vcbPtr->blockSize);
   if (!root) {
       printf("Failed to allocate root directory\n");
       return;
   }

   // Clear directory block
   memset(root, 0, DIR_ENTRY_BLOCKS * vcbPtr->blockSize);

   //set directories to free
   int numEntries = (DIR_ENTRY_BLOCKS*vcbPtr->blockSize)/sizeof(DirEntry);
   for (int i = 2; i < numEntries; i++){
        root[i].isUsed = 0;
   }

   // Create "." entry
   strcpy(root[0].dirName, "/");
   strncpy(root[0].name, ".", 1);
   root[0].size = DIR_ENTRY_BLOCKS * vcbPtr->blockSize;  // Set to total directory block size
   root[0].block = vcbPtr->rootDirectory;
   root[0].isDirectory = 1;
   root[0].isUsed = 1;

   // Create ".." entry (same as "." for root)
   strcpy(root[1].dirName, "/");
   strncpy(root[1].name, "..", 2);
   root[1].size = DIR_ENTRY_BLOCKS * vcbPtr->blockSize;
   root[1].block = vcbPtr->rootDirectory;
   root[1].isDirectory = 1;
   root[1].isUsed = 1;

   //memcpy(rootDir, cwd, sizeof(DirEntry));
   //memcpy(rootDir, root, sizeof(DirEntry));
   cwd = root;
   //strncpy(root[0].name, "f", 1);
   printf("The name of the root directory is %s", root[0].dirName);

   // Write root directory (6 blocks starting from vcbPtr->rootDirectory)
   printf("Writing root directory to block %d...\n", vcbPtr->rootDirectory);
   if (LBAwrite(root, DIR_ENTRY_BLOCKS, vcbPtr->rootDirectory) != DIR_ENTRY_BLOCKS) {
       printf("Error writing root directory\n");
       free(root);
       return;
   }

   //free(rootDir);
   printf("Root directory initialized\n");
}

void exitFileSystem() {
   // Write any cached data
   if (vcbPtr) {
       printf("Writing final VCB state...\n");
       LBAwrite(vcbPtr, 1, VCB_BLOCK);
       LBAwrite(freeSpaceMap, 5, FREE_SPACE_BLOCK);
       free(vcbPtr);
       free(freeSpaceMap);
       free(root);
       //free(cwd);
       root = NULL;
       cwd = NULL;
       vcbPtr = NULL;
   }

   printf("File system exited cleanly\n");
}