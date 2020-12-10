/**
 * Project was incredibly hard, please lord have mercy <3
 */

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

/**
 * Main driver function, contains rudimentary shell with limited functionality
 */
int main() {

    printf("CFFS shell starting - (type help for... help)\n");

    while(1) {

        // allocates memory for input buffer, is freed on exit()
        printf("> ");
        char* input_buffer = malloc(sizeof(char) * 512);
        input_buffer = getInput();
        char** args = malloc(sizeof(char*) * 3);
        for(int i = 0; i < 3; i++)
            args[i] = malloc(sizeof(char) * 512);
        int numargs = parseInput(input_buffer, args);

        // control for commands
        if(strcmp(args[0], "make_file") == 0 && strlen(args[1]) < 55) {
            make_file(args[1]);
        } else if(strcmp(args[0], "write_file") == 0 && strlen(args[1]) < 55 && strlen(args[2]) < 512) {
            write_file(args[1], args[2]);
        } else if(strcmp(args[0], "delete_file") == 0 && strlen(args[1]) < 55) {
            delete_file(args[1]);
        } else if(strcmp(args[0], "exit") == 0) {
            printf("- Shell terminating\n");
            for(int i = 0; i < 3; i++)
                free(args[i]);
            free(args);
            free(input_buffer);
            exit(EXIT_SUCCESS);
        } else if(strcmp(args[0], "ls") == 0) {
            for(int i = 0; i < MAX_FILES; i++)
                if(files[i].filename != NULL)
                    printf("%s   ", files[i].filename);
            printf("\n");
        } else if(strcmp(args[0], "help") == 0) {
            printf("Commands:   make_file   write_file   delete_file (WIP)   ls   exit\n");
        } else {
            printf("Unable to find that command, try something else\n");
        }
    }

    return 0;
}

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
    if(strcmp(content, "{0}") == 0) {
        for(int i = 0; i < BLOCK_SIZE; i++)
            disk_drive[block].bytes[i] = 0x00;
        return;
    }
    

    /** this function was used for additive writing, but became complex for multiple blocks **/
    // int pointer = disk_drive[block].cptr;
    // for(int i = 0; i < strlen(content); i++, disk_drive[block].cptr++)
    //     disk_drive[block].bytes[pointer + i] = content[i];

    for(int i = 0; i < strlen(content); i++)
        disk_drive[block].bytes[i] = content[i];

    printf("File write was successful!\n");
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
            // printf("%d\n", inbm.bytes[i] >> j);
            if(inbm.bytes[i] >> j)
                continue;
            inbm.bytes[i] ^= 1 << j;
            current.inode_index = 3 + (i * 8) + j;
            // printf("%d\n", current.inode_index);
            flag = 1;
            break;
        }
        if(flag)
            break;
    }
    files[len_files++] = current;
    init_inode(current);
    disk_drive[1] = inbm;

    printf("\"%s\" was successfully created.\n", name);
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

    // db_index conversion inspired by // https://stackoverflow.com/questions/5784605/converting-hex-value-in-char-array-to-an-integer-value
    Block inode = disk_drive[current.inode_index];
    int blocks_needed = strlen(content) / 128 + 1;
    char temp[128];
    int db_index;
    if(blocks_needed < 4) {
        for(int i = 0; i < blocks_needed; i++) {
            db_index = inode.bytes[i * 4] << 24 | inode.bytes[i * 4 + 1] << 16 | inode.bytes[i * 4 + 2] << 8 | inode.bytes[i * 4 + 3];
            for(int j = 0; j < 128; j++) {
                temp[j] = content[(i * 128) + j];
            }
            disk_write(content, db_index);
        }
        printf("The file contains: ");
        for(int i = 0; i < 4; i++)
            printf("%s", disk_read(db_index + i));
        printf("\n");
    } else {
        printf("Unfortunately, that's not going to work captain. We're all outta technology!\n");
    }
    //else if(strlen(content) < 124 * 2) {
        // write to first block for all up to 124
        // write rest to second block
    //} // continue else statements, no indirect pointer for now :(
}

/**
 * WIP
 * 
 * Deletes a file given a user specified filename. At the moment, rudimentary;
 * clears the inode of any information, and clears each corresponding direct
 * data block of any information. Does not go back through data block bitmap
 * to switch bits back to free.
 */
void delete_file(char* filename) {

    CFile current;
    int caught = 0;
    int file_index;
    for(int i = 0; i < len_files; i++)
        if(strcmp(files[i].filename, filename) == 0) {
            current = files[i];
            caught = 1;
            file_index = i;
        }
    
    if(!caught) {
        printf("Unable to find file, please try again\n");
        return;
    }

    Block inode = disk_drive[current.inode_index];
    int db_index;
    for(int i = 0; i < 4; i++) {
        db_index = inode.bytes[i * 4] << 24 | inode.bytes[i * 4 + 1] << 16 | inode.bytes[i * 4 + 2] << 8 | inode.bytes[i * 4 + 3];
        disk_write("{0}", db_index);
    }    
    disk_write("{0}", current.inode_index);
    files[file_index].filename = '\0';
    files[file_index].inode_index = 0;

    printf("Deleted %s\n", filename);
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
            for(int k = 0; k < 5; k++) { // per bit of byte (cop out as can't find way to overflow to next byte)
                if(dbg.bytes[j] >> k)
                    continue;
                db_index = i + (j * 8) + k; // data block index
                inode = ins_int(inode, db_index, k * 4);
                dbg.bytes[j] ^= 1 << k; // toggle bit
                flag = 1;
            }
            if(flag) {
                disk_drive[i] = dbg;
                goto jump;
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
 * separate each argument. For this project, this function has mostly
 * been hardcoded as iterating over a while loop requires parsing quotes
 * and ain't nobody got time for that.
 * 
 * @param string user shell input
 * @param args passed args to modify
 * 
 * @return count of arguments that were parsed
 */
int parseInput(char* string, char** args) {

    char* parse = strtok(string, DELIM);
    strcpy(args[0], parse);
    if(parse != NULL) {
        parse = strtok(NULL, DELIM);
        if(parse != NULL) {
            strcpy(args[1], parse);
            if(parse != NULL) {
                parse = strtok(NULL, "\0");
                if(parse != NULL)
                    strcpy(args[2], parse);
            }
        }
    }
    return 3;
}