CC = gcc
INLINE_FLAGS = -g3 -std=gnu99 -Wall -Wextra -Wpointer-arith -Wpedantic -O2
CFLAGS = $(INLINE_FLAGS) -Werror

# ---- Files ----
CORE_SRC   = src/mini_core.c
HELP_SRC   = src/mini_helpers.c
TEST_SRC   = src/tester.c

OBJS   = $(CORE_SRC:.c=.o) $(HELP_SRC:.c=.o)
BIN    = build/tester

.PHONY: all clean test run

all: $(BIN)

build:
	@mkdir -p build

$(BIN): build $(OBJS) $(TEST_SRC) include/mini.h include/mini_helpers.h
	$(CC) $(CFLAGS) $(INCLUDES) $(OBJS) $(TEST_SRC) -o $(BIN)

# Generic object rule
%.o: %.c
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

test: $(BIN)
	$(BIN)

run: test

clean:
	rm -rf build $(OBJS)