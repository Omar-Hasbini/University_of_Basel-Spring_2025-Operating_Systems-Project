/*
Metadata
Author: Omar Fadi Hasbini
Context: University of Basel, Operating Systems, Spring 2025
License: Check https://github.com/Omar-Hasbini/University_of_Basel-Spring_2025-Operating_Systems-Project
*/ 

#include <stdio.h>
#include <string.h>
#include "tagdb.h"

int main(int argc, char *argv[]) {
    if (argc == 1 || argc >= 5) {
        fprintf(stderr, "Error: you didn't provide the correct amount of arguments\n");
        fprintf(stderr, "Usage:\n");
        fprintf(stderr, "  tagger list\n");
        fprintf(stderr, "  tagger list <file>\n");
        fprintf(stderr, "  tagger search <tag>\n");
        fprintf(stderr, "  tagger add <file> <tag>\n");
        fprintf(stderr, "  tagger remove <file> <tag>\n");
        return -1;
    } else if (argc == 2) {
        if (strcmp(argv[1], "list") == 0) {
            char** all_tags = NULL;
            size_t count_out = 0;

            int status = list_all_tags(&all_tags, &count_out);

            if (status == -1) {
                fprintf(stderr, "Error: command failed. See previous output for details.\n");
                return -1;
            }

            for (size_t i = 0; i < count_out; i++) {
                printf("%s\n", all_tags[i]);
                free(all_tags[i]);
            }

            free(all_tags);  
            return 0;
        } else {

            fprintf(stderr, "Error: unknown command\n");
            return 1;
        }
    } else if (argc == 3) {
        if (strcmp(argv[1], "list") == 0) {
            return list_file_tags(argv[2]);
        } else if (strcmp(argv[1], "search") == 0) {
            return search_by_tag(argv[2]);
        }
        else {
            fprintf(stderr, "Error: unknown command\n");
            return 1;
        }
    } else if (argc == 4) {
        if (strcmp(argv[1], "add") == 0) {
            return add_tag(argv[2], argv[3]);
        }
        else if (strcmp(argv[1], "remove") == 0) {
            return remove_tag(argv[2], argv[3]);
        } else {
            fprintf(stderr, "Error: unknown command\n");
            return 1;
        }
    } 

    return 1;
}