// daemon.c
#define _GNU_SOURCE      /* Enable GNU extensions for functions like readlink */
#include <stdio.h>       /* Standard I/O routines */
#include <stdlib.h>      /* General utilities: malloc, getenv */
#include <limits.h>      /* PATH_MAX constant */
#include <unistd.h>      /* POSIX API: access, usleep, readlink */
#include <string.h>      /* String handling functions */
#include <errno.h>       /* Error number definitions */
#include <sys/xattr.h>   /* Extended attribute functions */
#include <bpf/libbpf.h>  /* libbpf core definitions */
#include <bpf/bpf.h>     /* BPF helper functions */

#define ORIGIN_ATTR "user.origin"  /* Name of the xattr storing origin path */

/* Data structure matching the BPF ring buffer event layout */
struct event_t {
    __u32 pid;              /* Process ID that created the file */
    char filename[256];     /* Filename as passed to openat */
};

/*
 * Callback invoked for each BPF event (file creation with O_CREAT flag).
 * It resolves the full path, skips the Downloads folder, waits for
 * the file to appear, and sets an extended attribute with the origin.
 */
static int handle_event(void *ctx, void *data, size_t len) {
    const struct event_t *e = data;
    char path[PATH_MAX];

    /* Skip empty filename events */
    if (e->filename[0] == '\0') {
        fprintf(stderr, "⚠️ Empty filename from PID=%u\n", e->pid);
        return 0;
    }

    printf("🧠 Event received: PID=%u, filename=%s\n",
           e->pid, e->filename);

    /* Build absolute path without using realpath() */
    if (e->filename[0] == '/') {
        /* Already an absolute path: copy directly */
        strncpy(path, e->filename, sizeof(path));
        path[sizeof(path)-1] = '\0';
    } else {
        /* Relative path: prefix with process working directory */
        char proc_cwd[64];
        snprintf(proc_cwd, sizeof(proc_cwd),
                 "/proc/%u/cwd", e->pid);

        char cwd[PATH_MAX];
        ssize_t len = 0;
        int rltries = 5;
        /* Retry readlink() a few times in case the symlink isn't ready */
        while (rltries-- > 0) {
            len = readlink(proc_cwd, cwd, sizeof(cwd) - 1);
            if (len > 0)
                break;
            usleep(1000);  /* Wait 1 ms */
        }

        /* Fail if unable to read cwd after retries */
        if (len <= 0) {
            fprintf(stderr,
                    "⚠️ readlink failed for %s after retries: %s\n",
                    proc_cwd, strerror(errno));
            return 0;
        }

        cwd[len] = '\0';  /* Null-terminate the cwd string */

        /* Combine cwd + '/' + filename, checking for overflow */
        size_t cwdlen = strlen(cwd);
        size_t namelen = strlen(e->filename);
        if (cwdlen + 1 + namelen + 1 > sizeof(path)) {
            fprintf(stderr,
                    "⚠️ Path too long, skipping: %s + %s\n",
                    cwd, e->filename);
            return 0;
        }
        memcpy(path, cwd, cwdlen);
        path[cwdlen] = '/';
        memcpy(path + cwdlen + 1, e->filename, namelen);
        path[cwdlen + 1 + namelen] = '\0';
    }

    printf("📍 Path: %s\n", path);

    /* Exclude files under the user's ~/Downloads directory */
    const char *home = getenv("HOME");
    if (home) {
        char excl[PATH_MAX];
        snprintf(excl, sizeof(excl),
                 "%s/Downloads/", home);
        if (strncmp(path, excl, strlen(excl)) == 0) {
            printf("🔇 Skipping downloads path: %s\n", path);
            return 0;
        }
    }

    /* Wait briefly in case the file is still being created */
    int tries = 5;
    while (tries-- > 0 && access(path, F_OK) != 0)
        usleep(1000);

    /* If the file still doesn't exist, skip it */
    if (access(path, F_OK) != 0) {
        fprintf(stderr, "❌ File still absent: %s\n", path);
        return 0;
    }

    /* Set the extended attribute with the absolute path */
    if (setxattr(path, ORIGIN_ATTR,
                 path, strlen(path), 0) == 0)
        printf("✅ Origin set on %s → %s\n", path, path);
    else
        fprintf(stderr,
                "❌ setxattr failed on %s: %s\n",
                path, strerror(errno));

    return 0;
}

/* Custom print callback for libbpf debug output */
static int libbpf_print_fn(enum libbpf_print_level level,
                          const char *fmt, va_list args) {
    return vfprintf(stderr, fmt, args);
}

/*
 * Program entry point: load and attach the BPF program, then
 * poll the ring buffer for file creation events indefinitely.
 */
int main() {
    struct bpf_object *obj;
    int map_fd, err;
    struct ring_buffer *rb = NULL;
    struct bpf_program *prog;
    struct bpf_link *link = NULL;

    /* Redirect libbpf logs to stderr */
    libbpf_set_print(libbpf_print_fn);

    printf("🚀 Loading BPF object...\n");
    obj = bpf_object__open_file("filetrace.bpf.o", NULL);
    if (!obj) {
        fprintf(stderr, "❌ bpf_object__open_file failed\n");
        return 1;
    }
    if ((err = bpf_object__load(obj))) {
        fprintf(stderr, "❌ bpf_object__load: %d\n", err);
        return 1;
    }

    /* Find and attach the tracepoint program */
    prog = bpf_object__find_program_by_name(obj, "handle_openat");
    if (!prog) {
        fprintf(stderr, "❌ program 'handle_openat' not found\n");
        return 1;
    }
    link = bpf_program__attach(prog);
    if (!link) {
        fprintf(stderr, "❌ bpf_program__attach failed\n");
        return 1;
    }

    /* Set up the ring buffer from the 'events' map */
    map_fd = bpf_object__find_map_fd_by_name(obj, "events");
    if (map_fd < 0) {
        fprintf(stderr, "❌ map 'events' not found\n");
        return 1;
    }
    rb = ring_buffer__new(map_fd, handle_event, NULL, NULL);
    if (!rb) {
        fprintf(stderr, "❌ ring_buffer__new failed\n");
        return 1;
    }

    printf("✅ Daemon started, listening for file creations...\n");
    /* Poll the ring buffer forever (or until an error occurs) */
    while (1) {
        err = ring_buffer__poll(rb, 100 /* milliseconds */);
        if (err < 0 && err != -EINTR) {
            fprintf(stderr, "❌ ring_buffer__poll: %d\n", err);
            break;
        }
    }

    /* Clean up resources */
    ring_buffer__free(rb);
    bpf_link__destroy(link);
    bpf_object__close(obj);
    return 0;
}

