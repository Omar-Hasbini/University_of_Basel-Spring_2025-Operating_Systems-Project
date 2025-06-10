/* filetrace.bpf.c */
#include "vmlinux.h"            /* Kernel internal type definitions */
#include <bpf/bpf_helpers.h>     /* BPF helper macros and functions */
#include <bpf/bpf_tracing.h>     /* Tracepoint and kprobe definitions */

char LICENSE[] SEC("license") = "GPL";  /* Declare GPL license for the BPF program */

/*
 * Event structure passed from kernel to user-space via ring buffer:
 * - pid: Process ID of the creator
 * - filename: User-supplied filename passed to openat syscall
 */
struct event_t {
    u32 pid;
    char filename[256];
};

/*
 * Define a ring buffer map named "events" with 16 MiB capacity.
 * User-space daemon will poll this buffer to receive file creation events.
 */
struct {
    __uint(type, BPF_MAP_TYPE_RINGBUF);
    __uint(max_entries, 1 << 24); // 16 MiB
} events SEC(".maps");

/*
 * Attach to the sys_enter_openat tracepoint. Trigger only when O_CREAT flag is used.
 * We capture the process ID and filename argument, then submit to ring buffer.
 */
SEC("tracepoint/syscalls/sys_enter_openat")
int handle_openat(struct trace_event_raw_sys_enter *ctx) {
    /* Extract the 'flags' and 'filename' parameters from tracepoint arguments */
    int flags = (int)ctx->args[2];
    const char *fname = (const char *)ctx->args[1];

    /* Only proceed if O_CREAT bit is set in flags (octal 0100) */
    if (!(flags & 0100))
        return 0;

    /* Reserve space in the ring buffer for our event structure */
    struct event_t *e = bpf_ringbuf_reserve(&events, sizeof(*e), 0);
    if (!e)
        return 0;

    /* Fill in the event data: high 32 bits of pid_tgid is the PID */
    e->pid = bpf_get_current_pid_tgid() >> 32;

    /* Safely copy the filename from user-space to our event buffer */
    bpf_probe_read_user_str(e->filename,
                            sizeof(e->filename), fname);

    /* Submit the filled event into the ring buffer for user-space to read */
    bpf_ringbuf_submit(e, 0);
    return 0;
}

