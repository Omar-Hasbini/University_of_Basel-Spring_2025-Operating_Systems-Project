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
build:
	echo "Compiling into the executable"
	$(CC) $(SRCS) $(LIBS) $(INCLUDES) -o $(OUT)
# EOC

clean:
	rm -f $(OUT) *.o

install: build
	echo "Installing the tagger executable systemwide."
	sudo cp $(OUT) /usr/local/bin/$(OUT)
	echo "Installing the man page."
	sudo cp ./man/tagger.1 /usr/share/man/man1/
#   Forces overwrite if compressed file already exists
	sudo gzip -f /usr/share/man/man1/tagger.1


uninstall:
	echo "Removing the tagger extension from the system path"
	sudo rm -f /usr/local/bin/$(OUT)
	echo "Removing the man page."
	sudo rm -f /usr/share/man/man1/tagger.1.gz

.PHONY: build install clean uninstall