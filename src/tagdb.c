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

    struct json_object* parsed_json;


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

int save_db(json_object* db) {
    char* full_path = get_db_path();
    
    FILE* fp = fopen(full_path, "w");
    if (!fp) {
        fprintf(stderr, "Error: could not open the DB json file.\n");
        free(full_path);
        return -1;
    }

    // Can also use the variant without "_ext" suffix, but it will create minified / compact JSON
    const char* json_as_string = json_object_to_json_string_ext(db, JSON_C_TO_STRING_PRETTY);

    // Add \n so that the JSON file ends with a new line (for compatiblity purposes)
    fprintf(fp, "%s\n", json_as_string);

    fclose(fp);
    free(full_path);
    return 0;
}

int add_tag(const char *filename, const char *tag) {
    // Entry in the JSON DB to keep track of all existing tags,
    //  "/" is usually reserved Linux Filesystems and so can never collide
    const char* all_tags = "/__all_tags__";

    if (!check_file_exists(filename)) {
        fprintf(stderr, "Error: file does not exist.\n");
        return -1;
    }

    if (tag == NULL || strlen(tag) == 0 || strlen(tag) > 255) {
        fprintf(stderr, "Error: Tag is empty or is too long (max 255 char).\n");
        return -1;
    }

    json_object* db = load_tag_db();
    if (!db) {
        fprintf(stderr, "Error: could not load the database.\n");
        return -1;
    }

    json_object* file_entry;
    json_object_object_get_ex(db, filename, &file_entry);

    if (!file_entry) {
        json_object* tags_array = json_object_new_array();
        json_object_array_add(tags_array, json_object_new_string(tag));
        json_object_object_add(db, filename, tags_array);
    } else {
        int tag_exists_already = 0;     
        for (int i = 0; i < json_object_array_length(file_entry); i++ ) {
            json_object* current_tag = json_object_array_get_idx(file_entry, i);

            if (strcmp(json_object_get_string(current_tag), tag) == 0) {
                tag_exists_already = 1;
                break;
            }
        }

        if(tag_exists_already) {
            fprintf(stderr, "Error: file already assigned this tag.\n");
            return -1;
        } else {
            json_object_array_add(file_entry, json_object_new_string(tag));
        }
    }

    
    json_object* all_tags_entry;
    json_object_object_get_ex(db, all_tags, &all_tags_entry);

    if (!all_tags_entry) {
        json_object* all_tags_array = json_object_new_array();
        json_object_array_add(all_tags_array, json_object_new_string(tag));
        json_object_object_add(db, all_tags, all_tags_array);
    } else {
        int tag_exists_globally = 0;     
            for (int i = 0; i < json_object_array_length(all_tags_entry); i++ ) {
                json_object* current_tag = json_object_array_get_idx(all_tags_entry, i);

                if (strcmp(json_object_get_string(current_tag), tag) == 0) {
                    tag_exists_globally = 1;
                    break;
                }
            }

        if(!tag_exists_globally) {
            json_object_array_add(all_tags_entry, json_object_new_string(tag));
            fprintf(stdout, "New tag detected: added to the list of all tags.\n");
        }
    }

    fprintf(stdout, "Success: added tag to file\n");
    save_db(db);
    json_object_put(db);
    return 0;
}

// TO-DO: sync globally
int remove_tag(const char *filename, const char *tag) {
    if (!check_file_exists(filename)) {
        fprintf(stderr, "Error: file does not exist.\n");
        return -1;
    }

    json_object* db = load_tag_db();
    if (!db) {
        fprintf(stderr, "Error: could not load the database.\n");
        return -1;
    }

    json_object* file_entry;
    json_object_object_get_ex(db, filename, &file_entry);

    if (!file_entry) {
        fprintf(stderr, "Error: file has no assigned tags.\n");
        return -1;
    } else {
        int tag_exists_already = 0;     
        for (int i = 0; i < json_object_array_length(file_entry); i++ ) {
            json_object* current_tag = json_object_array_get_idx(file_entry, i);

            if (strcmp(json_object_get_string(current_tag), tag) == 0) {
                tag_exists_already = 1;
                break;
            }
        }

        if(tag_exists_already) {
            fprintf(stderr, "Error: file already assigned this tag.\n");
            return -1;
        } else {
            json_object_array_add(file_entry, json_object_new_string(tag));
        }
    }

    // TO-DO: sync globally


    fprintf(stdout, "Success: removed tag from file\n");
    save_db(db);
    json_object_put(db);
    return 0;

}

int search_by_tag(const char *tag) {
    json_object* db = load_tag_db();
    if (!db) {
        fprintf(stderr, "Error: could not load the database.\n");
        return -1;
    }
}

/*
    Returns a dynamically allocated array of strings.
    The number of tags is stored in "*count_out".
    Returns 0 on success, -1 on failure.
*/
int list_all_tags(char*** list_all_tags, size_t* count_out) {
    const char* all_tags = "/__all_tags__";

    json_object* db = load_tag_db();
    if (!db) {
        fprintf(stderr, "Error: could not load the database.\n");
        return -1;
    }

    json_object* all_tags_entry;
    json_object_object_get_ex(db, all_tags, &all_tags_entry);

    size_t count_unique_tags = 0;

    if (!all_tags_entry) {
        // "/__all_tags__" does not exist yet.
        *count_out = 0;
        json_object_put(db);
        fprintf(stderr, "Error: there is no entry for all tags in the DB.\n");
        return -1;
    } else {    
        count_unique_tags = json_object_array_length(all_tags_entry); 
        *count_out = count_unique_tags;
        *list_all_tags = malloc(count_unique_tags * sizeof(char*));

        if (!(*list_all_tags)) {
            json_object_put(db);
            fprintf(stderr, "Error: could not allocate memory for the list of tags.\n");
            return -1;
        }

        for (int i = 0; i < count_unique_tags; i++) {
            json_object* current_entry = json_object_array_get_idx(all_tags_entry, i);
            (*list_all_tags)[i] = strdup(json_object_get_string(current_entry)); 
        }
    }

    json_object_put(db);
    return 0;
}

int list_file_tags(const char *filename) {
    if (!check_file_exists(filename)) {
        fprintf(stderr, "Error: file does not exist.\n");
        return -1;
    }

    json_object* db = load_tag_db();
    if (!db) {
        fprintf(stderr, "Error: could not load the database.\n");
        return -1;
    }
}