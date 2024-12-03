/**************************************************************
* Class::  CSC-415-0# Spring 2024
* Name::
* Student IDs::
* GitHub-Name::
* Group-Name::
* Project:: Basic File System
*
* File:: b_io.c
*
* Description:: Basic File System - Key File I/O Operations
*
**************************************************************/

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "b_io.h"
#include "mfs.h"
#include "fsLow.h"
#include "parsePath.h"
#include "fsFreespace.h"

#define MAXFCBS 20
#define B_CHUNK_SIZE 512

typedef struct b_fcb {
    char buf[B_CHUNK_SIZE];    // Fixed size buffer
    int index;                 // Current position in buffer
    int buflen;                // Valid bytes in buffer
    int flags;                 // Access mode flags
    DirEntry file;            // File entry (not a pointer)
    uint32_t blockIndex;       // Current block number
    int isDirty;              // Buffer needs to be written
    int isUsed;               // FCB is in use
    int filePosition;
} b_fcb;
	
static b_fcb fcbArray[MAXFCBS];
static int startup = 0; //Indicates that this has not been initialized


//Method to initialize our file system
void b_init ()
	{
		int i;
	//init fcbArray to all free
	for (i = 0; i < MAXFCBS; i++) {
        fcbArray[i].isUsed = 0;
        fcbArray[i].isDirty = 0;
        fcbArray[i].index = 0;
        fcbArray[i].buflen = 0;
        fcbArray[i].blockIndex = 0;
	}
	startup = 1;
	}

//Method to get a free FCB element
b_io_fd b_getFCB ()
	{
		int i;
	for (i = 0; i < MAXFCBS; i++) {
        if (!fcbArray[i].isUsed) {
            fcbArray[i].isUsed = 1;
            fcbArray[i].index = 0;
            fcbArray[i].buflen = 0;
            fcbArray[i].blockIndex = 0;
            fcbArray[i].isDirty = 0;
            return i;		//Not thread safe (But do not worry about it for this assignment)
			}
		}
	return (-1);  //all in use
	}
	
// Interface to open a buffered file
// Modification of interface for this assignment, flags match the Linux flags for open
// O_RDONLY, O_WRONLY, or O_RDWR
b_io_fd b_open(char* filename, int flags) {
    if (startup == 0) b_init();

    b_io_fd fd = b_getFCB();
    if (fd < 0) {
        return -1;  // No free FCB available
    }

    // Initialize FCB
    fcbArray[fd].isUsed = 1;
    fcbArray[fd].index = 0;
    fcbArray[fd].buflen = 0;
    fcbArray[fd].flags = flags;
    fcbArray[fd].blockIndex = 0;
    fcbArray[fd].isDirty = 0;
    fcbArray[fd].filePosition = 0;

    // Use cwd as starting point
    DirEntry* parent = cwd;
    int index;
    char* lastElem;

    if (parsePath(filename, &parent, &index, &lastElem) == -1) {
        fcbArray[fd].isUsed = 0;  // Path parsing failed
        return -1;
    }

    // Handle file creation
    if (index == -1) {
        if (!(flags & O_CREAT)) {
            fcbArray[fd].isUsed = 0;  // File doesn't exist and O_CREAT not specified
            return -1;
        }

        // Find free entry
        int newIndex = -1;
        int numEntries = parent[0].size / sizeof(DirEntry);

        for (int i = 2; i < numEntries; i++) {
            if (!parent[i].isUsed) {
                newIndex = i;
                break;
            }
        }

        if (newIndex == -1) {
            fcbArray[fd].isUsed = 0;  // No free directory entries
            return -1;
        }

        // Allocate block for new file
        int newBlock = findFreeBlocks(1);
        if (newBlock == -1) {
            fcbArray[fd].isUsed = 0;  // No free blocks available
            return -1;
        }

        // Initialize new entry
        parent[newIndex].isUsed = 1;
        parent[newIndex].isDirectory = 0;
        parent[newIndex].size = 0;
        parent[newIndex].block = newBlock;
        strncpy(parent[newIndex].name, lastElem, sizeof(parent[newIndex].name) - 1);
        strncpy(parent[newIndex].dirName, lastElem, sizeof(parent[newIndex].dirName) - 1);

        // Write directory once for new file
        writeDir(parent);
        index = newIndex;
    }

    // Copy file info to FCB
    memcpy(&fcbArray[fd].file, &parent[index], sizeof(DirEntry));
    return fd;
}

// Interface to seek function	
int b_seek(b_io_fd fd, off_t offset, int whence) {
    if (!startup || fd < 0 || fd >= MAXFCBS || !fcbArray[fd].isUsed) {
        return -1;
    }

    long newPos;

    // Calculate new position
    switch (whence) {
        case SEEK_SET: newPos = offset; break;
        case SEEK_CUR: newPos = fcbArray[fd].blockIndex * B_CHUNK_SIZE + fcbArray[fd].index + offset; break;
        case SEEK_END: newPos = fcbArray[fd].file.size + offset; break;
        default: return -1;
    }

    if (newPos < 0 || newPos > fcbArray[fd].file.size) {
        return -1;
    }

    // Write current buffer if dirty
    if (fcbArray[fd].isDirty) {
        LBAwrite(fcbArray[fd].buf, 1, fcbArray[fd].file.block + fcbArray[fd].blockIndex);
        fcbArray[fd].isDirty = 0;
    }

    // Set new position
    fcbArray[fd].blockIndex = newPos / B_CHUNK_SIZE;
    fcbArray[fd].index = newPos % B_CHUNK_SIZE;
    
    // Load correct block
    if (LBAread(fcbArray[fd].buf, 1, fcbArray[fd].file.block + fcbArray[fd].blockIndex) != 1) {
        return -1;
    }
    fcbArray[fd].buflen = B_CHUNK_SIZE;

    return newPos;
}



// Interface to write function	
int b_write(b_io_fd fd, char* buffer, int count) {
    if (!startup || fd < 0 || fd >= MAXFCBS || !fcbArray[fd].isUsed) {
        printf("Error: Invalid file descriptor\n");
        return -1;
    }

    int blockNum = fcbArray[fd].file.block + (fcbArray[fd].filePosition / B_CHUNK_SIZE);
    int offset = fcbArray[fd].filePosition % B_CHUNK_SIZE;

    // Write to disk
    if (offset > 0 || count < B_CHUNK_SIZE) {
        // Need to read the block first for partial writes
        if (LBAread(fcbArray[fd].buf, 1, blockNum) != 1) {
            printf("Error: Failed to read block for writing\n");
            return -1;
        }
    }

    memcpy(fcbArray[fd].buf + offset, buffer, count);
    
    if (LBAwrite(fcbArray[fd].buf, 1, blockNum) != 1) {
        printf("Error: Failed to write to disk\n");
        return -1;
    }

    fcbArray[fd].filePosition += count;
    if (fcbArray[fd].filePosition > fcbArray[fd].file.size) {
        fcbArray[fd].file.size = fcbArray[fd].filePosition;
    }

    return count;
}



// Interface to read a buffer

// Filling the callers request is broken into three parts
// Part 1 is what can be filled from the current buffer, which may or may not be enough
// Part 2 is after using what was left in our buffer there is still 1 or more block
//        size chunks needed to fill the callers request.  This represents the number of
//        bytes in multiples of the blocksize.
// Part 3 is a value less than blocksize which is what remains to copy to the callers buffer
//        after fulfilling part 1 and part 2.  This would always be filled from a refill 
//        of our buffer.
//  +-------------+------------------------------------------------+--------+
//  |             |                                                |        |
//  | filled from |  filled direct in multiples of the block size  | filled |
//  | existing    |                                                | from   |
//  | buffer      |                                                |refilled|
//  |             |                                                | buffer |
//  |             |                                                |        |
//  | Part1       |  Part 2                                        | Part3  |
//  +-------------+------------------------------------------------+--------+
int b_read(b_io_fd fd, char* buffer, int count) {
    printf("\n--- Reading file: %s ---\n", fcbArray[fd].file.name);
    if (!startup || fd < 0 || fd >= MAXFCBS || !fcbArray[fd].isUsed) {
        printf("Error: Invalid file descriptor\n");
        return -1;
    }

    // Check read permission
    if ((fcbArray[fd].flags & O_ACCMODE) == O_WRONLY) {
        printf("Error: File not opened for reading\n");
        return -1;
    }

    printf("Current position: %d, File size: %d\n", 
           fcbArray[fd].filePosition, fcbArray[fd].file.size);

    // Check if we're at EOF
    if (fcbArray[fd].filePosition >= fcbArray[fd].file.size) {
        printf("At end of file\n");
        return 0;
    }

    // Calculate how much we can read
    int remainingBytes = fcbArray[fd].file.size - fcbArray[fd].filePosition;
    int bytesToRead = (count < remainingBytes) ? count : remainingBytes;

    printf("Reading %d bytes from block %d\n", 
           bytesToRead, fcbArray[fd].file.block + fcbArray[fd].blockIndex);

    // Read from disk
    if (LBAread(fcbArray[fd].buf, 1, 
                fcbArray[fd].file.block + fcbArray[fd].blockIndex) != 1) {
        printf("Error: Failed to read from disk\n");
        return -1;
    }

    // Copy to user's buffer
    int bufferOffset = fcbArray[fd].filePosition % B_CHUNK_SIZE;
    memcpy(buffer, fcbArray[fd].buf + bufferOffset, bytesToRead);

    // Update position
    fcbArray[fd].filePosition += bytesToRead;
    fcbArray[fd].blockIndex = fcbArray[fd].filePosition / B_CHUNK_SIZE;
    fcbArray[fd].index = fcbArray[fd].filePosition % B_CHUNK_SIZE;

    printf("Read %d bytes successfully\n", bytesToRead);
    return bytesToRead;
}

	
// Interface to Close the file	

int b_close(b_io_fd fd) {
    if (!startup || fd < 0 || fd >= MAXFCBS || !fcbArray[fd].isUsed) {
        return -1;
    }

    printf("\n--- Closing file: %s ---\n", fcbArray[fd].file.name);

    // Write any remaining dirty data
    if (fcbArray[fd].isDirty) {
        printf("Writing final buffer to disk\n");
        LBAwrite(fcbArray[fd].buf, 1, fcbArray[fd].file.block + fcbArray[fd].blockIndex);
    }

    // Update parent directory only if file size changed
    if (fcbArray[fd].file.size != fcbArray[fd].filePosition) {
        DirEntry* parent = cwd;  // Start with cwd
        int index;
        char* lastElem;

        if (parsePath(fcbArray[fd].file.name, &parent, &index, &lastElem) == 0 && index != -1) {
            printf("Updating file size in directory\n");
            parent[index].size = fcbArray[fd].filePosition;
            writeDir(parent);
        }
    }
    
    fcbArray[fd].isUsed = 0;
    printf("File closed successfully\n");
    return 0;
}