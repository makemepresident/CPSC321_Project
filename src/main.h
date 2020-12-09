#define BLOCK_SIZE 128 // in bytes
#define GROUP_SIZE 1024 // in data blocks
#define MAX_BLOCKS 10240 // in blocks
#define MAX_POINTER 1310720 // max index
#define GROUP_BM_INC 1025 // incrementer for data group bitmaps

#define FS_ID "f0f03410" // necessary, but useless for our case
#define MAX_BLOCKS_STR "10240"
#define NUM_INODES "1024"

void startCFFS();
char* disk_read(int, int);
void disk_write(char*, int);
char* getInput();
int parseInput(char*, char**);