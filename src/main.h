#define BLOCK_SIZE 128 // in bytes
#define GROUP_SIZE 1024 // in data blocks
#define MAX_BLOCKS 10240 // in blocks
#define MAX_POINTER 1310720 // max index
#define GROUP_BM_INC 1025 // incrementer for data group bitmaps

#define FS_ID "0xf0f03410" // necessary, but useless for our case
#define TOTAL_BLOCKS "10240"
#define TOTAL_INODES "1024"
#define MAX_FILES 20

typedef struct Block {
    unsigned char bytes[BLOCK_SIZE - sizeof(int)];
    int cptr; // used to index last open char in block
} Block;

typedef struct CFile {
    char* filename;
    int inode_index;
} CFile;

void startCFFS();
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