//#include "mfs.h"
#include "mfs.h"
//#include "fsInit.c"
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include "parsePath.h"
#define NAME_LIMIT 20
#define BLOCK_SIZE 512


int parsePath(char* path, DirEntry** returnParent, int* index, char** lastElementName) {

    //printf("the path is %s\n",path);
    if (path == NULL) {
        return -1;
    }

    if (strlen(path)==0) {
        return -1;
    }

    DirEntry* start;
    DirEntry* parent;

    if (path[0]=='/'){
        start = root;
        //printf("the start is root\n");
    }
    else{
        start = cwd;
        //printf("the start is cwd\n");
    }

    parent = start;
    char* token1;
    char* token2;
    char* savePtr;

    token1 = strtok_r(path,"/",&savePtr);
    //printf("The first token is %s\n",token1);

    if (token1 == NULL){
        *returnParent = parent;
        *lastElementName = NULL;
        *index = 0;
        return 0;
    }

    while (1) {
        token2 = strtok_r(NULL,"/",&savePtr);
        //printf("The second token is %s\n",token2);
        //printf("The current parent is %s\n",parent[0].dirName);
        int idx = FindInDirectory(parent,token1);
        //printf("The index is %d\n",idx);
        if (token2 == NULL){
            *returnParent = parent;
            *index = idx;
            *lastElementName = token1;
            return 0;
        }

        if(idx==-1){
            return -1;
        }
        if(parent[idx].isDirectory != 1){
            return -1;
        }
        DirEntry* newParent = loadDirectory(&(parent[idx]));
        FreeDir(parent);
        parent = newParent;
        token1 = token2;
    }    
}