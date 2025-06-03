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
        return 1;
    } else if (argc == 2) {
        if (strcmp(argv[1], "list") == 0) {
            return list_all_tags();
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