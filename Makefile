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
	echo "Compiling into the executable"
	$(CC) $(SRCS) $(LIBS) $(INCLUDES) -o $(OUT)
#

clean:
	rm -f $(OUT) *.o

install: all
	echo "Installing the extension systemwide"
	sudo cp $(OUT) /usr/local/bin/$(OUT)

uninstall:
	echo "Removing the tagger extension from the system path"
	sudo rm -f /usr/local/bin/$(OUT)