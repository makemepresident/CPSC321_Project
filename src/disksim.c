#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "shell.h"
#include "main.h"

char* block;
char* group;

int main() {

    printf("Terminal\n");
    while(1) {
        char** args = malloc(sizeof(char*) * MAX_ARGS);
        for(int i = 0; i < MAX_ARGS; args++)
            *args = malloc(sizeof(char) * MAX_ARG_LEN);
        printf(">");
        char* input_buff = malloc(sizeof(char) * BUFF_LEN);
        input_buff = getInput();
        if(strcmp(input_buff, "exit\n") == 0)
            exit(EXIT_SUCCESS);
        int numargs = parseInput(input_buff, args);
        free(input_buff);
        args = realloc(args, sizeof(char*) * numargs);
        
        if(strcmp(args[0], "cffs") == 0) {
            startCFFS();
        } else {
            if(fork() == 0) {
               execvp(args[0], args);
            } else {
                wait(NULL);
            }
        }
    }

    return 0;
}

void startCFFS() {

    block = malloc(sizeof(char) * BLOCK_SIZE);
    group = malloc(sizeof(block) * GROUP_SIZE);

    
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