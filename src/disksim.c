#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

#include "shell.h"
#include "main.h"

Block disk_drive[MAX_BLOCKS];
CFile files[MAX_FILES];
int len_files = 0;

int main() {

    return 0;
}

void startCFFS() {}

/**
 * Raw read to the CFFS disk based on the index of a block. Contains
 * handling to ensure block is within space.
 * 
 * @param block block index
 * 
 * @return string content in entire block
 */
char* disk_read(int block) {

    if(block < 0 || block > MAX_BLOCKS)
        return NULL;

    char* temp = malloc(sizeof(char) * BLOCK_SIZE);
    for(int i = 0; i < BLOCK_SIZE; i++)
        temp[i] = disk_drive[block].bytes[i];

    return temp;
}

/**
 * Raw write to the CFFS disk based on a given string of content and
 * a block index. Contains handling to ensure we write within the
 * bounds of the disk.
 * 
 * Contains a function that gets executed from a specific string in
 * delete_file() that is used to reset the data block.
 * 
 * @param content string containing content to write to the block
 * @param block index of block to write to
 */
void disk_write(char* content, int block) {

    if(block < 0 || block > MAX_BLOCKS)
        return;

    // Function for resetting data block char[] called from delete_file()
    if(strcmp(content, "{0}")) {
        for(int i = 0; i < BLOCK_SIZE; i++)
            disk_drive[block].bytes[i] = 0x00;
        return;
    }
    
    int pointer = disk_drive[block].cptr;
    for(int i = 0; i < strlen(content); i++, disk_drive[block].cptr++)
        disk_drive[block].bytes[pointer + i] = content[i];
}

/**
 * Partition driver function for correctness
 */
void partition() {

    init_superblock();
    init_inbm();
}

/**
 * Superblock generator in which we define index 0 of the disk as the superblock
 */
void init_superblock() {

    disk_write(FS_ID, 0);
    disk_write(TOTAL_BLOCKS, 0);
    disk_write(TOTAL_INODES, 0);
}

/**
 * Initializes the inode bitmap such that each character is 0x00
 */
void init_inbm() {
    
    Block inbm = disk_drive[1];
    for(int i = 0; i < sizeof(inbm.bytes) / sizeof(char); i++)
        inbm.bytes[i] = '\0';
}

/**
 * Responsible for adding the file to the list of files currently in the
 * file system as well as allocating an inode to the file. Consequently,
 * this function calls init_inode().
 * 
 * @param name input filename from shell input
 */
void make_file(char* name) {
    
    CFile current = files[len_files];
    current.filename = name;
    Block inbm = disk_drive[1];
    int flag = 0;
    for(int i = 0; i < sizeof(inbm.bytes) / sizeof(char); i++) {
        for(int j = 0; j < 8; j++) {
            if(inbm.bytes[i] >> j)
                continue;
            inbm.bytes[i] ^= 1 << j;
            current.inode_index = 3 + (i * 8) + j;
            flag = 1;
            break;
        }
        if(flag)
            break;
    }
    init_inode(current);
    files[len_files++] = current;
}

/**
 * Responsible for writing content to the specified file based on a name. This
 * function receives data block information from the inode pointed at by the
 * corresponding CFile
 * 
 * @param filename shell input filename (must correspond to an existing make_file())
 * @param content string content to write to the file
 */ 
void write_file(char* filename, char* content) {

    CFile current;
    int caught = 0;
    for(int i = 0; i < len_files; i++)
        if(strcmp(files[i].filename, filename) == 0) {
            current = files[i];
            caught = 1;
        }
    
    if(!caught) {
        printf("Unable to find file, please try again\n");
        return;
    }

    Block inode = disk_drive[current.inode_index];
    if(strlen(content) < 124) {
        // https://stackoverflow.com/questions/5784605/converting-hex-value-in-char-array-to-an-integer-value
        int db_index = inode.bytes[0] << 24 | inode.bytes[1] << 16 | inode.bytes[2] << 8 | inode.bytes[3];
        disk_write(content, db_index);
    } else if(strlen(content) < 124 * 2) {
        // write to first block for all up to 124
        // write rest to second block
    } // continue else statements, no indirect pointer for now :(

    printf("File write was successful!\n");
}

/**
 * WIP might not have time :(
 */
void delete_file(char* filename) {

    CFile current;
    int del_index;
    int caught = 0;
    for(int i = 0; i < len_files; i++)
        if(strcmp(files[i].filename, filename) == 0) {
            current = files[i];
            caught = 1;
            del_index = i;
        }
    
    if(!caught) {
        printf("Unable to find file, please try again\n");
        return;
    }

    Block inode = disk_drive[current.inode_index];
    int db_index = inode.bytes[0] << 24 | inode.bytes[1] << 16 | inode.bytes[2] << 8 | inode.bytes[3];
    disk_write("{0}", db_index);
    disk_write("{0}", current.inode_index);
    files[del_index].filename = 0x00;
    files[del_index].inode_index = 0;
}

/**
 * My brain hurts. Detailed steps line by line.
 * 
 * Function responsible for assigning an inode to a new file. Furthermore,
 * function is responsible for iterating over each data block group, finding
 * 5 contiguous blocks (4 pointers, 1 indirect), toggling, and allocating
 * to the initialized inode.
 * 
 * @param current CFile object passed from the make_file() function
 * 
 * @return File inode (possibly deprecated and redundant)
 */ 
Block init_inode(CFile current) {

    Block inode = disk_drive[current.inode_index];
    int flag = 0;
    Block dbg;
    int db_index;
    // Iterate over all data block groups
    // Find data block group with 5 contiguous blocks
    for(int i = 1025; i < MAX_BLOCKS; i += 1024) { // hits each data block group
        dbg = disk_drive[i]; // data group BLOCK struct
        for(int j = 0; j < sizeof(dbg.bytes) / sizeof(char); j++) { // per byte of block
            for(int k = 0; k < 2; k++) { // per bit of byte (cop out as can't find way to overflow to next byte)
                if(dbg.bytes[j] >> k)
                    continue;
                    for(int l = 1; l < 5; l++) { // check if next 4 bits are 0
                        if(dbg.bytes[j] >> k + l)
                            break;
                        for(int m = k; m < 4; m++) { // could do bitwise 4 at time, but no
                            db_index = i + (j * 8) + k + m; // data block index
                            inode = ins_int(inode, db_index, m * 4);
                            dbg.bytes[i] ^= 1 << k; // toggle bit
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

    return inode;
}

/**
 * Function responsible for converting an integer (4 byte) index to hex
 * values that can be inserted into a char array from char[n] to char[n + 3]
 * within a passed inode. This is used to assign pointers to the inode for
 * retrieving and inserting data.
 * 
 * https://stackoverflow.com/questions/3784263/converting-an-int-into-a-4-byte-char-array-c
 * 
 * @param inode inode in which to write hex values
 * @param db_index index of data block in which to write pointers
 * @param start_index used to determine which part of char[] to write to
 * 
 * @return updated inode block to reassign in calling function
 */
Block ins_int(Block inode, int db_index, int start_index) {

    inode.bytes[start_index] = (db_index >> 24) & 0xFF;
    inode.bytes[start_index + 1] = (db_index >> 16) & 0xFF;
    inode.bytes[start_index + 2] = (db_index >> 8) & 0xFF;
    inode.bytes[start_index + 3] = db_index & 0xFF;

    return inode;
}

/**
 * Shell input function (taken from my assignment 1). This function
 * simply waits for user input and assigns it to a malloc'ed char pointer
 * in the calling function.
 * 
 * @return user input in form of string
 */ 
char* getInput() {

    char* input = NULL;
    size_t ad = 0;
    getline(&input, &ad, stdin);
    input[strlen(input) - 1] = '\0';
    return input;
}

/**
 * Input parser (also taken from my assignment 1). This function takes
 * input from getInput() and returns a double char pointer in order to
 * separate each argument.
 * 
 * @param string user shell input
 * @param args passed args to modify
 * 
 * @return count of arguments that were parsed
 */
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