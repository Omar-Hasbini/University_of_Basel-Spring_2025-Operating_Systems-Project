# Makefile for both daemons: filetrace (BPF-based) and downloads-origin

BPF_CLANG    ?= clang
BPF_CFLAGS   = -O2 -g -Wall -target bpf -D__TARGET_ARCH_x86

CC           = cc
CFLAGS       = -g -Wall
BPF_INCLUDES = -I/usr/include/bpf
LDFLAGS_BPF  = -lbpf -lelf -lz
LDFLAGS_DL   = -lsqlite3

all: filetrace.bpf.o daemon daemon_download_origin

filetrace.bpf.o: filetrace.bpf.c
	$(BPF_CLANG) $(BPF_CFLAGS) -c $< -o $@

daemon: daemon.c
	$(CC) $(CFLAGS) $(BPF_INCLUDES) -o $@ $< $(LDFLAGS_BPF)

daemon_download_origin: daemon_download_origin.c
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS_DL)

clean:
	rm -f filetrace.bpf.o daemon daemon_download_origin

