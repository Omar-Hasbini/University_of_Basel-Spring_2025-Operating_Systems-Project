# Metadata
# Author: Omar Fadi Hasbini
# Context: University of Basel, Operating Systems, Spring 2025
# License: Check https://github.com/Omar-Hasbini/University_of_Basel-Spring_2025-Operating_Systems-Project

# Compiler
CC = gcc

# Executable's name
OUT = tagger

# .c files to compile
SRCS = src/tagger.c src/tagdb.c

# Directory with .h files
INCLUDES = -Iinclude

# Libraries
LIBS = -ljson-c

# Default when you run "make" with no additional arguments
all:
	gcc $(SRCS) $(LIBS) $(INCLUDES) -o $(OUT)

# Use with "make clean"
clean:
	rm -f $(OUT)