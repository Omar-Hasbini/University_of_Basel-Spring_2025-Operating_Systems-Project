#ifndef TAGDB_H
#define TAGDB_H

int add_tag(const char *filename, const char *tag);
int remove_tag(const char *filename, const char *tag);
int search_by_tag(const char *tag);
int list_all_tags();
int list_file_tags(const char *filename);

#endif