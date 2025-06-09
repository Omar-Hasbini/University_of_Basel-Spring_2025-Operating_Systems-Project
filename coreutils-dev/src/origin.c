

#include "origin.h"

char *get_xattr_value(const char *filepath, const char *attr_name) {
#ifdef __linux__
    // determine the size neededd for the attribute value
    ssize_t size = getxattr(filepath, attr_name, NULL, 0);

    if (size == -1) {
        if (errno == ENODATA) {
            return NULL;
        } else {
            perror("getxattr error");
            return NULL;
        }
    }

    char *value = (char *)malloc(size + 1);
    if (value == NULL) {
        perror("malloc failed");
        return NULL;
    }

    ssize_t bytes_read = getxattr(filepath, attr_name, value, size);
    if (bytes_read == -1) {
        perror("getxattr error");
        free(value);
        return NULL;
    }

    value[bytes_read] = '\0';
    return value;
#else
    fprintf(stderr, "Warning: Extended attributes not supported on this OS for file: %s\n", filepath);
    return NULL;
#endif
}

void print_file_info(const char *filepath, bool show_origin_flag) {
    struct stat sb;

    // Use lstat to handle symbolic links correctly (gets info about the link itself)
    if (lstat(filepath, &sb) == -1) {
        perror("lstat error");
        return;
    }

    // printf("%s", filepath);

    // If -origin flag is set, try to get and print the origin metadata
    if (show_origin_flag) {
        char *origin_metadata = get_xattr_value(filepath, ORIGIN_XATTR_NAME);
        if (origin_metadata != NULL) {
            printf("Origin: %s", origin_metadata);
            free(origin_metadata); 
        } else {
            printf("Origin: N/A");
        }
    }
    printf("\n");
}

int main(int argc, char *argv[]) {
    bool show_origin = false;
    // Pointers to store directory/file paths, if provided
    char **paths = NULL;
    int num_paths = 0;

    paths = (char **)malloc(sizeof(char *) * (argc - 1));
    if (paths == NULL) {
        perror("malloc failed for paths");
        return EXIT_FAILURE;
    }

    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "-origin") == 0) {
            show_origin = true;
        } else {
            paths[num_paths++] = argv[i];
        }
    }

    // If no paths are provided, default to the current directory "."
    if (num_paths == 0) {
        paths[num_paths++] = ".";
    }

    for (int i = 0; i < num_paths; ++i) {
        const char *current_path = paths[i];
        struct stat sb;

        if (lstat(current_path, &sb) == -1) {
            perror("lstat error");
            fprintf(stderr, "Could not access '%s'\n", current_path);
            continue; 
        }

        if (S_ISDIR(sb.st_mode)) {
            DIR *dir = opendir(current_path);
            if (dir == NULL) {
                perror("opendir error");
                fprintf(stderr, "Could not open directory '%s'\n", current_path);
                continue;
            }

            printf("\n%s:\n", current_path); 
            struct dirent *dp;
            while ((dp = readdir(dir)) != NULL) {
                // Skip "." and ".." entries
                if (strcmp(dp->d_name, ".") == 0 || strcmp(dp->d_name, "..") == 0) {
                    continue;
                }

                // Construct the full path for the directory entry
                char full_path[1024]; 
                snprintf(full_path, sizeof(full_path), "%s/%s", current_path, dp->d_name);

                print_file_info(full_path, show_origin);
            }
            closedir(dir);
        } else if (S_ISREG(sb.st_mode) || S_ISLNK(sb.st_mode)) {
            print_file_info(current_path, show_origin);
        } else {
            printf("%s [Special File Type]\n", current_path);
        }
    }

    free(paths); 
    return EXIT_SUCCESS;
}




