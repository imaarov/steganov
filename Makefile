CC = gcc

CFLAGS = -DENABLE_DEBUG -O -Wall -Wextra -Iinclude -fsanitize=address -g -O0

CLANG_TIDY = clang-tidy

TARGET = bin/steganov

SRC := $(filter-out src/crc32.c,$(wildcard src/*.c))

OBJS := \
	$(SRC:src/%.c=build/%.o) \
	build/crc32.o

TESTSRC := $(wildcard tests/*.c)
TEST_OBJS := $(TESTSRC:tests/%.c=build/tests/%.o)

TIDY_FILES := $(SRC) $(TESTSRC)

all: $(TARGET)

$(TARGET): $(OBJS)
	@mkdir -p bin
	$(CC) $(CFLAGS) -o $@ $(OBJS)

build/%.o: src/%.c
	@mkdir -p build
	$(CC) $(CFLAGS) -c $< -o $@

build/crc32.o: src/crc32.s
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

tidy:
	$(CLANG_TIDY) -p . -config-file=.clang-tidy $(TIDY_FILES)

tidy-strict:
	$(CLANG_TIDY) -p . -config-file=.strict-clang-tidy $(TIDY_FILES)

format:
	find . -type f \( -name '*.c' -o -name '*.h' \) -exec clang-format -i {} +

.PHONY: all test run-test clean tidy tidy-strict format
