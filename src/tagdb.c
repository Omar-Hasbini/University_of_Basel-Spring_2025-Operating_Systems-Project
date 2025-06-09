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

    // Entry in the JSON DB to keep track of all existing tags,
    // Collision is impossible, since we store the full path of a file, which will include at least the "/" from the root dir.
    #define ALL_TAGS_KEY "__meta__:all_tags"

    /*
        This library prints out to stdout and stderr, which shouldn't be the case ideally
        as a library should not concern itself with this and since it floods these communication channels
        Nevertheless it works out for this project's use case, however such an implementation should be avoided
        in the future.
    */

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
        const char* suffix = "/.file_tags/tags.json";

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

        // Add \n so that the JSON file ends with a new line (for compatiblity purposes).
        fprintf(fp, "%s\n", json_as_string);

        fclose(fp);
        free(full_path);
        return 0;
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

        mkdir(full_path, 0700);
        json_object* db = json_object_new_object();

        int status = save_db(db);

        if (status == -1) {
            fprintf(stderr, "Error: Newly created DB could not be saved.\n");
            return -1;
        }

        free(full_path);
        return 0;
    }

    // Source (with personal modification): https://www.youtube.com/watch?v=dQyXuFWylm4
    struct json_object* load_tag_db() {
        if (!check_tagdb_exists()) {
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

    int assign_tag(const char *file_name, const char *tag) {
        char *absolute_path = realpath(file_name, NULL);

        if (!absolute_path) {
            perror("realpath failed");
            return -1;
        }

        if (!check_file_exists(absolute_path)) {
            fprintf(stderr, "Error: file does not exist.\n");
            free(absolute_path);
            return -1;
        }

        if (tag == NULL || strlen(tag) == 0 || strlen(tag) > 255) {
            fprintf(stderr, "Error: tag is empty or is too long (max 255 char).\n");
            return -1;
        }

        json_object* db = load_tag_db();
        if (!db) {
            fprintf(stderr, "Error: could not load the DB.\n");
            free(absolute_path);
            return -1;
        }

        json_object* file_entry;
        json_object_object_get_ex(db, absolute_path, &file_entry);

        if (!file_entry) {
            json_object* tags_array = json_object_new_array();
            json_object_array_add(tags_array, json_object_new_string(tag));
            json_object_object_add(db, absolute_path, tags_array);
        } else {
            int tag_exists_already = 0;     

            int len = json_object_array_length(file_entry);  
            for (int i = 0; i < len; i++ ) {
                json_object* current_tag = json_object_array_get_idx(file_entry, i);

                if (strcmp(json_object_get_string(current_tag), tag) == 0) {
                    tag_exists_already = 1;
                    break;
                }
            }

            if(tag_exists_already) {
                fprintf(stderr, "Error: file already assigned this tag.\n");
                json_object_put(db);
                return -1;
            } else {
                int status_assign = json_object_array_add(file_entry, json_object_new_string(tag));
                if (status_assign != 0) {
                    fprintf(stderr, "Error: tag could not be assigned.\n");
                    json_object_put(db);
                    return -1;
                }
            }
        }

        json_object* all_tags_entry;
        json_object_object_get_ex(db, ALL_TAGS_KEY, &all_tags_entry);

        if (!all_tags_entry) {
            json_object* all_tags_array = json_object_new_array();
            json_object_array_add(all_tags_array, json_object_new_string(tag));
            json_object_object_add(db, ALL_TAGS_KEY, all_tags_array);
        } else {
            int tag_exists_globally = 0; 

            int len = json_object_array_length(all_tags_entry);  
            for (int i = 0; i < len; i++ ) {
                json_object* current_tag = json_object_array_get_idx(all_tags_entry, i);

                if (strcmp(json_object_get_string(current_tag), tag) == 0) {
                    tag_exists_globally = 1;
                    break;
                }
            }

            if(!tag_exists_globally) {
                int status_assign_global = json_object_array_add(all_tags_entry, json_object_new_string(tag));
                if (status_assign_global == 0) {
                    fprintf(stdout, "New tag detected: added to the list of all tags.\n");
                } else {
                    fprintf(stderr, "Error: tag could not be added to the list of all tags.\n");
                    json_object_put(db);
                    return -1;
                }
            }
        }


        if (save_db(db) != 0) {
            fprintf(stderr, "Error: could not save the DB after successful assignment.\n");
            json_object_put(db);
            return -1;
        }

        free(absolute_path);
        json_object_put(db);
        return 0;
    }

    int deassign_tag(const char *file_name, const char *tag) {
        char *absolute_path = realpath(file_name, NULL);
        if (!absolute_path) {
            perror("realpath failed");
            return -1;
        }

        if (!check_file_exists(absolute_path)) {
            fprintf(stderr, "Error: file does not exist.\n");
            free(absolute_path);
            return -1;
        }

        json_object* db = load_tag_db();
        if (!db) {
            fprintf(stderr, "Error: could not load the DB.\n");
            return -1;
        }

        json_object* file_entry;
        json_object_object_get_ex(db, absolute_path, &file_entry);

        if (!file_entry) {
            fprintf(stderr, "Error: file has no assigned tags.\n");
            json_object_put(db);
            return -1;
        } else { 
            int tag_index = -1;

            int len = json_object_array_length(file_entry);   

            for (int i = 0; i < len; i++ ) {
                json_object* current_tag = json_object_array_get_idx(file_entry, i);

                if (strcmp(json_object_get_string(current_tag), tag) == 0) {
                    tag_index = i;
                    break;
                }
            }

            if(tag_index == -1) {
                fprintf(stderr, "Error: either tag not assigned to the file and/or the file has no entry in the DB.\n");
                json_object_put(db);
                free(absolute_path)
                return -1;
            } else {

                if (len == 1) {
                    json_object_object_del(db, absolute_path);
                } else {
                    int status_deletion = json_object_array_del_idx(file_entry, tag_index, 1);
                    if (status_deletion !=  0) {
                        fprintf(stderr, "Error: tag could not be deassigned.\n");
                        json_object_put(db);
                        free(absolute_path);
                        return -1;
                    }
                }
            }
        }
        // TO-DO: sync globally
        //

        if (save_db(db) != 0) {
            fprintf(stdout, "Error: could not save the DB after successful deletion.\n");
            json_object_put(db);
            free(absolute_path);
            return -1;
        }

        free(absolute_path);
        json_object_put(db);
        return 0;
    }

    /* 
        - Search for the files who have this tag assigned to them.
        - Returns a dynamically allocated array of strings for the filenames.
        - count_out is the # of matched files

        Returns 0 on success, -1 on failure.

        EOS
    */
    int search_by_tag(const char *tag, char*** result_files, size_t* count_out) {

        json_object* db = load_tag_db();
        if (!db) {
            fprintf(stderr, "Error: could not load the DB.\n");
            return -1;
        }
        (*count_out) = 0;

        // Nested for loop, $O(n^2)$ runtime unfortunately. 
        json_object_object_foreach(db, key, val) {

            if (strcmp(key, ALL_TAGS_KEY) != 0) {

                size_t size_current_val = json_object_array_length(val);
                for (int i = 0; i < size_current_val; i++ ) {
                    json_object* current_tag = json_object_array_get_idx(val, i);

                    if (strcmp(json_object_get_string(current_tag), tag) == 0) {
                        
                        char** temp = realloc(*result_files, (*count_out + 1) * sizeof(char*));

                        if (!temp) {
                            fprintf(stderr, "Error: could not allocate memory for the list of files.\n");
                            json_object_put(db);
                            return -1;
                        }

                        *result_files = temp;
                        (*result_files)[*count_out] = strdup(key);
                        if (!(*result_files)[*count_out]) {
                            fprintf(stderr, "Error: could not allocate memory for a filename in the list.\n");
                            json_object_put(db);
                            return -1;
                        }
                        (*count_out)++;
                        // it is enough to find 1 tag 
                        break;
                    }
                }

            }
            
        }	

        json_object_put(db);
        return 0;
    }

    /*
        Returns a dynamically allocated array of strings.
        The number of tags is stored in "*count_out".
        Returns 0 on success, -1 on failure.
    */
    int list_all_tags(char*** all_tags, size_t* count_out) {
        
        json_object* db = load_tag_db();
        if (!db) {
            fprintf(stderr, "Error: could not load the DB.\n");
            return -1;
        }

        json_object* all_tags_entry;
        json_object_object_get_ex(db, ALL_TAGS_KEY, &all_tags_entry);


        if (!all_tags_entry || json_object_array_length(all_tags_entry) == 0) {
            // "/__all_tags__" does not exist yet.
            fprintf(stderr, "Error: there is no collection with all the tags in the DB.\n");
            *count_out = 0;
            json_object_put(db);
            *all_tags = NULL;
            return -1;
        }   
        
        *count_out = json_object_array_length(all_tags_entry); 
        *all_tags = malloc(*count_out * sizeof(char*));

        if (!(*all_tags)) {
            json_object_put(db);
            fprintf(stderr, "Error: could not allocate memory for the list of tags.\n");
            return -1;
        }

        for (int i = 0; i < *count_out; i++) {
            json_object* current_entry = json_object_array_get_idx(all_tags_entry, i);
            (*all_tags)[i] = strdup(json_object_get_string(current_entry)); 
        }
        
        json_object_put(db);
        return 0;
    }

    int list_file_tags(const char *file_name, char*** file_tags, size_t* count_out) {

        char *absolute_path = realpath(file_name, NULL);
        if (!absolute_path) {
            if (access(file_name, F_OK) == 0) {
                fprintf(stderr, "Warning: realpath failed, using raw path\n");
                absolute_path = strdup(file_name);
            } else {
                perror("realpath failed");
                return -1;
            }
        }

        json_object* db = load_tag_db();
        if (!db) {
            fprintf(stderr, "Error: could not load the DB.\n");
            return -1;
        }

        json_object* file_entry;
        json_object_object_get_ex(db, absolute_path, &file_entry);

        if (!file_entry || json_object_array_length(file_entry) == 0) {
            fprintf(stderr, "Error: file has no entry in the DB or the set of its tags is empty.\n");
            *count_out = 0;
            *file_tags = NULL;
            json_object_put(db);
            return -1;
        } 

        *count_out = json_object_array_length(file_entry); 
        *file_tags = malloc(*count_out * sizeof(char*));

        if (!(*file_tags)) {
            json_object_put(db);
            fprintf(stderr, "Error: could not allocate memory for the list of tags.\n");
            return -1;
        }

        for (int i = 0; i < *count_out; i++) {
            json_object* current_entry = json_object_array_get_idx(file_entry, i);
            (*file_tags)[i] = strdup(json_object_get_string(current_entry)); 
        }
        
        free(absolute_path);
        json_object_put(db);
        return 0;
    }

    int deassign_all_tags(const char *file_name) {  
        char *absolute_path = realpath(file_name, NULL);

        if (!absolute_path) {
            perror("realpath failed");
            return -1;
        }

        if (!check_file_exists(absolute_path)) {
            fprintf(stderr, "Error: file does not exist.\n");
            free(absolute_path);
            return -1;
        }

        json_object* db = load_tag_db();
        if (!db) {
            fprintf(stderr, "Error: could not load the DB.\n");
            free(absolute_path);
            return -1;
        }

        json_object* empty_array = json_object_new_array();

        json_object *existing;
        if (!json_object_object_get_ex(db, absolute_path, &existing)) {
            fprintf(stderr, "Warning: file had no tags assigned. It's key will be removed from the DB.\n");
        }

        json_object_object_del(db, absolute_path);

        save_db(db);
        json_object_put(db);
        free(absolute_path);

        return 0;
    }

    int deassign_all_tags_systemwide() {
        char* full_path = get_db_path();

        if (!full_path) {
            fprintf(stderr, "Error: could not retrieve the DB's path.\n");
            return -1;
        }

        int status = remove(full_path);
        free(full_path);

        if (status != 0) {
            fprintf(stderr, "Error: could not deassign all tags.\n");
            return -1;
        }   

        return 0;
    }

    int count_tags(const char* file_name, size_t* count_out) {
        char *absolute_path = realpath(file_name, NULL);
        if (!absolute_path) {
            if (access(file_name, F_OK) == 0) {
                fprintf(stderr, "Warning: realpath failed, using raw path\n");
                absolute_path = strdup(file_name);
            } else {
                perror("realpath failed");
                return -1;
            }
        }

        json_object* db = load_tag_db();
        if (!db) {
            fprintf(stderr, "Error: could not load the DB.\n");
            free(absolute_path);
            return -1;
        }

        json_object* file_entry;
        json_object_object_get_ex(db, absolute_path, &file_entry);

        if (!file_entry) {
            *count_out = 0;
        } else { 
            *count_out = json_object_array_length(file_entry);   
        }
        free(absolute_path);
        json_object_put(db);
        return 0;
    }

    /*
        Returns 1 if tag exists, 0 if not.
    */
    int tag_exists(const char* tag) {
        json_object* db = load_tag_db();
        if (!db) {
            fprintf(stderr, "Error: could not load the DB.\n");
            return -1;
        }

        json_object* file_entry;
        json_object_object_get_ex(db, ALL_TAGS_KEY, &file_entry);

        if (!file_entry) {
            json_object_put(db);
            return 0;
        } else { 
            int len = json_object_array_length(file_entry);  
            for (int i = 0; i < len; i++) {
                json_object* current_tag = json_object_array_get_idx(file_entry, i);

                if (strcmp(json_object_get_string(current_tag), tag) == 0) {
                    json_object_put(db);
                    return 1;
                }
            }
            
        }

        json_object_put(db);
        return 0;
    }

    /* 
        Returns 1 if file has tag assigned, 0 if not.
    */
    int file_has_tag(const char* file_name, const char* tag) {
        char *absolute_path = realpath(file_name, NULL);
        if (!absolute_path) {
            perror("realpath failed");
            return -1;
        }

        json_object* db = load_tag_db();
        if (!db) {
            fprintf(stderr, "Error: could not load the DB.\n");
            return -1;
        }

        json_object* file_entry;
        json_object_object_get_ex(db, absolute_path, &file_entry);

        if (!file_entry) {
            free(absolute_path);
            json_object_put(db);
            return 0;
        }

        int file_has_tag = 0;     

        int len = json_object_array_length(file_entry);  
        for (int i = 0; i < len; i++ ) {
            json_object* current_tag = json_object_array_get_idx(file_entry, i);

            if (strcmp(json_object_get_string(current_tag), tag) == 0) {
                file_has_tag = 1;
                break;
            }
        }


        free(absolute_path);
        json_object_put(db);
        return file_has_tag;
    }

    int list_all_files_with_tags(char*** result_files, size_t* count_out) {
        json_object* db = load_tag_db();
        if (!db) {
            fprintf(stderr, "Error: could not load the DB.\n");
            return -1;
        }

        json_object_object_foreach(db, key, val) {
            size_t size_current_val = json_object_array_length(val);
            if (size_current_val >= 1) {
                char** temp = realloc(*result_files, (*count_out + 1) * sizeof(char*));

                if (!temp) {
                    fprintf(stderr, "Error: could not allocate memory for the list of files.\n");
                    json_object_put(db);
                    return -1;
                }

                *result_files = temp;
                (*result_files)[*count_out] = strdup(key);
                if (!(*result_files)[*count_out]) {
                    fprintf(stderr, "Error: could not allocate memory for a filename in the list.\n");
                    json_object_put(db);
                    return -1;
                }
                (*count_out)++;
            }  
        }	

        json_object_put(db);
        return 0;
    }  

    int assign_all_tags_to_file(const char* file_path) {
        // Idempotent function

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

        json_object* db = load_tag_db();
        if (!db) {
            fprintf(stderr, "Error: could not load the DB.\n");
            return -1;
        }

        json_object* all_tags_entry;
        json_object_object_get_ex(db, ALL_TAGS_KEY, &all_tags_entry);

        size_t len_arr_all_tags = json_object_array_length(all_tags_entry);

        if (!all_tags_entry || len_arr_all_tags == 0) {
            fprintf(stderr, "Error: there are no tags yet in the DB and so none can be assigned to this file.\n");
            json_object_put(db);
            free(absolute_path);
            return -1;
        }

        json_object* arr_with_all_tags = json_object_new_array();

        // Safe overwrite due to monotonicity S_1 >= S_0 because $S_1 := S_0 \cup \{all tags\}$
        for (size_t i = 0; i < len_arr_all_tags; i++) {
            char* current_tag = json_object_get_string(json_object_array_get_idx(all_tags_entry, i));
            json_object_array_add(arr_with_all_tags, json_object_new_string(current_tag));
        }

        json_object_object_add(db, absolute_path, arr_with_all_tags);

        save_db(db);
        json_object_put(db);
        free(absolute_path);
        return 0;
    }

//  Disabled due to time constraints and needing further case-handling.
/*
    int rename_tag(const char* old_tag, const char* new_tag) {
        json_object* db = load_tag_db();
        if (!db) {
            fprintf(stderr, "Error: could not load the DB.\n");
            return -1;
        }

        // check if tag exists 
        json_object* all_tags_entry;
        json_object_object_get_ex(db, ALL_TAGS_KEY, &all_tags_entry);


        if (!all_tags_entry) {
            fprintf(stderr, "Error: there are no tags yet in the DB => the old tag does not exist in the first place.\n");
            json_object_put(db);
            return -1;
        }

        size_t len_arr_all_tags = json_object_array_length(all_tags_entry);

        if (len_arr_all_tags == 0){
            fprintf(stderr, "Error: there are no tags yet in the DB => the old tag does not exist in the first place.\n");
            json_object_put(db);
            return -1;
        }


        int tag_exists = 0;
        char* current_tag;

        for (size_t i = 0; i < len_arr_all_tags; i++) {
            current_tag = json_object_get_string(json_object_array_get_idx(all_tags_entry, i));
            
            if (strcmp(current_tag, old_tag) == 0) {
                tag_exists = 1;
                break;
            }
        }

        if (!tag_exists) {
            fprintf(stderr, "Error: the old tag could not be found in the DB.\n");
            json_object_put(db);
            return -1;
        }

        json_object_object_foreach(db, key, val) {
            size_t size_current_val = json_object_array_length(val);
                for (int i = 0; i < size_current_val; i++ ) {
                    char* current_tag = json_object_get_string(json_object_array_get_idx(val, i));

                    if (strcmp(json_object_get_string(current_tag), old_tag) == 0) {
                        json_object_array_put_idx(val, i, current_tag);

                        // Only 1 instance of this tag should exist.
                        break;
                    }
                }
        }

        json_object_put(db);
        return 0;
    }
*/

    int count_files_with_tag(const char* tag, size_t* count_out) {
        json_object* db = load_tag_db();
        if (!db) {
            fprintf(stderr, "Error: could not load the DB.\n");
            return -1;
        }
        (*count_out) = 0;


        json_object_object_foreach(db, key, val) {
            if (strcmp(key, ALL_TAGS_KEY) != 0) {
                size_t size_current_val = json_object_array_length(val);
                for (size_t i = 0; i < size_current_val; i++ ) {
                    char* current_tag = json_object_get_string(json_object_array_get_idx(val, i));

                    if (strcmp(current_tag, tag) == 0) {
                        (*count_out)++;
                        break;
                    }
                }
            }
        }

        json_object_put(db);
        return 0;
    }

    int remove_tag_globally(const char* tag) {
        json_object* db = load_tag_db();
        if (!db) {
            fprintf(stderr, "Error: could not load the DB.\n");
            return -1;
        }

        json_object_object_foreach(db, key, val) {
            ssize_t size_current_val = json_object_array_length(val);
            ssize_t index = -1;
            
            for (ssize_t i = 0; i < size_current_val; i++ ) {
                char* current_tag = json_object_get_string(json_object_array_get_idx(val, i));

                if (strcmp(current_tag, tag) == 0) {
                    index = i;
                    break;
                }
            }

            if (index != -1) {
                json_object_array_del_idx(val, index, 1);
                // File had only 1 tag which was removed
                if (size_current_val == 1) {
                    json_object_object_del(db, key);
                }
            }
        }

        json_object_put(db);
        return 0;
    }

    

    
    /*
        can be implemented if time allows:
            - groups of files or groups of tags
            - copy_and_assign_tags_from (seems redundant when considering the option above)
            - distribute
            - auto-complete terminal
            - man page
    */ 