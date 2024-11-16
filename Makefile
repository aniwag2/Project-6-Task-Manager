CC = gcc
CFLAGS = -Wall -Wextra `pkg-config --cflags gtk+-3.0`
LDFLAGS = `pkg-config --libs gtk+-3.0`
SOURCES = $(wildcard src/*.c)
OBJECTS = $(SOURCES:.c=.o)
TARGET = system_monitor

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) -o $@ $^ $(LDFLAGS)  # Indent this line with a TAB

%.o: %.c
	$(CC) -c $< -o $@ $(CFLAGS)  # Indent this line with a TAB

clean:
	rm -f src/*.o $(TARGET)  # Indent this line with a TAB

