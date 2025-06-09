#include <bpf/libbpf.h>
#include <stdio.h>
#include <unistd.h>
#include <signal.h>

static volatile sig_atomic_t exiting = 0;

void handle_signal(int sig) {
    exiting = 1;
}

int main() {
    struct bpf_object *obj;
    struct bpf_program *prog;
    struct bpf_link *link = NULL;
    int err;

    signal(SIGINT, handle_signal);
    signal(SIGTERM, handle_signal);

    obj = bpf_object__open_file("trace.bpf.o", NULL);
    if (!obj) {
        fprintf(stderr, "❌ Failed to open BPF object\n");
        return 1;
    }

    err = bpf_object__load(obj);
    if (err) {
        fprintf(stderr, "❌ Failed to load BPF object: %d\n", err);
        return 1;
    }

    prog = bpf_object__find_program_by_name(obj, "handle_execve");
    if (!prog) {
        fprintf(stderr, "❌ Failed to find BPF program 'handle_execve'\n");
        return 1;
    }

    link = bpf_program__attach_tracepoint(prog, "syscalls", "sys_enter_execve");
    if (!link) {
        fprintf(stderr, "❌ Failed to attach tracepoint\n");
        return 1;
    }

    printf("✅ Tracepoint attached. Press Ctrl+C to exit.\n");

    while (!exiting) {
        sleep(1);
    }

    bpf_link__destroy(link);
    bpf_object__close(obj);
    return 0;
}

