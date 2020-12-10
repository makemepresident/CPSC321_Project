#define BLOCK_SIZE 128 // in bytes
#define MAX_BLOCKS 10240 // in blocks

#define FS_ID "0xf0f03410" // necessary, but useless for our case
#define TOTAL_BLOCKS "10240"
#define TOTAL_INODES "1024"
#define MAX_FILES 20

typedef struct Block {
    unsigned char bytes[BLOCK_SIZE - sizeof(int)];
    // int cptr; // used to index last open char in block for old additive write
} Block;

typedef struct CFile {
    char* filename;
    int inode_index;
} CFile;

char* disk_read(int);
void disk_write(char*, int);
void partition();
void init_superblock();
void init_inbm();
void init_inodes();
void make_file(char*);
void write_file(char*, char*);
Block init_inode(CFile);
Block ins_int(Block, int, int);
void delete_file(char*);
char* getInput();
int parseInput(char*, char**);