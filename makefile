# makefile
CC ?= gcc
CFLAGS ?= -O3 -Wall -Wextra -std=c99 -D_XOPEN_SOURCE=700 -Iinclude

PREFIX ?= /usr/local
BIN_DIR = bin
SYSTEMD_DIR ?= /etc/systemd/system

BENCH_CFLAGS = $(CFLAGS) -flto -march=native

ifeq ($(shell uname -s), Darwin)
	CFLAGS += -D__MACH__
	BENCH_CFLAGS += -D__MACH__
endif

.PHONY: all clean test bench daemon install uninstall

all: install

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

install: daemon
	@sudo install -d $(DESTDIR)$(PREFIX)/bin
	@sudo install -m 755 $(BIN_DIR)/vt-daemon $(DESTDIR)$(PREFIX)/bin/vt-daemon
	@if [ -d $(DESTDIR)$(SYSTEMD_DIR) ] || [ -z "$(DESTDIR)" ]; then \
		sudo install -d $(DESTDIR)$(SYSTEMD_DIR); \
		sudo install -m 644 vt-daemon.service $(DESTDIR)$(SYSTEMD_DIR)/vt-daemon.service; \
	fi

uninstall:
	@rm -f $(DESTDIR)$(PREFIX)/bin/vt-daemon
	@rm -f $(DESTDIR)$(SYSTEMD_DIR)/vt-daemon.service

clean:
	@rm -rf $(BIN_DIR)