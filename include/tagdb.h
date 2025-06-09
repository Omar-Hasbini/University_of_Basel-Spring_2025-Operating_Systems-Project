/*
Metadata
Author: Omar Fadi Hasbini
Context: University of Basel, Operating Systems, Spring 2025
License: Check https://github.com/Omar-Hasbini/University_of_Basel-Spring_2025-Operating_Systems-Project
*/ 

#ifndef TAGDB_H
#define TAGDB_H

int assign_tag(const char *filename, const char *tag);
int deassign_tag(const char *filename, const char *tag);
int search_by_tag(const char *tag, char*** result_files, size_t* count_out);
int list_all_tags(char*** all_tags, size_t* count_out);
int list_file_tags(const char *filename, char*** file_tags, size_t* count_out);
int deassign_all_tags_systemwide();
int deassign_all_tags(const char *file_name);
int count_tags(const char* file_name, size_t* count_out);
int tag_exists(const char* tag);
int file_has_tag(const char* file_path, const char* tag);
int list_all_files_with_tags(char*** result_files, size_t* count_out);
int assign_all_tags_to_file(const char* file_path);
// int rename_tag(const char* old_tag, const char* new_tag);
int count_files_with_tag(const char* tag, size_t* count_out);
int remove_tag_globally(const char* tag)

#endif
