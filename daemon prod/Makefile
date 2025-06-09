# Makefile for ring-buffer based filetrace daemon

BPF_CLANG ?= clang
BPF_CFLAGS = -O2 -g -Wall -target bpf -D__TARGET_ARCH_x86

all: filetrace.bpf.o daemon

filetrace.bpf.o: filetrace.bpf.c
	$(BPF_CLANG) $(BPF_CFLAGS) -c $< -o $@

daemon: daemon.c
	cc -g -Wall -I/usr/include/bpf \
	   -o daemon daemon.c \
	   -lbpf -lelf -lz

clean:
	rm -f filetrace.bpf.o daemon

