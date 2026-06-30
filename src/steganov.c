#define VERSION                        "1.0.0"
#define BUFFER_LENGTH                  32
#define DEFAULT_USER_INPUT_NUMBER_BASE 10
#include "../include/decode.h"
#include "../include/encode.h"
#include "../include/utils.h"
#include <stdio.h>
#include <stdlib.h>

void prompt(char **file_name, char **secret_message);
void prompt_cleanup(char *secret_pointer, char *file_name_pointer);

int main() {
    printf("steganov v%s\n", VERSION);

    char *secret_message = NULL;
    char *file_name      = NULL;

    prompt(&file_name, &secret_message);
    prompt_cleanup(secret_message, file_name);

    return EXIT_SUCCESS;
}

void prompt(char **file_name, char **secret_message) {
    char buffer[BUFFER_LENGTH];
    char *message = NULL;
    while (1) {
        printf("select the operation:\n1-encode data\n2-decode data\n3-exit\n");
        void *buff = fgets(buffer, sizeof(buffer), stdin);
        if (buff == NULL) {
            printf("ERROR: Failed to read user input from stdin\n");
            return;
        }
        int userOption = (int)strtol(buffer, NULL, DEFAULT_USER_INPUT_NUMBER_BASE);
        switch (userOption) {
        case ENCODE:
            if (encode(*file_name, *secret_message)) {
                printf("INFO: successful encoding\n");
            } else {
                printf("INFO: failed to encoding\n");
            }
            break;
        case DECODE:
            message = decode(*file_name);
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

void prompt_cleanup(char *secret_pointer, char *file_name_pointer) {
    free(secret_pointer);
    free(file_name_pointer);
}
