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


//#include <stdio.h>
/**
 * parsePath
 * Parses a given file path and determines its parent directory, the index of the target
 * entry within the parent directory, and the name of the last component in the path.
 * 
 * Parameters:
 * - path: The file path to be parsed (modifies this string internally).
 * - returnParent: A pointer to store the parent directory of the target path.
 * - index: A pointer to store the index of the target entry within the parent directory.
 * - lastElementName: A pointer to store the name of the last component in the path.
 * 
 * Returns:
 * - 0 on successful parsing.
 * - -1 if the path is invalid, a directory is not found, or a non-directory is encountered in the path.
 * 
 * Behavior:
 * - Starts parsing from the root directory if the path begins with `/`.
 * - Otherwise, starts parsing from the current working directory (`cwd`).
 * - Iterates through each component in the path using `/` as a delimiter.
 * - Checks for the existence and validity of each component in the path.
 * - Handles scenarios where the path ends with a file or directory.
 * 
 * Logging Levels (controlled by `LOG_LEVEL`):
 * - [ERROR]: Logs errors such as invalid paths or missing directories.
 * - [INFO]: Logs high-level information about the parsing process.
 * - [DEBUG]: Logs detailed information, including token processing and directory transitions.
 * 
 * Notes:
 * - Uses `FindInDirectory` to locate components in the directory.
 * - Frees the memory of directories that are no longer in use.
 * - Modifies the input `path` string internally, as it tokenizes the string.
 */
// Logging macros for better control
#define LOG_LEVEL 1 // 0 - No Logs, 1 - Errors only, 2 - Info, 3 - Debug

#define LOG_ERROR(fmt, ...) if (LOG_LEVEL >= 1) printf("[ERROR] " fmt "\n", ##__VA_ARGS__)
#define LOG_INFO(fmt, ...) if (LOG_LEVEL >= 2) printf("[INFO] " fmt "\n", ##__VA_ARGS__)
#define LOG_DEBUG(fmt, ...) if (LOG_LEVEL >= 3) printf("[DEBUG] " fmt "\n", ##__VA_ARGS__)

int parsePath(char* path, DirEntry** returnParent, int* index, char** lastElementName) {
    LOG_DEBUG("--- Parsing Path: %s ---", path);
    
    if (path == NULL || strlen(path) == 0) {
        LOG_ERROR("Invalid path");
        return -1;
    }

    // Determine starting directory (root or cwd)
    DirEntry* current;
    if (path[0] == '/') {
        current = root;
        LOG_INFO("Starting from root directory (block: %d)", root[0].block);
    } else {
        current = cwd;
        LOG_INFO("Starting from current directory (block: %d)", cwd[0].block);
    }

    LOG_DEBUG("Starting directory: %s (block: %d, size: %d)", 
              current[0].dirName, current[0].block, current[0].size);

    // Tokenize path
    char* savePtr;
    char* token;
    char* nextToken;
    token = strtok_r(path, "/", &savePtr);
    
    // If no token (e.g., "/" path)
    if (token == NULL) {
        *returnParent = current;
        *lastElementName = NULL;
        *index = 0;
        return 0;
    }

    // Process path components
    while (1) {
        nextToken = strtok_r(NULL, "/", &savePtr);
        LOG_DEBUG("Processing token: %s, Next token: %s", 
                  token, nextToken ? nextToken : "NULL");

        // Find current token in directory
        int currentIndex = FindInDirectory(current, token);
        LOG_DEBUG("Found at index: %d", currentIndex);

        if (nextToken == NULL) {
            // This is the last component
            *returnParent = current;
            *index = currentIndex;
            *lastElementName = token;
            LOG_INFO("Path parsing complete - Parent: %s, Index: %d, Last Element: %s", 
                     current[0].dirName, currentIndex, token);
            return 0;
        }

        // Not last component - must be a directory
        if (currentIndex == -1) {
            LOG_ERROR("Directory not found: %s", token);
            return -1;
        }

        if (current[currentIndex].isDirectory != 1) {
            LOG_ERROR("%s is not a directory", token);
            return -1;
        }

        // Load next directory
        DirEntry* next = loadDirectory(&(current[currentIndex]));
        if (next == NULL) {
            LOG_ERROR("Failed to load directory: %s", token);
            return -1;
        }

        // Free current if it's not root or cwd
        FreeDir(current);
        current = next;
        token = nextToken;

        LOG_INFO("Moved to directory: %s (block: %d)", 
                 current[0].dirName, current[0].block);
    }
}