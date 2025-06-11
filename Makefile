# Compiler
CC = gcc

# Executable's name
OUT = note

# .c files to compile
SRCS = src/note.c src/note_db.c

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
	echo "Installing the notes executable systemwide."
	sudo cp $(OUT) /usr/local/bin/$(OUT)


uninstall:
	echo "Removing the note extension from the system path"
	sudo rm -f /usr/local/bin/$(OUT)

.PHONY: build install clean uninstall

