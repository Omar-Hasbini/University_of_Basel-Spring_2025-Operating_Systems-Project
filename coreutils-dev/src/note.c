#include <config.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/xattr.h>

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
    fprintf(stderr, "  note add <file> <note>\n");
    fprintf(stderr, "  note show <file>\n");
    fprintf(stderr, "  note remove <file>\n");
    fprintf(stderr, "  note --help\n");
    fprintf(stderr, "  note --version\n");
    exit(EXIT_FAILURE);
}

static void
version(void)
{
    printf("note (coreutils) %s\n", PACKAGE_VERSION);
    exit(EXIT_SUCCESS);
}

static void
add_note(const char *file, const char *note)
{
    if (setxattr(file, NOTE_XATTR, note, strlen(note), 0) != 0)
    {
        perror("setxattr");
        exit(EXIT_FAILURE);
    }
}

static void
show_note(const char *file)
{
    ssize_t size = getxattr(file, NOTE_XATTR, NULL, 0);
    if (size < 0)
    {
        if (errno == ENODATA)
        {
            printf("No note set.\n");
            return;
        }
        perror("getxattr");
        exit(EXIT_FAILURE);
    }

    char *buffer = malloc(size + 1);
    if (!buffer)
    {
        perror("malloc");
        exit(EXIT_FAILURE);
    }

    if (getxattr(file, NOTE_XATTR, buffer, size) < 0)
    {
        perror("getxattr");
        free(buffer);
        exit(EXIT_FAILURE);
    }

    buffer[size] = '\0';
    printf("%s\n", buffer);
    free(buffer);
}

static void
remove_note(const char *file)
{
    if (removexattr(file, NOTE_XATTR) != 0)
    {
        if (errno == ENODATA)
        {
            printf("No note to remove.\n");
            return;
        }
        perror("removexattr");
        exit(EXIT_FAILURE);
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

    if (strcmp(argv[1], "add") == 0)
    {
        if (argc != 4)
            usage(argv[0]);
        add_note(argv[2], argv[3]);
    }
    else if (strcmp(argv[1], "show") == 0)
    {
        if (argc != 3)
            usage(argv[0]);
        show_note(argv[2]);
    }
    else if (strcmp(argv[1], "remove") == 0)
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

