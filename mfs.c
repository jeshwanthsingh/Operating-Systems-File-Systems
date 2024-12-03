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

int fs_mkdir(const char *pathname, mode_t mode){
    printf("\n---Make Directory---\n");
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
    printf("\n--mkdir-- parent:\n");
    //printf("The current last element is %s, and the index is %d\n", lastElementName, index);

    if (index!=-1){
        printf("Directory name exists\n");
        return -2;
    }

    //Add code for writing the directory to disk and modifying the dirEntry in parent
    int numEntries = 51;
    //printf("Creating new directory in parent %s\n",retParent[0].dirName);
    DirEntry* newDirectory;
    newDirectory = createDir(numEntries, retParent, lastElementName);
    //printf("The new directory is %s at block %d\n",newDirectory[0].name,newDirectory[0].block);

    //finding empty directory entry in parent
    int i;
    for (i = 2; i < numEntries; i++){
        if (retParent[i].isUsed == 0){
            //printf("copying new Directory in index %d of parent\n",i);
            strcpy(retParent[i].name,lastElementName);
            strcpy(retParent[i].dirName,lastElementName);
            //strcpy(newDirectory[0].dirName,lastElementName);
            retParent[i].block = newDirectory[0].block;
            retParent[i].isDirectory = newDirectory[0].isDirectory;
            retParent[i].isUsed = 1;
            retParent[i].size = newDirectory[0].size;
            break;
        }        
    }
    //printf("The new directory is %s and at %d block\n",newDirectory[0].dirName,retParent[i].block);
    //printf("writing directories to disk\n");
    //writeDir(newDirectory);
    writeDir(retParent);
    FreeDir(newDirectory);
    FreeDir(retParent);
    return 0;
}

int fs_rmdir(const char* pathname){
    printf("\n---Remove Directory---\n");
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
    printf("--rmdir--\n");
    //printf("The current last element is %s, and the index is %d\n", lastElementName, index);

    if (index==-1){
        printf("Directory does not exist");
        return -2;
    }

    int numEntries = retParent[index].size/sizeof(DirEntry);
    DirEntry* dirToRemove = loadDirectory(&(retParent[index]));
    for (int i = 2; i < numEntries; i++){
        if (dirToRemove[i].isUsed == 1){
            printf("Directory is not empty");
            return -2;
        }
    }

    int numBlocksToRelease = (dirToRemove[0].size+BLOCK_SIZE-1)/BLOCK_SIZE;
    releaseSpace(dirToRemove[0].block,numBlocksToRelease);
    retParent[index].isUsed = 0;
    writeDir(retParent);
    FreeDir(retParent);
    FreeDir(dirToRemove);
    return 0;
}

int fs_isDir(char* pathname){
    printf("\n---Is Directory---\n");
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
    if (index==-1){
        printf("No such directory or file\n");
    }
    int numEntries = retParent[index].size/sizeof(DirEntry);
    //printf("The directory is in parent %s, at index %d with size %d\n",retParent[index].dirName,index, retParent[index].size);
    DirEntry* dir = loadDirectory(&retParent[index]);
    //printf("The dir is %s and %s\n",dir[0].dirName,dir[0].name);
    return dir[0].isDirectory==1;    
}

int fs_isFile(char* pathname){
    printf("\n---Is File---\n");
    return !fs_isDir(pathname);
}

//Directory iteration functions
fdDir * fs_opendir(const char *pathname){
    printf("\n---Opening Directory---\n");

    if (strcmp(pathname,cwd[0].dirName) == 0){
        fdDir* dirIterator;
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

int fs_closedir(fdDir *dirp){
    printf("\n---Close Directory---\n");	
	dirp->directory = NULL;
	free(dirp->directory);
	free(dirp);
    return 0;
}

struct fs_diriteminfo* fs_readdir(fdDir* dirp) {
    fs_diriteminfo* newInfo = malloc(sizeof(fs_diriteminfo));
    int numEntries = dirp->directory[0].size / sizeof(DirEntry);
    
    if (dirp->dirEntryPosition >= numEntries) {
        free(newInfo);
        return NULL;
    }
    
    if (!dirp->directory[dirp->dirEntryPosition].isUsed) {
        free(newInfo);
        return NULL;
    }
    
    strcpy(newInfo->d_name, dirp->directory[dirp->dirEntryPosition].name);
    newInfo->d_reclen = dirp->directory[dirp->dirEntryPosition].size;
    newInfo->fileType = dirp->directory[dirp->dirEntryPosition].isDirectory;
    
    dirp->dirEntryPosition++;
    return newInfo;
}

int fs_stat(const char *pathname, struct fs_stat *buf){
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
	
	buf->st_size = retParent[index].size;
	buf->st_blksize = vcbPtr->blockSize;
	buf->st_blocks = (retParent[index].size+BLOCK_SIZE-1)/BLOCK_SIZE;;
	return 0; 
}

DirEntry* createDir(int numEntries, DirEntry* parent, char* name){
    
    int bytesNeeded = numEntries * sizeof(DirEntry);
    int blocksNeeded = (bytesNeeded+BLOCK_SIZE-1)/BLOCK_SIZE;
    int actualBytes = blocksNeeded * BLOCK_SIZE;
    //printf("We need %d blocks for the new directory\n",blocksNeeded);
    DirEntry* new = malloc(actualBytes);
    int loc = findFreeBlocks(blocksNeeded);
    //printf("free blocks found from block number %d\n",loc);
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

    writeDir(new);
    return new;
}

int writeDir(DirEntry* dir) {
    if (dir == NULL) {
        printf("Error: NULL directory pointer\n");
        return -1;
    }

    int blocks = (dir[0].size + BLOCK_SIZE - 1) / BLOCK_SIZE;
    int result = LBAwrite(dir, blocks, dir[0].block);
    if (result != blocks) {
        printf("Error: LBAwrite failed. Wrote %d blocks, expected %d\n", result, blocks);
        return -1;
    }

    return 0;
}


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

DirEntry* loadDirectory(DirEntry* entry){
    if (entry == NULL || entry->isDirectory != 1){
        return NULL;
    }

    int blocksNeeded = (entry->size + BLOCK_SIZE -1)/BLOCK_SIZE;
    int bytesNeeded = blocksNeeded*BLOCK_SIZE;

    DirEntry* new = malloc(bytesNeeded);
    //printf("The block for the entry is %d and size of entry is %d and blocks needed are %d\n",entry->block, entry->size, blocksNeeded);
    LBAread(new, blocksNeeded, entry->block);
    //printf("The directory name is %s and %s\n", new[0].name, new[0].dirName);
    return new;
}

int DirEntryUsed(DirEntry* entry){

    if (entry->isUsed == 1){
        return 0;
    }
    else{
        return 1;
    }

    
}


//Misc directory functions
char* fs_getcwd(char* pathname, size_t size){
    printf("\n---Get Current Working Directory---\n");

    strcpy(pathname,cwd->dirName);
    return pathname;
}

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

int fs_delete(char* filename){
    printf("The function is not defined yet\n");
    return 0;
}