all: trace.bpf.o trace_user

trace.bpf.o: trace.bpf.c
	clang -O2 -g -target bpf -D__TARGET_ARCH_x86 -c $< -o $@

trace_user: trace_user.c
	cc -g -Wall -o $@ $^ -lbpf

clean:
	rm -f trace_user trace.bpf.o

