#include <stdlib.h>
#include <string.h>
#define VERSION "1.0.0"
#define MAX_FILE_NAME_LEN 255
#define MAX_SECRET_MESSAGE_LEN 255
#include <stdio.h>
#include "../include/encode.h"

void prompt(char **fileName, char **secretMessage);
char* getFileName();
char* getSecret();

int main() {
    printf("steganov v%s\n", VERSION);
    char *secretMessage = NULL;
    char *fileName = NULL;
    prompt(&fileName, &secretMessage);
    printf("DEBUG: fileName: %s, secretMessage: %s\n", fileName, secretMessage);
    printf("the format of the file is : %s\n", fileName);


    // clean up
    free(secretMessage);
    free(fileName);
    return 0;
}

void prompt(char **fileName, char **secretMessage) {
    char buffer[32];
    while (1) {
        printf("select the operation:\n1-encode data\n2-decode data\n3-exit\n");
        void *buff = fgets(buffer, sizeof(buffer), stdin);
        if (buff == NULL) {
            printf("ERROR: Failed to read user input from stdin\n");
            return;
        }
        int userOption = (int)strtol(buffer, NULL, 10);
        if (userOption == 1) {
            *fileName      = getFileName();
            *secretMessage = getSecret();
            if (*fileName == NULL || *secretMessage == NULL) {
                continue;
            }
            encode(*fileName, *secretMessage);
        }else if(userOption == 2) {
            *fileName      = getFileName();
            if (*fileName == NULL) {
                continue;
            }
            //TODO decode
        }else {
            //TODO cleanup & exit
            break;
        }
    }
}

char* getSecret() {
    char *secretMessage = malloc(sizeof(char) * MAX_SECRET_MESSAGE_LEN);
    memset(secretMessage, 0, sizeof(char) * MAX_SECRET_MESSAGE_LEN);
    printf("enter the secret message you want to be encoded in data: \n");
    void *buff = fgets(secretMessage, MAX_SECRET_MESSAGE_LEN, stdin);
    if (buff == NULL) {
        printf("ERROR: Failed to read user input from stdin\n");
        return NULL;
    }
    secretMessage[strcspn(secretMessage, "\r\n")] = 0;
    return secretMessage;
}

char* getFileName() {
    char *fileName = malloc(sizeof(char) * MAX_FILE_NAME_LEN);
    memset(fileName, 0, sizeof(char) * MAX_FILE_NAME_LEN);
    printf("enter the file name where data should encode/decode there: \n");
    void *buff = fgets(fileName, MAX_FILE_NAME_LEN, stdin);
    if (buff == NULL) {
        printf("ERROR: Failed to read user input from stdin\n");
        return NULL;
    }
    fileName[strcspn(fileName, "\r\n")] = 0;
    return fileName;
}
