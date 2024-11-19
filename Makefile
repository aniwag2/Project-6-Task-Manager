# Compiler and flags
CC = gcc
CFLAGS = -Wall -Wextra `pkg-config --cflags gtk+-3.0`
LDFLAGS = `pkg-config --libs gtk+-3.0`
SOURCES = $(wildcard src/*.c)
OBJECTS = $(SOURCES:.c=.o)
TARGET = system_monitor

# Default target
all: $(TARGET)

# Link the object files to create the binary
$(TARGET): $(OBJECTS)
	$(CC) -o $@ $^ $(LDFLAGS)

# Compile source files to object files
%.o: %.c
	$(CC) -c $< -o $@ $(CFLAGS)

# Clean up build files
clean:
	rm -f src/*.o $(TARGET)

.PHONY: all clean

