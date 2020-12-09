#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "shell.h"
#include "main.h"

typedef struct Block {
    unsigned char bytes[BLOCK_SIZE];
    int cptr;
} Block;

typedef struct CFile {
    char* filename;
    int inode_index;
} CFile;

Block disk_drive[MAX_BLOCKS];
CFile files[MAX_FILES];
int len_files = 0;
int superblock_index = 0;
int inode_bitmap_index = 128;
int inodes_index = 131072;
int db_groups_index = 1310720;

// [0 - 128, 128 - 256, 256 - 131072, data groups ........]

int main() {

    make_file("bigfile");
    printf("%s\n", disk_read(2));

    return 0;
}

void startCFFS() {}

char* disk_read(int block) {

    if(block < 0 || block > MAX_BLOCKS)
        return NULL;

    char* temp = malloc(sizeof(char) * BLOCK_SIZE);
    for(int i = 0; i < BLOCK_SIZE; i++)
        temp[i] = disk_drive[block].bytes[i];
    return temp;
}

void disk_write(char* content, int block) {

    if(block < 0 || block > MAX_BLOCKS)
        return;
    
    int pointer = disk_drive[block].cptr;
    for(int i = 0; i < strlen(content); i++, disk_drive[block].cptr++)
        disk_drive[block].bytes[pointer + i] = content[i];
}

void partition() {

    init_superblock();
    init_inbm();
    // init_inodes();
}

void init_superblock() {

    disk_write(FS_ID, 0);
    disk_write(TOTAL_BLOCKS, 0);
    disk_write(TOTAL_INODES, 0);
}

void init_inbm() {
    
    Block inbm = disk_drive[1];
    for(int i = 0; i < sizeof(inbm.bytes) / sizeof(char); i++)
        inbm.bytes[i] = '\0';
}

void make_file(char* name) {
    
    CFile current = files[len_files++];
    current.filename = name;
    // Look through inode bitmap and find first available inode
    Block inbm = disk_drive[1];
    int flag = 0;
    for(int i = 0; i < sizeof(inbm.bytes) / sizeof(char); i++) {
        for(int j = 0; j < 8; j++) {
            if(inbm.bytes[i] >> 1)
                continue;
            inbm.bytes[i] ^= 1 << j;
            current.inode_index = 2 + (i * 8) + j;
            flag = 1;
            break;
        }
        if(flag)
            break;
    }
}

void write_file(char* filename, char* content) {

    CFile current = files[0];
    init_inode(current);
}

void init_inode(CFile current) {

    
}