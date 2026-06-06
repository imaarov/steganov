#define VERSION                "1.0.0"
#include <stdlib.h>
#include <stdio.h>
#include "../include/utils.h"
#include "../include/encode.h"
#include "../include/decode.h"

void prompt(char **fileName, char **secretMessage);

int main() {
    printf("steganov v%s\n", VERSION);
    char *secretMessage = NULL;
    char *fileName = NULL;
    prompt(&fileName, &secretMessage);

    // clean up
    free(secretMessage);
    free(fileName);
    return 0;
}

void prompt(char **fileName, char **secretMessage) {
    char buffer[32];
    char *message = NULL;
    while (1) {
        printf("select the operation:\n1-encode data\n2-decode data\n3-exit\n");
        void *buff = fgets(buffer, sizeof(buffer), stdin);
        if (buff == NULL) {
            printf("ERROR: Failed to read user input from stdin\n");
            return;
        }
        int userOption = (int)strtol(buffer, NULL, 10);
        switch (userOption) {
            case ENCODE:
                if (encode(*fileName, *secretMessage)) {
                    printf("INFO: successful encoding\n");
                }else {
                    printf("INFO: failed to encoding\n");
                }
                break;
            case DECODE:
                message = decode(*fileName);
                if (message == NULL) {
                    printf("WARNING: could not find a message\n");
                    break;
                }
                printf("INFO: secret message extracted from file is: %s\n", message);
                break;
            case EXIT:
            default:
                return;
        }
    }
}
