#define VERSION                        "1.0.0"
#define BUFFER_LENGTH                  32
#define DEFAULT_USER_INPUT_NUMBER_BASE 10
#include "../include/decode.h"
#include "../include/encode.h"
#include "../include/utils.h"
#include <stdio.h>
#include <stdlib.h>

void prompt();

int main() {
    printf("steganov v%s\n", VERSION);
    prompt();
    return EXIT_SUCCESS;
}

void prompt() {
    char buffer[BUFFER_LENGTH];
    char *message = NULL;
    while (1) {
        printf("select the operation:\n1-encode data\n2-decode data\n3-exit\n");
        void *buff = fgets(buffer, sizeof(buffer), stdin);
        if (buff == NULL) {
            printf("ERROR: Failed to read user input from stdin\n");
            return;
        }
        int user_option = (int)strtol(buffer, NULL, DEFAULT_USER_INPUT_NUMBER_BASE);
        switch (user_option) {
        case ENCODE:
            if (encode()) {
                printf("INFO: successful encoding\n");
            } else {
                printf("INFO: failed to encoding\n");
            }
            break;
        case DECODE:
            message = decode();
            if (message == NULL) {
                printf("WARNING: could not find a message\n");
                break;
            }
            printf("INFO: secret message extracted from file is: %s\n", message);
            free(message);
            break;
        case EXIT:
        default:
            return;
        }
    }
}
