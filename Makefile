cc=gcc
CFLAGS= -DENABLE_DEBUG -O -Wall -Wextra -Iinclude
TARGET=bin/steganov
SRC=$(wildcard src/*.c)
OBJS=$(SRC:src/%.c=build/%.o)
all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS)

build/%.o: src/%.c
	@mkdir -p build
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf build
