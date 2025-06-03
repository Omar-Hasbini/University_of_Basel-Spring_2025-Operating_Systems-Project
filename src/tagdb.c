#include <stdio.h>
#include <stdlib.h>
#include <json-c/json.h>
#include <string.h>

int main(int argc, char *argv[]) {
    if (argc == 1) {
        fprintf(stderr, "Error: you didn't provide any arguments or commands\n");
        return 1;
    } else if (argc == 2) {
        if (strcmp(argv[1], "list") == 0) {

        } else {
            fprintf(stderr, "Error: unknown command\n");
            return 1;
        }
    } else if (argc == 3) {
        if (strcmp(argv[1], "list") == 0) {
        
        } else if (strcmp(argv[1], "search") == 0) {

        }
        else {
            fprintf(stderr, "Error: unknown command\n");
            return 1;
        }
    } else if (argc == 4) {
        if (strcmp(argv[1], "add") == 0) {

        }
        else if (strcmp(argv[1], "remove") == 0) {

        } else {
            fprintf(stderr, "Error: unknown command\n");
            return 1;
        }
    } else {
        fprintf(stderr, "Error: you provided too many arguments\n");
        return 1;
    }

    return 0;
}