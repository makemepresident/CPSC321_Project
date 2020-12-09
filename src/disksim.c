#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "shell.h"
#include "main.h"

char disk_drive[MAX_BLOCKS * BLOCK_SIZE];
int bitmap[2] = {128, 256};
int inodes[2] = {256, 131072};
int dbg[2] = {131072, 1310720};

// [0 - 128, 128 - 256, 256 - 131072, data groups ........]

int main() {
    
    char* t = "hello";
    disk_write(t, 25);
    disk_read(25, 50);

    return 0;
}

void startCFFS() {}

char* disk_read(int start_index, int end_index) {

    int diff = end_index - start_index;
    char* temp = malloc(sizeof(char) * diff);
    for(int i = 0; i < diff; i++)
        temp[i] = disk_drive[start_index + i];
    return temp;
}

void disk_write(char* content, int start_index) {

    for(int i = 0; i < strlen(content); i++)
        disk_drive[start_index + i] = content[i];
}

void partition() {
    // dd[0] = superblock
    // dd[1] = inode bitmap
    // inode end = max_blocks * .1
    // dd[2 to inode end] = inode blocks
    //
    disk_drive[0] = FS_ID;
    disk_drive[4] = MAX_BLOCKS_STR;
    disk_drive[8] = NUM_INODES;

    // initialize inode bitmap
    for(int i = 128; i < 128 * 2; i++)
        disk_drive[i] = '\0';

    // final index of less than 

}

void make_file() {
    // go through bitmap, find first empty inode
    // go to that inode
    // and we're like bruh, this is how it do
    int inode;
    for(int i = bitmap[0]; i < bitmap[1]; i++)
        for(int j = 0; j < 8; j++)
            if(disk_drive[i] >> j & 0) {
                inode = BLOCK_SIZE * i + j;
                disk_drive[i] = disk_drive[i] >> j | 1;
            }
    disk_drive[inode] = '\0'; // explicit, but already done
    // [sb, inbm, inode1...inode1024, dbg1...dbg1024]
    // dbg = 1 bitmap block + 1024 data blocks
    

    // go to first dbg bitmap
    // find first free block, if first to first + len(string) is free
    // write string to that section of memory
    // record in list? how about notepad
    // for(int i = dbg[0]; i < dbg[1]; i += GROUP_BM_INC)
    //     for(int j = i; j < 128; j++)
    //         for(int k = 0; k < 8; k++)
    //             if(disk_drive[j] >> k & 0)
    //                 for

}

char* getInput() {
    char* input = NULL;
    size_t ad = 0;
    getline(&input, &ad, stdin);
    input[strlen(input) - 1] = '\0';
    return input;
}

int parseInput(char* string, char** args) {
    if(strcspn(string, DELIM) == strlen(string)) {
        strcpy(*args, string);
        return 1;
    }
    char* arg = strtok(string, DELIM);
    int count = 0;
    while(arg != NULL) {
        args[count++] = arg;
        arg = strtok(NULL, DELIM);
    }
    args[count] = NULL;
    return count + 1;
}