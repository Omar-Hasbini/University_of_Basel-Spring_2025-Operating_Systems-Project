/*
Metadata
Author: Omar Fadi Hasbini
Context: University of Basel, Operating Systems, Spring 2025
License: Check https://github.com/Omar-Hasbini/University_of_Basel-Spring_2025-Operating_Systems-Project
*/ 

/*
    The library this CLI tool uses prints out to stdout and stderr, which shouldn't be the case ideally
    as a library should not concern itself with this and since it floods these communication channels.
    Nevertheless it works out for this project's use case, however such an implementation should be avoided
    in the future.
*/

/*
    The code has been tested and cleaned up moderately, however this + development was done 
    under a rather short deadline. Thus, further testing and refactoring,
    as well as treating with precaution is desirable.
*/

#include <stdio.h>
#include <string.h>
#include <locale.h>
#include "tagdb.h"


void print_usage() {
    const char* general;
    const char* assignment;
    const char* deassignment;
    const char* queries;
    const char* maintenance;

    // Detect UTF-8 locale support
    const char* loc = setlocale(LC_CTYPE, "");
    int use_emoji = loc && strstr(loc, "UTF-8");

    // Taxonomy
    general     = use_emoji ? "📁 General Commands"       : "[*] General Commands";
    assignment  = use_emoji ? "📥 Assignment Commands"    : "[+] Assignment Commands";
    deassignment= use_emoji ? "🗑️ Deassignment Commands"  : "[-] Deassignment Commands";
    queries     = use_emoji ? "❓ Query Commands"          : "[?] Query Commands";
    maintenance = use_emoji ? "🛠️ Maintenance Commands"   : "[!] Maintenance Commands";

    fprintf(stderr, "\nUsage (N.B.: <file_path> can be relative or absolute):\n");
    fprintf(stderr, "  tagger help\n");
    fprintf(stderr, "  tagger list\n");
    fprintf(stderr, "\n");

    fprintf(stderr, "%s\n", general);
    fprintf(stderr, "  tagger list <file_path>\n");
    fprintf(stderr, "  tagger list-all-files-with-tags\n");
    fprintf(stderr, "  tagger search <tag>\n");

    fprintf(stderr, "\n%s\n", assignment);
    fprintf(stderr, "  tagger assign <file_path> <tag>\n");
    fprintf(stderr, "  tagger assign-all-tags-to-file <file_path>\n");

    fprintf(stderr, "\n%s\n", deassignment);
    fprintf(stderr, "  tagger deassign <file_path> <tag>\n");
    fprintf(stderr, "  tagger deassign-all-tags <file_path>\n");
    fprintf(stderr, "  tagger deassign-all-tags-systemwide\n");
    fprintf(stderr, "  tagger remove-tag-globally <tag>\n");

    fprintf(stderr, "\n%s\n", queries);
    fprintf(stderr, "  tagger count-tags <file_path>\n");
    fprintf(stderr, "  tagger count-files-with-tag <tag>\n");
    fprintf(stderr, "  tagger tag-exists <tag>\n");
    fprintf(stderr, "  tagger file-has-tag <file_path> <tag>\n");

    fprintf(stderr, "\n%s\n", maintenance);
    fprintf(stderr, "  tagger rename-tag <old_tag> <new_tag>    (NOT IMPLEMENTED)\n");

    fprintf(stderr, "\nSee LICENSE for details. This tool is provided for educational purposes only.\n");
    fprintf(stderr, "Use these commands at your own discretion. The author assumes no responsibility\n");
    fprintf(stderr, "and provides no warranty, liability, or guarantee. It is not intended for use in\n");
    fprintf(stderr, "any critical systems, including but not limited to flight control.\n");
}

int main(int argc, char *argv[]) {
    
    if (argc == 1 || argc >= 5) {
        fprintf(stderr, "Error: Invalid amount of arguments\n");
        print_usage();
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

            printf("Total: %zu tag(s).\n", count_out);
            for (size_t i = 0; i < count_out; i++) {
                printf("%s\n", all_tags[i]);
                free(all_tags[i]);
            }
            free(all_tags);  
            return 0;

        } else if (strcmp(argv[1], "deassign-all-tags-systemwide") == 0){
            int status = deassign_all_tags_systemwide();

            if (status == -1) {
                fprintf(stderr, "Error: command failed. See previous output for details.\n");
                return -1;
            }
            printf("Sucess: all tags have been deassigned (i.e., the DB was deleted).\n");
            return 0;

        } else if (strcmp(argv[1], "list-all-files-with-tags") == 0) {
            char** result_files = NULL;
            size_t count_out = 0;

            int status = list_all_files_with_tags(&result_files, &count_out);

            if (status == -1) {
                fprintf(stderr, "Error: command failed. See previous output for details.\n");
                return -1;
            }
            
            printf("Total: %zu tag(s).\n", count_out);
            printf("List:\n");
            for (size_t i = 0; i < count_out; i++) {
                printf("%s\n", result_files[i]);
                free(result_files[i]);
            }

            free(result_files);  
            return 0;

        } else if (strcmp(argv[1], "help") == 0) { 

            printf("For more information, consult the man page by typing in the shell's terminal: man tagger\n");
            return 0;

        } else {
            fprintf(stderr, "Error: unknown command\n");
            print_usage();
            return -1;
        }
    } else if (argc == 3) {
        if (strcmp(argv[1], "list") == 0) { 
            char** file_tags = NULL;
            size_t count_out = 0;

            int status = list_file_tags(argv[2], &file_tags, &count_out);

            if (status == -1) {
                fprintf(stderr, "Error: command failed. See previous output for details.\n");
                return -1;
            }

            printf("Total: %zu tag(s).\n", count_out);
            printf("List:\n");
            for (size_t i = 0; i < count_out; i++) {
                printf("%s\n", file_tags[i]);
                free(file_tags[i]);
            }

            free(file_tags);  
            return 0;
            
        } else if (strcmp(argv[1], "search") == 0) {
            char** result_files = NULL;
            size_t count_out = 0;

            int status = search_by_tag(argv[2], &result_files, &count_out);

            if (status == -1) {
                fprintf(stderr, "Error: command failed. See previous output for details.\n");
                return -1;
            }

            printf("Total: %zu tag(s).\n", count_out);
            printf("List of files with this tag:\n");
            for (size_t i = 0; i < count_out; i++) {
                printf("%s\n", result_files[i]);
                free(result_files[i]);
            }
            free(result_files);  
            return 0;
        } else if (strcmp(argv[1], "deassign-all-tags") == 0) {

            int status = deassign_all_tags(argv[2]);
            if (status == -1) {
                fprintf(stderr, "Error: command failed. See previous output for details.\n");
                return -1;
            }
            printf("Success: the file has no more tags assigned.\n");
            return 0;

        } else if (strcmp(argv[1], "count-tags") == 0) {

            size_t count_out = 0;
            int status =  count_tags(argv[2], &count_out);
            if (status == -1) {
                fprintf(stderr, "Error: command failed. See previous output for details.\n");
            }
            printf("Outcome: File has %zu tag(s)\n", count_out);
            return 0;

        } else if (strcmp(argv[1], "tag-exists") == 0) {

            int exists = tag_exists(argv[2]);
            if (exists == -1) {
                fprintf(stderr, "Error: command failed. See previous output for details.\n");
                return -1;
            } else if (exists == 1) {
                printf("Outcome: This tag exists in the DB.\n");
            } else {
                printf("Outcome: This tag does not exist in the DB.\n");
            }
            return 0;

        } else if (strcmp(argv[1], "assign-all-tags-to-file") == 0) {

            int status = assign_all_tags_to_file(argv[2]);
            if (status == -1) {
                fprintf(stderr, "Error: command failed. See previous output for details.\n");
                return -1;
            }
            printf("Success: all tags assigned to file\n");
            return 0;

        } else if (strcmp(argv[1], "count-files-with-tag") == 0) {

            size_t count_out;
            int status = count_files_with_tag(argv[2], &count_out);
            if (status == -1) {
                fprintf(stderr, "Error: command failed. See previous output for details.\n");
                return -1;
            }
            printf("There are %zu file(s) with this tag\n", count_out);
            return 0;

        } else if (strcmp(argv[1], "remove-tag-globally") == 0) {

            int status = remove_tag_globally(argv[2]);
            if (status == -1) {
                fprintf(stderr, "Error: command failed. See previous output for details.\n");
                return -1;
            }
            printf("Success: tag was removed globally.\n");
            return 0;
        
        } else {
            fprintf(stderr, "Error: unknown command\n");
            print_usage();
            return -1;
        } 
    
    } else if (argc == 4) {

        if (strcmp(argv[1], "assign") == 0) {
                int status = assign_tag(argv[2], argv[3]);

                if (status == -1) {
                    fprintf(stderr, "Error: command failed. See previous output for details.\n");
                    return -1;
                }

                printf("Success: the tag was assigned to this file.\n");
                return 0;
        } else if (strcmp(argv[1], "deassign") == 0) {
            int status = deassign_tag(argv[2], argv[3]);

            if (status == -1) {
                fprintf(stderr, "Error: command failed. See previous output for details.\n");
                return -1;
            }
            printf("Success: the tag was deassigned from this file.\n");
            return 0;
            
        } else if (strcmp(argv[1], "file-has-tag") == 0) {

            int exists = file_has_tag(argv[2], argv[3]);
            if (exists == -1) {
                fprintf(stderr, "Error: command failed. See previous output for details.\n");
                return -1;
            } else if (exists == 1) {
                printf("Outcome: The tag is assigned to this file.\n");
            } else {
                printf("Outcome: The tag is not assigned to this file.\n");
            }
            return 0;

// Not implemented
 /*           
        } 
        else if (strcmp(argv[1], "rename-tag")) {
            int status = rename_tag(argv[2], argv[3]):;

            if (status == -1) {
                fprintf(stderr, "Error: command failed. See previous output for details.\n");
                return -1;
            }

            printf("Success: tag was renamed and this change was propagated throughout the DB.\n");
            return 0;
*/
// 

        } else {
            // Unreachable code but may be necessary to avoid compiler error
            // No turns out it is reachable.
            fprintf(stderr, "Error: unknown command\n");
            print_usage();
            return -1;
        }
    } else {
        fprintf(stderr, "Error: Invalid amount of arguments\n");
        print_usage();
        return -1;
    }
}