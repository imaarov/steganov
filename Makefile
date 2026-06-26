cc=gcc
CFLAGS= -DENABLE_DEBUG -O -Wall -Wextra -Iinclude

TARGET=bin/steganov

SRC=$(wildcard src/*.c)
OBJS=$(SRC:src/%.c=build/%.o)

TESTSRC=$(wildcard tests/*.c)
TEST_OBJS=$(TESTSRC:tests/%.c=build/tests/%.o)

all: $(TARGET)

$(TARGET): $(OBJS)
	@mkdir -p bin
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS)

build/%.o: src/%.c
	@mkdir -p build
	$(CC) $(CFLAGS) -c $< -o $@

build/tests/%.o: tests/%.c
	@mkdir -p build/tests
	$(CC) $(CFLAGS) -c $< -o $@

test: $(TEST_OBJS) $(filter-out build/steganov.o,$(OBJS))
	@mkdir -p bin
	$(CC) $(CFLAGS) -o bin/test_runner \
	$(TEST_OBJS) \
	$(filter-out build/steganov.o,$(OBJS))

run-test: test
	./bin/test_runner

clean:
	rm -rf build bin/steganov bin/test_runner


.PHONY: format test clean all
format:
	find . -type f \( -name '*.c' -o -name '*.h' \) -exec clang-format -i {} +
