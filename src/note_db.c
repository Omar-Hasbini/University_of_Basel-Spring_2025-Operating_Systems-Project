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
#include <errno.h>
#include "note_db.h"


/*
    The code has been tested and cleaned up partially, however it is still affected by a short deadline. 
    Thus, further testing, refactoring and treating with precaution is desirable.
*/

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
    return access(file_path, F_OK) == 0;
}

char* get_db_path() {
    // Source (with personal modification): https://stackoverflow.com/questions/308695/how-do-i-concatenate-const-literal-strings-in-c
    char* home = get_home();
    const char* suffix = "/.file_notes/notes.json";

    if (!home) {
        fprintf(stderr, "Error: could not determine the home directory path.\n");
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
    free(home);

    return full_path;
}

int save_db(json_object* db) {
    char* full_path = get_db_path();

    if (!full_path) {
        fprintf(stderr, "Error: could not retrieve the DB's path.\n");
        return -1;
    }
    
    FILE* fp = fopen(full_path, "w");
    if (!fp) {
        fprintf(stderr, "Error: could not open the DB json file.\n");
        free(full_path);
        return -1;
    }

    // Can also use the variant without "_ext" suffix, but it will create minified / compact JSON
    const char* json_as_string = json_object_to_json_string_ext(db, JSON_C_TO_STRING_PRETTY);

    // Add "\n" so that the JSON file ends with a new line (for compatiblity purposes).
    fprintf(fp, "%s\n", json_as_string);

    fclose(fp);
    free(full_path);
    return 0;
}

// Return 1 if db exists, 0 otherwise
int check_notedb_exists() {
    char* full_path = get_db_path();

    if (!full_path) {
        return 0; 
    }

    int status = check_file_exists(full_path);
    free(full_path);

    return status;
}

int create_db() {
    char* full_path = get_db_path();

    if (!full_path) {
        fprintf(stderr, "Error: could not retrieve the DB's path.\n");
        return -1;
    }

    char* last_slash = NULL;
    size_t index = 0;


    for (size_t i = 0; i < strlen(full_path); i++) {
        if (full_path[i] == '/') {
            last_slash = &full_path[i]; 
            index = i;
        }
    }

    full_path[index + 1] = '\0';

    if (mkdir(full_path, 0700) != 0 && errno != EEXIST) {
        perror("mkdir failed");
        free(full_path);
        return -1;
    }

    json_object* db = json_object_new_object();

    int status = save_db(db);

    if (status == -1) {
        fprintf(stderr, "Error: Newly created DB could not be saved.\n");
        json_object_put(db);
        free(full_path);
        return -1;
    }

    free(full_path);
    return 0;
}

// Source (with personal modification): https://www.youtube.com/watch?v=dQyXuFWylm4
struct json_object* load_note_db() {
    if (!check_notedb_exists()) {
        int status = create_db();
        if (status == -1) {
            fprintf(stderr, "Error: DB could not be created.\n");
            return NULL;
        }
    }

    char* full_path = get_db_path();

    if (!full_path) {
        fprintf(stderr, "Error: could not retrieve the DB's path.\n");
        return NULL;
    }

    FILE *fp;

    struct json_object* parsed_json;


    fp = fopen(full_path, "r");
    
    if (!fp) {
        fprintf(stderr, "Error: could not open the DB .json file\n");
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
        fprintf(stderr, "Error: could not allocate memory for buffer while loading the DB.\n");
        free(full_path);
        return NULL;
    }

    size_t read_bytes = fread(buffer, 1, size_db, fp);
    fclose(fp);

    if (read_bytes != size_db) {
        fprintf(stderr, "Error: could not read the DB file.\n");
        free(buffer);
        free(full_path);
        return NULL;
    }

    buffer[size_db] = '\0';

    parsed_json = json_tokener_parse(buffer);
    if (!parsed_json) {
        fprintf(stderr, "Error: JSON DB is corrupted or invalid.\n");
        free(buffer);
        free(full_path);
        return NULL;
    }

    free(full_path);
    free(buffer);

    return parsed_json;
}

int assign_note(const char *file_path, const char *note) {

    if (!note) {
        fprintf(stderr, "Error: <note> is NULL.\n");
        return -1;
    }

    char *absolute_path = realpath(file_path, NULL);

    if (!absolute_path) {
        perror("realpath failed");
        return -1;
    }

    if (!check_file_exists(absolute_path)) {
        fprintf(stderr, "Error: file does not exist.\n");
        free(absolute_path);
        return -1;
    }

    if (strlen(note) == 0) {
        fprintf(stderr, "Error: note is empty.\n");
        free(absolute_path);
        return -1;
    }

    json_object* db = load_note_db();
    if (!db) {
        fprintf(stderr, "Error: could not load the DB.\n");
        free(absolute_path);
        return -1;
    }

    json_object* note_array = json_object_new_array();
    json_object_array_add(note_array, json_object_new_string(note));
    json_object_object_add(db, absolute_path, note_array);

    if (save_db(db) != 0) {
        fprintf(stderr, "Error: could not save the DB after successful assignment.\n");
        json_object_put(db);
        return -1;
    }

    free(absolute_path);
    json_object_put(db);
    return 0;
}

int deassign_note(const char *file_path) {

        char *absolute_path = realpath(file_path, NULL);
        if (!absolute_path) {
            perror("realpath failed");
            return -1;
        }

        if (!check_file_exists(absolute_path)) {
            fprintf(stderr, "Error: file does not exist.\n");
            free(absolute_path);
            return -1;
        }

        json_object* db = load_note_db();
        if (!db) {
            fprintf(stderr, "Error: could not load the DB.\n");
            free(absolute_path);
            return -1;
        }

        json_object* file_entry;
        json_object_object_get_ex(db, absolute_path, &file_entry);

        if (!file_entry) {
            fprintf(stderr, "Error: file has no assigned note.\n");
            json_object_put(db);
            free(absolute_path);
            return -1;
        } else { 
            json_object_object_del(db, absolute_path);
        }

        if (save_db(db) != 0) {
            fprintf(stdout, "Error: could not save the DB after successful deassignment.\n");
            json_object_put(db);
            free(absolute_path);
            return -1;
        }

        free(absolute_path);
        json_object_put(db);
        return 0;
}

 int show_file_note(const char *file_path, char** file_note) {

    if (!file_note) {
        fprintf(stderr, "Error: <file_note> is NULL.\n");
        return -1;
    }

    char *absolute_path = realpath(file_path, NULL);
    if (!absolute_path) {
        perror("realpath failed");
        return -1;
    }

    json_object* db = load_note_db();
    if (!db) {
        fprintf(stderr, "Error: could not load the DB.\n");
        free(absolute_path);
        return -1;
    }

    json_object* file_entry;
    json_object_object_get_ex(db, absolute_path, &file_entry);

    if (!file_entry || json_object_array_length(file_entry) == 0) {
        fprintf(stderr, "Error: file has no entry in the DB and/or has no note assigned.\n");
        *file_note = NULL;
        json_object_put(db);
        free(absolute_path);
        return -1;
    } 

    json_object* current_entry = json_object_array_get_idx(file_entry, 0);
    *file_note = strdup(json_object_get_string(current_entry)); 
    
    free(absolute_path);
    json_object_put(db);
    return 0;
}
