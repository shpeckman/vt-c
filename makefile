# makefile
CC ?= gcc
CFLAGS ?= -O3 -Wall -Wextra -std=c99 -D_XOPEN_SOURCE=700 -Iinclude

BIN_DIR = bin

BENCH_CFLAGS = $(CFLAGS) -flto -march=native

ifeq ($(shell uname -s), Darwin)
	CFLAGS += -D__MACH__
	BENCH_CFLAGS += -D__MACH__
endif

.PHONY: all clean test bench daemon

all: test bench daemon clean

$(BIN_DIR):
	@mkdir -p $(BIN_DIR)

$(BIN_DIR)/vt.o: src/vt.c include/vt.h | $(BIN_DIR)
	@$(CC) $(CFLAGS) -c src/vt.c -o $(BIN_DIR)/vt.o

$(BIN_DIR)/vt_test: t/vt_test.c $(BIN_DIR)/vt.o | $(BIN_DIR)
	@$(CC) $(CFLAGS) t/vt_test.c $(BIN_DIR)/vt.o -o $(BIN_DIR)/vt_test

$(BIN_DIR)/vt_bench: t/vt_bench.c src/vt.c include/vt.h | $(BIN_DIR)
	@$(CC) $(BENCH_CFLAGS) src/vt.c t/vt_bench.c -o $(BIN_DIR)/vt_bench

$(BIN_DIR)/vt-daemon: src/vt-daemon.c $(BIN_DIR)/vt.o | $(BIN_DIR)
	@$(CC) $(CFLAGS) src/vt-daemon.c $(BIN_DIR)/vt.o -o $(BIN_DIR)/vt-daemon

test: $(BIN_DIR)/vt_test
	@echo
	./$(BIN_DIR)/vt_test
	@echo

bench: $(BIN_DIR)/vt_bench
	@echo
	./$(BIN_DIR)/vt_bench
	@echo
	
daemon: $(BIN_DIR)/vt-daemon

clean:
	@rm -rf $(BIN_DIR)