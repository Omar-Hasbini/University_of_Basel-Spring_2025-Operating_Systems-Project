// daemon.c
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/xattr.h>
#include <bpf/libbpf.h>
#include <bpf/bpf.h>

#define ORIGIN_ATTR "user.origin"

struct event_t {
    __u32 pid;
    char filename[256];
};

static int handle_event(void *ctx, void *data, size_t len) {
    const struct event_t *e = data;
    char resolved[PATH_MAX];

    if (e->filename[0] == '\0') {
        fprintf(stderr, "⚠️ Empty filename from PID=%u\n", e->pid);
        return 0;
    }

    printf("🧠 Event received: PID=%u, filename=%s\n", e->pid, e->filename);
    if (!realpath(e->filename, resolved)) {
        fprintf(stderr, "⚠️ realpath failed for %s: %s\n", e->filename, strerror(errno));
        return 0;
    }

    printf("📍 Resolved: %s\n", resolved);

    // kurz warten, falls das File noch nicht vollständig angelegt ist
    int tries = 5;
    while (tries-- > 0 && access(resolved, F_OK) != 0)
        usleep(1000);

    if (access(resolved, F_OK) != 0) {
        fprintf(stderr, "❌ File still absent: %s\n", resolved);
        return 0;
    }

    // Attribut setzen
    const char *origin = "local";
    if (setxattr(resolved, ORIGIN_ATTR, origin, strlen(origin), 0) == 0)
        printf("✅ Origin set on %s → %s\n", resolved, origin);
    else
        fprintf(stderr, "❌ setxattr failed on %s: %s\n", resolved, strerror(errno));

    return 0;
}

static int libbpf_print_fn(enum libbpf_print_level level, const char *fmt, va_list args) {
    return vfprintf(stderr, fmt, args);
}

int main() {
    struct bpf_object *obj;
    int map_fd, err;
    struct ring_buffer *rb = NULL;
    struct bpf_program *prog;
    struct bpf_link *link = NULL;

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

    // Programm finden & attachen
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

    // Ringbuffer auf Map fd
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
    while (1) {
        err = ring_buffer__poll(rb, 100 /*ms*/);
        if (err < 0 && err != -EINTR) {
            fprintf(stderr, "❌ ring_buffer__poll: %d\n", err);
            break;
        }
    }

    ring_buffer__free(rb);
    bpf_link__destroy(link);
    bpf_object__close(obj);
    return 0;
}

