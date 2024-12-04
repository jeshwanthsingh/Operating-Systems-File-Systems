/**************************************************************
* Class::  CSC-415-0# Spring 2024
* Name::
* Student IDs::
* GitHub-Name::
* Group-Name::
* Project:: Basic File System
*
* File:: mfs.h
*
* Description:: 
*	This is the file system interface.
*	This is the interface needed by the driver to interact with
*	your filesystem.
*
**************************************************************/

#include "mfs.h"
#include "parsePath.c"
#include "fsFreespace.h"
#include "fsLow.h"

#define DIR_BLOCKS 6

/**
 * fs_mkdir
 * Creates a new directory with the specified path.
 * 
 * Parameters:
 * - pathname: The full path to the directory to be created.
 * - mode: Mode specifying directory permissions (unused in this implementation).
 * 
 * Returns:
 * - 0 on successful directory creation.
 * - -1 if path parsing fails.
 * - -2 if a directory with the same name already exists.
 */
int fs_mkdir(const char *pathname, mode_t mode){    //make directory
    printf("\n---Make Directory---\n");
    int pathLength = strlen(pathname);      //length of path
    char path[pathLength];      //path
    strcpy(path,pathname);      //copy path to path
    DirEntry* retParent;        //parent directory
    retParent = malloc(DIR_BLOCKS*BLOCK_SIZE);  //parent directory
    int index;
    char* lastElementName;
    if (parsePath(path,&retParent,&index,&lastElementName)==-1){    //error parsing path
        printf("error parsing path\n");
        return -1;
    }
    printf("\n--mkdir-- parent:\n");
    //printf("The current last element is %s, and the index is %d\n", lastElementName, index);

    if (index!=-1){
        printf("Directory name exists\n");
        return -2;
    }

    //create new directory
    int numEntries = 51;
    printf("Creating new directory in parent %s at block %d\n",retParent[0].dirName, retParent[0].block);
    DirEntry* newDirectory; //new directory
    newDirectory = createDir(numEntries, retParent, lastElementName);   //create directory
    printf("The new directory is %s at block %d\n",newDirectory[0].name,newDirectory[0].block);

    //finding empty directory entry in parent
    int i;
    for (i = 2; i < numEntries; i++){   //find empty directory entry
        if (retParent[i].isUsed == 0){  //is used
            printf("copying new Directory in index %d of parent\n",i);
            strcpy(retParent[i].name,lastElementName);  //remove last element
            strcpy(retParent[i].dirName,lastElementName);   //remove last element
            //strcpy(newDirectory[0].dirName,lastElementName);
            retParent[i].block = newDirectory[0].block;
            retParent[i].isDirectory = newDirectory[0].isDirectory;
            retParent[i].isUsed = 1;
            retParent[i].size = newDirectory[0].size;
            break;
        }        
    }
    printf("The new directory is %s and at %d block\n",newDirectory[0].dirName,retParent[i].block);
    //printf("writing directories to disk\n");
    //writeDir(newDirectory);
    if (writeDir(retParent)!=0){
        printf("Write Directory failed");
    }
    FreeDir(newDirectory);
    FreeDir(retParent);
    return 0;
}
/**
 * fs_rmdir
 * Removes a directory with the specified path.
 * 
 * Parameters:
 * - pathname: The full path to the directory to be removed.
 * 
 * Returns:
 * - 0 on successful removal.
 * - -1 if path parsing fails.
 * - -2 if the directory does not exist or is not empty.
 */
int fs_rmdir(const char* pathname){ //remove directory
    printf("\n---Remove Directory---\n");
    int pathLength = strlen(pathname);  //length of path
    char path[pathLength];
    strcpy(path,pathname);
    DirEntry* retParent;
    retParent = malloc(DIR_BLOCKS*BLOCK_SIZE);
    int index;
    char* lastElementName;
    if (parsePath(path,&retParent,&index,&lastElementName)==-1){
        printf("error parsing path\n");
        return -1;
    }
    printf("--rmdir--\n");
    //printf("The current last element is %s, and the index is %d\n", lastElementName, index);

    if (index==-1){
        printf("Directory does not exist\n");
        return -2;
    }

    int numEntries = retParent[index].size/sizeof(DirEntry);    //number of entries
    DirEntry* dirToRemove = loadDirectory(&(retParent[index])); //directory to remove
    for (int i = 2; i < numEntries; i++){   //remove directory
        if (dirToRemove[i].isUsed == 1){    //directory is used
            printf("Directory is not empty");
            return -2;
        }
    }

    int numBlocksToRelease = (dirToRemove[0].size+BLOCK_SIZE-1)/BLOCK_SIZE; //number of blocks to release
    releaseSpace(dirToRemove[0].block,numBlocksToRelease);  //release space
    retParent[index].isUsed = 0;    //not used
    if (writeDir(retParent)!=0){    //write directory
        printf("Write Directory failed");
    }
    FreeDir(retParent);
    FreeDir(dirToRemove);
    return 0;
}
/**
 * fs_isDir
 * Checks whether the specified path points to a directory.
 * 
 * Parameters:
 * - pathname: The path to be checked.
 * 
 * Returns:
 * - 1 if the path is a directory.
 * - 0 if the path is not a directory.
 * - -1 if path parsing fails or the directory does not exist.
 */
int fs_isDir(char* pathname){       //is directory
    printf("\n---Is Directory---\n");
    int pathLength = strlen(pathname);  //length of path
    char path[pathLength];
    strcpy(path,pathname);
    DirEntry* retParent;
    retParent = malloc(DIR_BLOCKS*BLOCK_SIZE);
    int index;
    char* lastElementName;
    if (parsePath(path,&retParent,&index,&lastElementName)==-1){ //error parsing path
        printf("error parsing path\n");
        return -1;
    }
    if (index==-1){
        printf("No such directory or file\n");
        return -1;
    }
    int numEntries = retParent[index].size/sizeof(DirEntry);
    //printf("The directory is in parent %s, at index %d with size %d\n",retParent[index].dirName,index, retParent[index].size);
    //DirEntry* dir = loadDirectory(&retParent[index]);
    //printf("The dir is %s and %s\n",dir[0].dirName,dir[0].name);
    return retParent[index].isDirectory==1;    
}
/**
 * fs_isFile
 * Checks whether the specified path points to a file.
 * 
 * Parameters:
 * - pathname: The path to be checked.
 * 
 * Returns:
 * - 1 if the path is a file.
 * - 0 otherwise.
 */
int fs_isFile(char* pathname){ // returns 1 if file, 0 otherwise
    printf("\n---Is File---\n");
    return !fs_isDir(pathname);
}
/**
 * fs_opendir
 * Opens a directory stream for the specified path.
 * 
 * Parameters:
 * - pathname: The full path to the directory to be opened.
 * 
 * Returns:
 * - A pointer to an fdDir structure representing the directory stream.
 * - NULL if the directory does not exist or path parsing fails.
 */
//Directory iteration functions
fdDir * fs_opendir(const char *pathname){   // returns a pointer to the directory stream
    printf("\n---Opening Directory---\n");

    if (strcmp(pathname,cwd[0].dirName) == 0){  // directory is current working directory
        fdDir* dirIterator;     // iterator
        dirIterator = malloc(sizeof(fdDir));
        dirIterator->directory = cwd;
        dirIterator->d_reclen = cwd[0].size;
        dirIterator->dirEntryPosition = 2;
        dirIterator->startingBlock = cwd[0].block;
        return dirIterator; 
    }
    int pathLength = strlen(pathname);
    char path[pathLength];
    strcpy(path,pathname);
    DirEntry* retParent;
    retParent = malloc(DIR_BLOCKS*BLOCK_SIZE);
    int index;
    char* lastElementName;
    if (parsePath(path,&retParent,&index,&lastElementName)==-1){
        printf("error parsing path\n");
        return NULL;
    }
    if (index==-1){
        printf("No such directory exists\n");
        return NULL;
    }
    fdDir* dirIterator;
    dirIterator = malloc(sizeof(fdDir));
    dirIterator->directory = loadDirectory(&(retParent[index]));
    dirIterator->d_reclen = dirIterator->directory[0].size;
    dirIterator->dirEntryPosition = 2;
    dirIterator->startingBlock = dirIterator->directory[0].block;
    return dirIterator; 
}
/**
 * fs_closedir
 * Closes an open directory stream.
 * 
 * Parameters:
 * - dirp: A pointer to the fdDir structure representing the directory stream.
 * 
 * Returns:
 * - 0 on success.
 */
int fs_closedir(fdDir *dirp){   //closedir
    printf("\n---Close Directory---\n");	
	dirp->directory = NULL;
	FreeDir(dirp->directory);
	free(dirp);
    return 0;
}
/**
 * fs_readdir
 * Reads the next directory entry from an open directory stream.
 * 
 * Parameters:
 * - dirp: A pointer to the fdDir structure representing the directory stream.
 * 
 * Returns:
 * - A pointer to an fs_diriteminfo structure containing information about the next entry.
 * - NULL if there are no more entries or an error occurs.
 */
struct fs_diriteminfo* fs_readdir(fdDir* dirp) {    //readdir
    fs_diriteminfo* newInfo = malloc(sizeof(fs_diriteminfo));   //newInfo
    int numEntries = dirp->directory[0].size / sizeof(DirEntry);    //numEntries
    //printf("The number of entries in the directory is %d\n",numEntries);
    while (dirp->dirEntryPosition < numEntries && !dirp->directory[dirp->dirEntryPosition].isUsed) {
        dirp->dirEntryPosition = dirp->dirEntryPosition+1; //increment
    }

    if (dirp->dirEntryPosition >= numEntries) { //directory position is greater than numEntries
        newInfo = NULL; //create new info
        free(newInfo);  //delete new info
        return NULL;
    }
    
    strcpy(newInfo->d_name, dirp->directory[dirp->dirEntryPosition].name);  //copy name
    newInfo->d_reclen = dirp->directory[dirp->dirEntryPosition].size;   //copy size
    newInfo->fileType = dirp->directory[dirp->dirEntryPosition].isDirectory; //copy file type
    
    dirp->dirEntryPosition = dirp->dirEntryPosition+1; //increment
    return newInfo;
}
/**
 * fs_stat
 * Retrieves information about a file or directory at the specified path.
 * 
 * Parameters:
 * - pathname: The path to the file or directory.
 * - buf: A pointer to an fs_stat structure to store the retrieved information.
 * 
 * Returns:
 * - 0 on success.
 * - -1 if path parsing fails or the file/directory does not exist.
 */
//File System Interface
int fs_stat(const char *pathname, struct fs_stat *buf){ //stat
	printf("\n---Stat---\n");
	int pathLength = strlen(pathname);
    char path[pathLength];
    strcpy(path,pathname);
    DirEntry* retParent;
    retParent = malloc(DIR_BLOCKS*BLOCK_SIZE);
    int index;
    char* lastElementName;
    if (parsePath(path,&retParent,&index,&lastElementName)==-1){
        printf("error parsing path\n");
        return -1;
    }
	if (index == -1){
        printf("No such file or directory found\n");
    }
	buf->st_size = retParent[index].size;
	buf->st_blksize = BLOCK_SIZE;
	buf->st_blocks = (retParent[index].size+BLOCK_SIZE-1)/BLOCK_SIZE;;
	return 0; 
}
/**
 * createDir
 * Allocates and initializes a new directory structure.
 * 
 * Parameters:
 * - numEntries: The maximum number of directory entries in the new directory.
 * - parent: A pointer to the parent directory's DirEntry structure.
 * - name: The name of the new directory to be created.
 * 
 * Returns:
 * - A pointer to the newly created directory structure.
 * 
 * Behavior:
 * - Allocates the required space for the new directory.
 * - Initializes special entries "." (self) and ".." (parent).
 * - Writes the new directory structure to disk using `writeDir`.
 * 
 * Notes:
 * - Uses `findFreeBlocks` to locate free disk space for the new directory.
 */
DirEntry* createDir(int numEntries, DirEntry* parent, char* name){
    
    int bytesNeeded = numEntries * sizeof(DirEntry);
    int blocksNeeded = (bytesNeeded+BLOCK_SIZE-1)/BLOCK_SIZE;
    int actualBytes = blocksNeeded * BLOCK_SIZE;
    printf("We need %d blocks for the new directory\n",blocksNeeded);
    DirEntry* new = malloc(actualBytes);
    int loc = findFreeBlocks(blocksNeeded);
    printf("free blocks found from block number %d\n",loc);
    int actualEntries = actualBytes/sizeof(DirEntry);

    //Initialize
    for (int i = 2; i < actualEntries; i++){
        new[i].isUsed = 0;
    }

    //self init
    strcpy(new[0].name,".");
    strcpy(new[0].dirName,name);
    new[0].block = loc;
    new[0].isDirectory = 1;
    new[0].size = actualEntries*sizeof(DirEntry);
    new[0].isUsed = 1;

    //parent init
    strcpy(new[1].dirName, parent[0].dirName);
    strcpy(new[1].name,"..");
    new[1].block = parent[0].block;
    new[1].size = parent[0].size;
    new[1].isDirectory = parent[0].isDirectory;
    new[1].isUsed = 1;

    if (writeDir(new)!=0){
        printf("Write Directory failed");
    };
    return new;
}
/**
 * writeDir
 * Writes a directory's data to disk.
 * 
 * Parameters:
 * - dir: A pointer to the directory structure to be written.
 * 
 * Returns:
 * - 0 on success.
 * - -1 if the write operation fails or if `dir` is NULL.
 * 
 * Behavior:
 * - Calculates the number of blocks needed to store the directory.
 * - Uses `LBAwrite` to write the directory to the disk.
 * 
 * Notes:
 * - Validates the input and ensures that the write operation is successful.
 */
int writeDir(DirEntry* dir) {
    if (dir == NULL) {
        printf("Error: NULL directory pointer\n");
        return -1;
    }

    int blocks = (dir[0].size + BLOCK_SIZE - 1) / BLOCK_SIZE;
    if (dir[0].isDirectory==0){
        blocks = FILE_ENTRY_BLOCKS;
    }
    int result = LBAwrite(dir, blocks, dir[0].block);
    if (result != blocks) {
        printf("Error: LBAwrite failed. Wrote %d blocks, expected %d\n", result, blocks);
        return -1;
    }

    return 0;
}

/**
 * FreeDir
 * Frees the memory allocated for a directory structure.
 * 
 * Parameters:
 * - dir: A pointer to the directory structure to be freed.
 * 
 * Behavior:
 * - Ensures that critical directories (e.g., root, cwd) are not freed.
 * - Frees the memory allocated for the directory.
 * 
 * Notes:
 * - Safeguards against double freeing or freeing important directory structures.
 */
void FreeDir(DirEntry* dir){
    if (dir == NULL){
        return;
    }
    if (dir == root){
        return;
    }
    if (dir == cwd){
        return;
    }
    free(dir);
}
/**
 * FindInDirectory
 * Searches for a file or subdirectory in a given directory.
 * 
 * Parameters:
 * - dir: A pointer to the directory structure to search in.
 * - name: The name of the file or subdirectory to search for.
 * 
 * Returns:
 * - The index of the found entry if it exists.
 * - -1 if the entry is not found.
 * - -2 if the input parameters are invalid.
 * 
 * Behavior:
 * - Iterates through the directory's entries to locate the specified name.
 * - Checks whether each entry is in use and matches the provided name.
 */
int FindInDirectory(DirEntry* dir, char* name){
    if (dir == NULL || name == NULL) {
        return -2;
    }

    int numEntries = dir[0].size/sizeof(DirEntry);

    for (int i = 0; i < numEntries; i++){
        
        if (dir[i].isUsed==1){
            if (strcmp(dir[i].name,name)==0){
                return i;
            }
        }
    }
    return -1;
}
/**
 * loadDirectory
 * Loads a directory from disk into memory.
 * 
 * Parameters:
 * - entry: A pointer to the DirEntry structure representing the directory to be loaded.
 * 
 * Returns:
 * - A pointer to the loaded directory structure.
 * - NULL if the entry is invalid or not a directory.
 * 
 * Behavior:
 * - Calculates the number of blocks needed for the directory.
 * - Reads the directory data from disk using `LBAread`.
 * 
 * Notes:
 * - Ensures the entry represents a directory before attempting to load it.
 */
DirEntry* loadDirectory(DirEntry* entry){
    if (entry == NULL || entry->isDirectory != 1){
        return NULL;
    }

    int blocksNeeded = (entry->size + BLOCK_SIZE -1)/BLOCK_SIZE;
    if (entry->isDirectory==0){
        blocksNeeded = FILE_ENTRY_BLOCKS;
    }
    int bytesNeeded = blocksNeeded*BLOCK_SIZE;

    DirEntry* new = malloc(bytesNeeded);
    //printf("The block for the entry is %d and size of entry is %d and blocks needed are %d\n",entry->block, entry->size, blocksNeeded);
    LBAread(new, blocksNeeded, entry->block);
    //printf("The directory name is %s and %s\n", new[0].name, new[0].dirName);
    return new;
}
/**
 * DirEntryUsed
 * Checks whether a directory entry is in use.
 * 
 * Parameters:
 * - entry: A pointer to the DirEntry structure to check.
 * 
 * Returns:
 * - 0 if the entry is in use.
 * - 1 if the entry is not in use.
 */
int DirEntryUsed(DirEntry* entry){

    if (entry->isUsed == 1){
        return 0;
    }
    else{
        return 1;
    }

    
}

/**
 * fs_getcwd
 * Retrieves the current working directory's path.
 * 
 * Parameters:
 * - pathname: A buffer to store the current working directory's path.
 * - size: The size of the buffer.
 * 
 * Returns:
 * - A pointer to the pathname buffer containing the current working directory's path.
 * 
 * Behavior:
 * - Copies the directory name of the current working directory (cwd) into the buffer.
 */
//Misc directory functions
char* fs_getcwd(char* pathname, size_t size){
    printf("\n---Get Current Working Directory---\n");

    strcpy(pathname,cwd->dirName);
    return pathname;
}
/**
 * fs_setcwd
 * Sets the current working directory to the specified path.
 * 
 * Parameters:
 * - pathname: The path to the directory to be set as the current working directory.
 * 
 * Returns:
 * - 0 on success.
 * - -1 if path parsing fails.
 * - -2 if the specified directory does not exist or is not a directory.
 * 
 * Behavior:
 * - Parses the path to locate the specified directory.
 * - Loads the directory into the cwd pointer if valid.
 */
int fs_setcwd(char* pathname){
    printf("\n---Set Current Working Directory---\n");
    int pathLength = strlen(pathname);
    char path[pathLength];
    strcpy(path,pathname);
    DirEntry* retParent;
    retParent = malloc(DIR_BLOCKS*BLOCK_SIZE);
    int index;
    char* lastElementName;
    if (parsePath(path,&retParent,&index,&lastElementName)==-1){
        printf("error parsing path\n");
        return -1;
    }
    if (index==-1 || retParent[index].isDirectory!=1){
        printf("No such directory exists\n");
        return -2;
    }
    //DirEntry* newCwd;
    cwd = loadDirectory(&(retParent[index]));
    //cwd = newCwd;
    return 0;
}
/**
 * fs_delete
 * Deletes a file with the specified name.
 * 
 * Parameters:
 * - filename: The full path to the file to be deleted.
 * 
 * Returns:
 * - 0 on success.
 * - -1 if path parsing fails.
 * - -2 if the file does not exist.
 * 
 * Behavior:
 * - Locates the file's entry in the parent directory.
 * - Releases the space occupied by the file using `releaseSpace`.
 * - Marks the directory entry as unused and writes the updated directory back to disk.
 */
int fs_delete(char* filename){
    printf("\n---Remove File---\n");
    int pathLength = strlen(filename);
    char path[pathLength];
    strcpy(path,filename);
    DirEntry* retParent;
    retParent = malloc(DIR_BLOCKS*BLOCK_SIZE);
    int index;
    char* lastElementName;
    if (parsePath(path,&retParent,&index,&lastElementName)==-1){
        printf("error parsing path\n");
        return -1;
    }
    if (index==-1){
        printf("No such file exists");
        return -2;
    }
    DirEntry fileToRemove = retParent[index];
    int numBlocksToRelease = FILE_ENTRY_BLOCKS;
    printf("The size of the file is %d\n",numBlocksToRelease);
    releaseSpace(fileToRemove.block,numBlocksToRelease);
    retParent[index].isUsed = 0;
    if(writeDir(retParent)!=0){
        printf("writing directory failed");
    }
    FreeDir(retParent);
    return 0;
}