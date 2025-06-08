# Compiler
CC = gcc

# Compiler flags
CFLAGS = -Wall -Wextra -std=c99 -g -Iinclude

# Source files
SRCS = src/tagdb.c src/tagger.c

# Object files (one .o per .c)
OBJS = src/tagdb.o src/tagger.o

# Output binary
OUT = tagger

# Linker flags
LIBS = -ljson-c

# Default target
all: $(OUT)

# Build rule for the output binary
$(OUT): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS)

# Compile each .c to .o (implicit rule)
src/%.o: src/%.c include/tagdb.h
	$(CC) $(CFLAGS) -c -o $@ $<

# Clean target to delete all build outputs
clean:
	rm -f $(OUT) src/*.o