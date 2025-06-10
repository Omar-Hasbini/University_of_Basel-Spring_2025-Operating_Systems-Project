//#include <config.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
//#include <sys/xattr.h>
#include "note_db.h"

#define NOTE_XATTR "user.note"

/* Function prototypes */
static void usage(const char *prog);
static void version(void);
static void add_note(const char *file, const char *note);
static void show_note(const char *file);
static void remove_note(const char *file);

static void
usage(const char *prog)
{
    (void) prog;  // suppress unused parameter warning
    fprintf(stderr, "Usage:\n");
    fprintf(stderr, "  note -add <file> <note>\n");
    fprintf(stderr, "  note -show <file>\n");
    fprintf(stderr, "  note -remove <file>\n");
    fprintf(stderr, "  note --help\n");
    fprintf(stderr, "  note --version\n");
    exit(EXIT_FAILURE);
}

static void
version(void)
{
    //printf("note (coreutils) %s\n", PACKAGE_VERSION);
    exit(EXIT_SUCCESS);
}

static void
add_note(const char *file, const char *note)
{
    if (assign_note(file, note) != 0)
    {
    	fprintf(stderr, "Error assigning note to %s\n", file);
    	exit(EXIT_FAILURE);
    }
}

static void
show_note(const char *file)
{   
    char *note = NULL;
    if (show_file_note(file, &note) != 0)
    {
    	fprintf(stderr, "Error retrieving note for %s\n", file);
    	exit(EXIT_FAILURE);
    }
    
    if (note == NULL || strlen(note) == 0)
    {
    	printf("No note set\n");
    }
    else
    {
    	printf("%s\n", note);
    }
    
    free(note);
}

static void
remove_note(const char *file)
{
    if (deassign_note(file) != 0)
    {
    	printf("No note to remove.\n");
    }
}

int
main(int argc, char *argv[])
{
    if (argc >= 2)
    {
        if (strcmp(argv[1], "--help") == 0)
            usage(argv[0]);
        else if (strcmp(argv[1], "--version") == 0)
            version();
    }

    if (argc < 3)
        usage(argv[0]);

    if (strcmp(argv[1], "-add") == 0)
    {
        if (argc != 4)
            usage(argv[0]);
        add_note(argv[2], argv[3]);
    }
    else if (strcmp(argv[1], "-show") == 0)
    {
        if (argc != 3)
            usage(argv[0]);
        show_note(argv[2]);
    }
    else if (strcmp(argv[1], "-remove") == 0)
    {
        if (argc != 3)
            usage(argv[0]);
        remove_note(argv[2]);
    }
    else
    {
        usage(argv[0]);
    }

    return EXIT_SUCCESS;
}

