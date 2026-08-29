# Makefile
CC ?= gcc
CFLAGS ?= -O3 -Wall -Wextra -std=c99 -D_POSIX_C_SOURCE=199309L

# If on macOS, allow mach_time resolution
ifeq ($(shell uname -s), Darwin)
	CFLAGS += -D__MACH__
endif

.PHONY: all clean test bench

all: test_bin bench_bin

vt.o: vt.c vt.h
	$(CC) $(CFLAGS) -c vt.c -o vt.o

test_bin: test.c vt.o
	$(CC) $(CFLAGS) test.c vt.o -o test_bin

bench_bin: bench.c vt.o
	$(CC) $(CFLAGS) bench.c vt.o -o bench_bin

test: test_bin
	./test_bin

bench: bench_bin
	./bench_bin

clean:
	rm -f vt.o test_bin bench_bin