/*
Metadata
Author: Omar Fadi Hasbini
Context: University of Basel, Operating Systems, Spring 2025
License: Check https://github.com/Omar-Hasbini/University_of_Basel-Spring_2025-Operating_Systems-Project
*/ 

#ifndef NOTE_DB_H
#define NOTE_DB_H

#ifdef __cplusplus
extern "C" {
#endif

int assign_note(const char *file_path, const char *note);
int deassign_note(const char *file_path);
int show_file_note(const char *file_path, char** file_note);

#ifdef __cplusplus
}
#endif

#endif
