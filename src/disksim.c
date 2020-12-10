#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "shell.h"
#include "main.h"

Block disk_drive[MAX_BLOCKS];
CFile files[MAX_FILES];
int len_files = 0;
int superblock_index = 0;
int inode_bitmap_index = 128;
int inodes_index = 131072;
int db_groups_index = 1310720;

int main() {

    make_file("bigfile");
    // make_file("smallfile");
    write_file("bigfile", "hello");
    // printf("%s\n", disk_read(1025));

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
    disk_drive[block].cptr++; // so doesn't overwrite last char
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
            if(inbm.bytes[i] >> j) // 00001111
                continue;
            inbm.bytes[i] ^= 1 << j;
            current.inode_index = 3 + (i * 8) + j;
            flag = 1;
            break;
        }
        if(flag)
            break;
    }
}

void write_file(char* filename, char* content) {

    CFile current;
    for(int i = 0; i < MAX_FILES; i++)
        if(strcmp(files[i].filename, filename) == 0) {
            current = files[i];
            break;
        }
    
    if(current.filename == NULL) {
        printf("Unable to find file, please try again");
        return;
    }

    Block inode = init_inode(current);
    if(strlen(content) < 124) {
        int db_index = inode.bytes[0] << 24 | inode.bytes[1] << 16 | inode.bytes[2] << 8 | inode.bytes[3];
        disk_write(content, db_index);
    }
}

Block init_inode(CFile current) {

    Block inode = disk_drive[current.inode_index];
    int dbg_bitmap[2] = {1025, 1024}; // starting index, increment value
    // Iterate over all data block groups
    // Find data block group with 5 contiguous blocks
    int flag = 0;
    Block dbg;
    int db_index;
    for(int i = dbg_bitmap[0]; i < MAX_BLOCKS; i += dbg_bitmap[1]) {
        dbg = disk_drive[i]; // data group BLOCK struct
        for(int j = 0; j < sizeof(dbg.bytes) / sizeof(char); j++) { // per byte of block
            for(int k = 0; k < 2; k++) { // per bit of byte (cop out as can't find way to overflow to next byte)
                if(dbg.bytes[j] >> k)
                    continue;
                    for(int l = 1; l < 5; l++) { // check if next 4 bits are 0
                        if(dbg.bytes[j] >> k + l)
                            break;
                        for(int m = k; m < 4; m++) { // needs new for loop
                            db_index = i + (j * 8) + k + m; // data block index
                            inode = ins_int(inode, db_index, m * 4);
                            dbg.bytes[i] ^= 1 << k;
                            flag = 1;
                        }
                        if(flag)
                            goto jump;
                    }
            }
        }
    }
    jump:
        disk_drive[current.inode_index] = inode;
        printf("Didn't explode?\n");
    return inode;
}

// taken from https://stackoverflow.com/questions/3784263/converting-an-int-into-a-4-byte-char-array-c
Block ins_int(Block inode, int db_index, int start_index) {

    // 10000000 00001000 00000010 00000000
    inode.bytes[start_index] = (db_index >> 24) & 0xFF; // 10000000
    inode.bytes[start_index + 1] = (db_index >> 16) & 0xFF;
    inode.bytes[start_index + 2] = (db_index >> 8) & 0xFF;
    inode.bytes[start_index + 3] = db_index & 0xFF;

    return inode;
}