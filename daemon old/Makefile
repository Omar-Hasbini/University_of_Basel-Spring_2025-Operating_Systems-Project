all:
	clang -O2 -g -Wall -target bpf -D__TARGET_ARCH_x86 -c filetrace.bpf.c -o filetrace.bpf.o
	cc -g -Wall -o daemon daemon.c -lbpf -lelf -lz

clean:
	rm -f filetrace.bpf.o daemon

