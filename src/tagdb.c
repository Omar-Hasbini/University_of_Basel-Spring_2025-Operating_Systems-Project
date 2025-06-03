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
    char dir[] = "~/home/.filetags/tags.json";
    FILE fp* = fopen(dir);
    if (fp) {
        return 0;
    } else {
        return 1;
    }
}

int check_file_exists() {
    // Source: https://stackoverflow.com/questions/308695/how-do-i-concatenate-const-literal-strings-in-c

    char* home[] = get_home;

    if (!home) {
        return 1;  
    }

    FILE fp* = fopen(tags_file_dir);
    if (fp) {
        return 0;
    } else {
        return 1;
    }
}

// Source: https://community.unix.com/t/getting-home-directory/248085/2

char* get_home() {
    struct passwd *pwd; 
    if ( (pwd=getpwuid(getuid())) != NULL )
    {
        returns strdup(pwd->pw_dir);
    } else {
        char* home_env = getenv("HOME");
        if (home_env != NULL) {
            return strdup(home_env);
        }
    }

    return NULL;
}