#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/resource.h>
#include <sys/time.h>
#include <sys/xattr.h>
#include <linux/bpf.h>
#include <bpf/libbpf.h>
#include <bpf/bpf.h>

#define MAX_FILENAME_LEN 256

struct event {
    __u32 pid;
    char filename[MAX_FILENAME_LEN];
};

static int handle_event(void *ctx, void *data, size_t data_sz) {
    struct event *e = data;

    if (e->filename[0] == '\0') {
        fprintf(stderr, "⚠️  Empty filename from PID %d — skipping\n", e->pid);
        return 0;
    }

    printf("🧠 Event received: PID=%d, filename=%s\n", e->pid, e->filename);

    char resolved_path[PATH_MAX];
    if (realpath(e->filename, resolved_path) == NULL) {
        fprintf(stderr, "⚠️  realpath failed: %s\n", strerror(errno));
        return 0;
    }

    printf("📍 Resolved path: %s\n", resolved_path);

    // Wait for file to exist (up to 5ms)
    int retries = 5;
    while (retries-- > 0 && access(resolved_path, F_OK) != 0) {
        usleep(1000); // 1 ms
    }

    if (access(resolved_path, F_OK) != 0) {
        fprintf(stderr, "❌ File still does not exist: %s\n", resolved_path);
        return 0;
    }

    const char *origin = "local";
    if (setxattr(resolved_path, "user.origin", origin, strlen(origin), 0) == 0) {
        printf("✅ Origin set: %s → %s\n", resolved_path, origin);
    } else {
        fprintf(stderr, "❌ setxattr failed on %s: %s\n", resolved_path, strerror(errno));
    }

    return 0;
}

static int libbpf_debug(enum libbpf_print_level level, const char *fmt, va_list args) {
    return vfprintf(stderr, fmt, args);
}

int main() {
    struct bpf_object *obj;
    struct bpf_program *prog;
    struct bpf_map *map;
    int map_fd, err;

    libbpf_set_print(libbpf_debug);

    printf("🚀 Opening BPF object...\n");
    obj = bpf_object__open_file("filetrace.bpf.o", NULL);
    if (!obj) {
        perror("❌ Failed to open BPF object");
        return 1;
    }

    err = bpf_object__load(obj);
    if (err) {
        fprintf(stderr, "❌ Failed to load BPF object: %s\n", strerror(-err));
        return 1;
    }

    prog = bpf_object__find_program_by_name(obj, "handle_openat");
    if (!prog) {
        fprintf(stderr, "❌ Failed to find program by name\n");
        return 1;
    }

    struct bpf_link *link = bpf_program__attach(prog);
    if (!link) {
        fprintf(stderr, "❌ Failed to attach BPF program\n");
        return 1;
    }

    map = bpf_object__find_map_by_name(obj, "events");
    if (!map) {
        fprintf(stderr, "❌ Failed to find map\n");
        return 1;
    }

    map_fd = bpf_map__fd(map);
    if (map_fd < 0) {
        perror("❌ Failed to get map fd");
        return 1;
    }

    struct ring_buffer *rb = ring_buffer__new(map_fd, handle_event, NULL, NULL);
    if (!rb) {
        fprintf(stderr, "❌ Failed to create ring buffer\n");
        return 1;
    }

    printf("✅ Daemon started. Waiting for file creation events...\n");

    while (1) {
        err = ring_buffer__poll(rb, 100 /* timeout ms */);
        if (err < 0) {
            fprintf(stderr, "❌ Error polling ring buffer: %d\n", err);
            break;
        }
    }

    ring_buffer__free(rb);
    bpf_link__destroy(link);
    bpf_object__close(obj);

    printf("👋 Exiting...\n");
    return 0;
}

