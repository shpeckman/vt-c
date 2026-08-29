# makefile
CC ?= gcc
CFLAGS ?= -O3 -Wall -Wextra -std=c99 -D_POSIX_C_SOURCE=199309L

BIN_DIR = bin

BENCH_CFLAGS = $(CFLAGS) -flto -march=native

ifeq ($(shell uname -s), Darwin)
	CFLAGS += -D__MACH__
	BENCH_CFLAGS += -D__MACH__
endif

.PHONY: all clean test bench cli

all: test bench cli clean

$(BIN_DIR):
	@mkdir -p $(BIN_DIR)

$(BIN_DIR)/vt.o: vt.c vt.h | $(BIN_DIR)
	@$(CC) $(CFLAGS) -c vt.c -o $(BIN_DIR)/vt.o

$(BIN_DIR)/test_bin: test.c $(BIN_DIR)/vt.o | $(BIN_DIR)
	@$(CC) $(CFLAGS) test.c $(BIN_DIR)/vt.o -o $(BIN_DIR)/test_bin

$(BIN_DIR)/bench_bin: bench.c vt.c | $(BIN_DIR)
	@$(CC) $(BENCH_CFLAGS) vt.c bench.c -o $(BIN_DIR)/bench_bin

$(BIN_DIR)/cli_bin: cli.c $(BIN_DIR)/vt.o | $(BIN_DIR)
	@$(CC) $(CFLAGS) cli.c $(BIN_DIR)/vt.o -o $(BIN_DIR)/cli_bin

test: $(BIN_DIR)/test_bin
	@echo
	./$(BIN_DIR)/test_bin
	@echo

bench: $(BIN_DIR)/bench_bin
	@echo
	./$(BIN_DIR)/bench_bin
	@echo
	
cli: $(BIN_DIR)/cli_bin

clean:
	@rm -rf $(BIN_DIR)