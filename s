# Compiler
CC = gcc

# Compiler flags
CFLAGS = -Wall -Wextra -std=c99 -g -Iinclude

# Source files
SRCS = src/tagdb.c src/tagger.c

# Output binary
OUT = tagger
 
 
# Linker flags (use json-c)
LIBS = -ljson-c

# Default target
all: $(OUT)

$(OUT): $(SRCS)
	$(CC) $(CFLAGS) -o $(OUT) $(SRCS) $(LIBS)

clean:
	rm -f $(OUT)
