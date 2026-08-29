# Makefile
CC ?= gcc
CFLAGS ?= -O3 -Wall -Wextra -std=c99 -D_POSIX_C_SOURCE=199309L

# Directory for compiled binaries and objects
BIN_DIR = bin

# Aggressive optimizations for benchmarking
BENCH_CFLAGS = $(CFLAGS) -flto -march=native

# If on macOS, allow mach_time resolution
ifeq ($(shell uname -s), Darwin)
	CFLAGS += -D__MACH__
	BENCH_CFLAGS += -D__MACH__
endif

.PHONY: all clean test bench

all: $(BIN_DIR)/test_bin $(BIN_DIR)/bench_bin

# Create the bin directory
$(BIN_DIR):
	mkdir -p $(BIN_DIR)

# The pipe (|) makes the directory an order-only prerequisite, 
# so its timestamp changing doesn't force recompilation.
$(BIN_DIR)/vt.o: vt.c vt.h | $(BIN_DIR)
	$(CC) $(CFLAGS) -c vt.c -o $(BIN_DIR)/vt.o

$(BIN_DIR)/test_bin: test.c $(BIN_DIR)/vt.o | $(BIN_DIR)
	$(CC) $(CFLAGS) test.c $(BIN_DIR)/vt.o -o $(BIN_DIR)/test_bin

# Compile bench and vt together with LTO
$(BIN_DIR)/bench_bin: bench.c vt.c | $(BIN_DIR)
	$(CC) $(BENCH_CFLAGS) vt.c bench.c -o $(BIN_DIR)/bench_bin

test: $(BIN_DIR)/test_bin
	./$(BIN_DIR)/test_bin

bench: $(BIN_DIR)/bench_bin
	./$(BIN_DIR)/bench_bin

clean:
	rm -rf $(BIN_DIR)