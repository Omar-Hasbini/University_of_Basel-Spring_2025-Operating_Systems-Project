#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <json-c/json.h>
#include <sys/types.h>
#include <unistd.h>
#include <pwd.h>
#include "tagdb.h"

int add_tag(const char *filename, const char *tag) {

    json_object* db = load_db();
}

int remove_tag(const char *filename, const char *tag) {
    json_object* db = load_db();
}

int search_by_tag(const char *tag) {
    json_object* db = load_db();
}

int list_all_tags() {
    json_object* db = load_db();
}

int list_file_tags(const char *filename) {
    json_object* db = load_db();
}

struct json_object* load_db() {
    
}

int save_db() {

}

int check_tagdb_exists() {
    // Source: https://stackoverflow.com/questions/308695/how-do-i-concatenate-const-literal-strings-in-c
    char* home = get_home();
    const char* suffix = "/.file_tags/tags.json";

    if (!home) {
        return 0;  
    }

    // Buffer allocation
    size_t length_path = strlen(home) + strlen(suffix) + 1;
    char* full_path = malloc(length_path);

    if (!full_path) {
        free(home);
        return 0;
    }

    strcpy(full_path, home);
    strcat(full_path, suffix);

    int status = check_file_exists(full_path);

    free(full_path);
    free(home);

    return status;
}

// Let function caller free "*file_path"
int check_file_exists(const char *file_path) {
    FILE *fp = fopen(file_path, "r");

    if (fp) {
        fclose(fp);
        return 1;
    } else {
        return 0;
    }
}

// Source: https://community.unix.com/t/getting-home-directory/248085/2
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