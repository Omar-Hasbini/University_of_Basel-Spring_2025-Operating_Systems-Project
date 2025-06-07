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
int list_all_tags(char*** list_all_tags, size_t* count_out);
int list_file_tags(const char *filename, char*** list_file_tags, size_t* count_out);
int deassign_all_tags_systemwide();
int deassign_all_tags(const char *file_name);

#endif
