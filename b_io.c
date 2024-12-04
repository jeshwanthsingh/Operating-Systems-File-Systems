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

#define MAXFCBS 20      // maximum number of open files
#define B_CHUNK_SIZE 512   // maximum size of a block

typedef struct b_fcb {
    char buf[B_CHUNK_SIZE];    // Fixed size buffer
    int index;                 // Current position in buffer
    int buflen;                // Valid bytes in buffer
    int flags;                 // Access mode flags
    DirEntry file;            // File entry (not a pointer)
    uint32_t blockIndex;       // Current block number
    int isDirty;              // Buffer needs to be written
    int isUsed;               // FCB is in use
    int filePosition;       // Current file position
} b_fcb;
	
static b_fcb fcbArray[MAXFCBS];
static int startup = 0; //Indicates that this has not been initialized

/**
 * b_init
 * Initializes the file system by resetting all entries in the File Control Block (FCB) array.
 * 
 * Behavior:
 * - Marks all FCB entries as unused.
 * - Resets buffer-related properties for each FCB.
 * - Sets the `startup` flag to indicate the file system is initialized.
 * 
 * Notes:
 * - This function is automatically called by `b_open` if the file system has not been initialized.
 */
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

/**
 * b_getFCB
 * Retrieves a free File Control Block (FCB) from the array of FCBs.
 * 
 * Returns:
 * - Index of the free FCB if available.
 * - -1 if no free FCB is available.
 * 
 * Behavior:
 * - Iterates through the FCB array to find an unused FCB.
 * - Marks the FCB as used and resets its properties.
 * 
 * Notes:
 * - This function is not thread-safe, but thread safety is not required for this assignment.
 */
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
/**
 * b_open
 * Opens a file and initializes an FCB for it.
 * 
 * Parameters:
 * - filename: The name of the file to be opened.
 * - flags: Access mode flags (e.g., O_RDONLY, O_WRONLY, O_RDWR).
 * 
 * Returns:
 * - File descriptor (index of the FCB) on success.
 * - -1 if an error occurs (e.g., no free FCB, file not found, or path parsing fails).
 * 
 * Behavior:
 * - Allocates an FCB for the file.
 * - If the file does not exist and O_CREAT is specified, creates the file and initializes its directory entry.
 * - Populates the FCB with information about the file.
 * 
 * Notes:
 * - Uses `parsePath` to locate the file or its parent directory.
 * - Handles file creation if the file does not exist.
 */
b_io_fd b_open(char* filename, int flags) {     //open file
    if (startup == 0) b_init();/*initialize if not already done*/   

    b_io_fd fd = b_getFCB();  // Get free FCB
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

    DirEntry* parent;
    int index;
    char* lastElem;

    if (parsePath(filename, &parent, &index, &lastElem) == -1) {            //parse the path
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
        int newIndex = -1;      // Number of entries in the parent directory
        int numParentEntries = parent[0].size / sizeof(DirEntry);       // Number of entries in the parent directory

        for (int i = 2; i < numParentEntries; i++) {        // Number of entries in the parent directory
            if (!parent[i].isUsed) {        // Directory entry is not used
                newIndex = i;
                break;
            }
        }

        if (newIndex == -1) {
            fcbArray[fd].isUsed = 0;  // No free directory entries
            return -1;
        }

        printf("---Creating the file %s---\n",filename);
        // Allocate block for new file
        DirEntry* newFile;
        int numEntries = B_CHUNK_SIZE/sizeof(DirEntry);
        //printf("The number of entries is %d\n",numEntries);
        newFile = createDir(numEntries,parent,filename);
        //int newBlock = findFreeBlocks(1);
        /*
        printf("free space found at %d\n",newBlock);
        if (newBlock == -1) {
            fcbArray[fd].isUsed = 0;  // No free blocks available
            return -1;
        }
        */

        // Initialize new entry in parent
        //printf("The file is created in parent %s at index %d\n",parent[0].dirName,newIndex);
        parent[newIndex].isUsed = 1;
        parent[newIndex].isDirectory = 0;
        parent[newIndex].size = B_CHUNK_SIZE;
        //printf("size allocation is okay\n");
        parent[newIndex].block = newFile[0].block;
        strcpy(parent[newIndex].name, lastElem);
        strcpy(parent[newIndex].dirName, lastElem);

        // Write directory once for new file
        //printf("Writing parent to disk\n");
        if(writeDir(parent)!=0){
            printf("Write Dir Failed");
        };
        free(newFile);
        FreeDir(parent);
        index = newIndex;
    }

    // Copy file info to FCB
    memcpy(&fcbArray[fd].file, &parent[index], sizeof(DirEntry));
    return fd;
}
/**
 * b_seek
 * Adjusts the file position for a file opened in buffered I/O.
 * 
 * Parameters:
 * - fd: File descriptor of the file.
 * - offset: Byte offset to seek to.
 * - whence: Reference point for offset (SEEK_SET, SEEK_CUR, SEEK_END).
 * 
 * Returns:
 * - New file position on success.
 * - -1 if an error occurs (e.g., invalid file descriptor, invalid offset).
 * 
 * Behavior:
 * - Calculates the new position based on `whence`.
 * - Writes any dirty data in the buffer to disk before moving the position.
 * - Reads the appropriate block into the buffer.
 */
// Interface to seek function	
int b_seek(b_io_fd fd, off_t offset, int whence) {
    if (!startup || fd < 0 || fd >= MAXFCBS || !fcbArray[fd].isUsed) {
        return -1;
    }

    long newPos;

    // Calculate new position
    switch (whence) {
        case SEEK_SET: newPos = offset; break;      // Set position to offset
        case SEEK_CUR: newPos = fcbArray[fd].blockIndex * B_CHUNK_SIZE + fcbArray[fd].index + offset; break;
        case SEEK_END: newPos = fcbArray[fd].file.size + offset; break;  // End of file
        default: return -1;
    }

    if (newPos < 0 || newPos > fcbArray[fd].file.size) {        // Check if new position is valid
        return -1;
    }

    // Write current buffer if dirty
    if (fcbArray[fd].isDirty) {         // Write buffer to disk
        LBAwrite(fcbArray[fd].buf, 1, fcbArray[fd].file.block + fcbArray[fd].blockIndex);
        fcbArray[fd].isDirty = 0;       // Reset dirty flag
    }

    // Set new position
    fcbArray[fd].blockIndex = newPos / B_CHUNK_SIZE;        // Calculate block index
    fcbArray[fd].index = newPos % B_CHUNK_SIZE;     // Calculate index within block
    
    // Load correct block
    if (LBAread(fcbArray[fd].buf, 1, fcbArray[fd].file.block + fcbArray[fd].blockIndex) != 1) {
        return -1;
    }
    fcbArray[fd].buflen = B_CHUNK_SIZE;     // Calculate buffer length

    return newPos;
}


/**
 * b_write
 * Writes data to a file using buffered I/O.
 * 
 * Parameters:
 * - fd: File descriptor of the file.
 * - buffer: Pointer to the data to be written.
 * - count: Number of bytes to write.
 * 
 * Returns:
 * - Number of bytes written on success.
 * - -1 if an error occurs (e.g., invalid file descriptor, disk write failure).
 * 
 * Behavior:
 * - Handles partial writes by reading the block first if necessary.
 * - Updates the file's size and position after writing.
 * 
 * Notes:
 * - Writes directly to the disk if the buffer is full.
 */
// Interface to write function	
int b_write(b_io_fd fd, char* buffer, int count) {          // Buffer to write, number of bytes to write
    if (!startup || fd < 0 || fd >= MAXFCBS || !fcbArray[fd].isUsed) {
        printf("Error: Invalid file descriptor\n");
        return -1;
    }

    int blockNum = fcbArray[fd].file.block + (fcbArray[fd].filePosition / B_CHUNK_SIZE);        // Block number to write to
    int offset = fcbArray[fd].filePosition % B_CHUNK_SIZE;      // Offset within block

    // Write to disk
    if (offset > 0 || count < B_CHUNK_SIZE) {
        // Need to read the block first for partial writes
        if (LBAread(fcbArray[fd].buf, 1, blockNum) != 1) {      // partial write
            printf("Error: Failed to read block for writing\n");
            return -1;
        }
    }

    memcpy(fcbArray[fd].buf + offset, buffer, count);       // Copy buffer to FCB buffer
    
    if (LBAwrite(fcbArray[fd].buf, 1, blockNum) != 1) {     // Write buffer to disk
        printf("Error: Failed to write to disk\n");
        return -1;
    }

    fcbArray[fd].filePosition += count;     // Update file position
    if (fcbArray[fd].filePosition > fcbArray[fd].file.size) {       // Update file size
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
int b_read(b_io_fd fd, char* buffer, int count) {       // read from file, buffer to read into, number of bytes to read
    printf("\n--- Reading file: %s ---\n", fcbArray[fd].file.name);     // read from file
    if (!startup || fd < 0 || fd >= MAXFCBS || !fcbArray[fd].isUsed) {
        printf("Error: Invalid file descriptor\n");
        return -1;
    }

    // Check read permission
    if ((fcbArray[fd].flags & O_ACCMODE) == O_WRONLY) {     // Check if file is opened for writing
        printf("Error: File not opened for reading\n");     // File not opened for reading
        return -1;
    }

    printf("Current position: %d, File size: %d\n",         // Current position and file size
           fcbArray[fd].filePosition, fcbArray[fd].file.size);      // Current position and file size

    // Check if we're at EOF
    if (fcbArray[fd].filePosition >= fcbArray[fd].file.size) {      // If we're at EOF
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
/**
 * b_close
 * Closes a file and releases its FCB.
 * 
 * Parameters:
 * - fd: File descriptor of the file to be closed.
 * 
 * Returns:
 * - 0 on success.
 * - -1 if an error occurs (e.g., invalid file descriptor).
 * 
 * Behavior:
 * - Writes any remaining dirty data in the buffer to disk.
 * - Updates the file size in the parent directory if it has changed.
 * - Marks the FCB as unused.
 */
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
        DirEntry* parent;
        int index;
        char* lastElem;

            // Parse path
        if (parsePath(fcbArray[fd].file.name, &parent, &index, &lastElem) == 0 && index != -1) {
            printf("Updating file size in directory\n");
            parent[index].size = fcbArray[fd].filePosition;     // update file size in directory
            writeDir(parent);       // update file size in directory
        }
    }
    
    fcbArray[fd].isUsed = 0;        // Free FCB
    printf("File closed successfully\n");
    return 0;
}