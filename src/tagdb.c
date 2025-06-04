/*
Metadata
Author: Omar Fadi Hasbini
Context: University of Basel, Operating Systems, Spring 2025
License: Check https://github.com/Omar-Hasbini/University_of_Basel-Spring_2025-Operating_Systems-Project
*/ 

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <json-c/json.h>
#include <sys/types.h>
#include <unistd.h>
#include <pwd.h>
#include "tagdb.h"

// Source (with personal modification): https://community.unix.com/t/getting-home-directory/248085/2
// This is not needed for shell inputs since the shell auto-expands the "~".
char* get_home() {
    struct passwd *pwd; 
    if ( (pwd=getpwuid(getuid())) != NULL )
    {
        return strdup(pwd->pw_dir);
    } else {
        char* home_env = getenv("HOME");
        if (home_env != NULL) {
            return strdup(home_env);
        }
    }

    return NULL;
}

// Let function caller free "*file_path"
// Return 1 if file exists, 0 otherwise
int check_file_exists(const char *file_path) {
    FILE *fp = fopen(file_path, "r");

    if (fp) {
        fclose(fp);
        return 1;
    } else {
        return 0;
    }
}

char* get_db_path() {
    // Source (with personal modification): https://stackoverflow.com/questions/308695/how-do-i-concatenate-const-literal-strings-in-c
    char* home = get_home();
    const char* suffix = "/.file_tags/tags.json";

    if (!home) {
        return NULL;  
    }

    // Buffer allocation
    size_t length_path = strlen(home) + strlen(suffix) + 1;
    char* full_path = malloc(length_path);

    if (!full_path) {
        free(home);
        return NULL;
    }

    strcpy(full_path, home);
    strcat(full_path, suffix);

    return full_path;
}


// Return 1 if db exists, 0 otherwise
int check_tagdb_exists() {
    char* full_path = get_db_path();
    if (!full_path) {
        return 0; 
    }

    int status = check_file_exists(full_path);
    free(full_path);

    return status;
}

// Source (with personal modification): https://www.youtube.com/watch?v=dQyXuFWylm4
struct json_object* load_tag_db() {
    if (!check_tagdb_exists()) {
        fprintf(stderr, "Error: DB does not exist and cannot be loaded\n");
        return NULL;
    }

    char* full_path = get_db_path();
    FILE *fp;

    struct json_object *parsed_json;


    fp = fopen(full_path, "r");
    
    if (!fp) {
        fprintf(stderr, "Error: could not open the DB json file\n");
        free(full_path);
        return NULL;
    }

    // Source: https://stackoverflow.com/questions/238603/how-can-i-get-a-files-size-in-c
    fseek(fp, 0L, SEEK_END);
    size_t size_db = ftell(fp);
    // EOS
    rewind(fp);

    char *buffer = malloc(size_db + 1);

    if (!buffer) {
        fprintf(stderr, "Error: could not allocate memory for buffer while loading the database.\n");
        free(full_path);
        return NULL;
    }

    size_t read_bytes = fread(buffer, 1, size_db, fp);
    fclose(fp);

    if (read_bytes != size_db) {
        fprintf(stderr, "Error: could not read the database file.\n");
        free(buffer);
        free(full_path);
        return NULL;
    }

    buffer[size_db] = '\0';

    parsed_json = json_tokener_parse(buffer);

    free(full_path);
    free(buffer);

    return parsed_json;
}

int save_db() {
    json_object* db = load_tag_db();
    if (!db) return -1;
}

int add_tag(const char *filename, const char *tag) {
    json_object* db = load_tag_db();
    if (!db) return -1;
}

int remove_tag(const char *filename, const char *tag) {
    json_object* db = load_tag_db();
    if (!db) return -1;
}

int search_by_tag(const char *tag) {
    json_object* db = load_tag_db();
    if (!db) return -1;
}

int list_all_tags() {
    json_object* db = load_tag_db();
    if (!db) return -1;
}

int list_file_tags(const char *filename) {
    json_object* db = load_tag_db();
    if (!db) return -1;
}