CC = gcc
CFLAGS = -Wall -Wextra `pkg-config --cflags gtk+-3.0`
LDFLAGS = `pkg-config --libs gtk+-3.0`
SOURCES = $(wildcard src/*.c)
OBJECTS = $(SOURCES:.c=.o)
TARGET = build/system_monitor

all: $(TARGET)

$(TARGET): $(OBJECTS)
    $(CC) -o $@ $^ $(LDFLAGS)

%.o: %.c
    $(CC) -c $< -o $@ $(CFLAGS)

clean:
    rm -f src/*.o $(TARGET)
