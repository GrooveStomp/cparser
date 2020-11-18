.DEFAULT_GOAL := release
.PHONY: help test clean release debug

# NOTE: Not using -Wpedantic because of GCC-specific expression statements.

CC=gcc
CFLAGS=-std=c99 -x c -Wno-format-security
RELEASE_CFLAGS=-O2
DEBUG_CFLAGS=-gdwarf-4 -g3 -fvar-tracking-assignments
EXE=cparser

debug:
	$(CC) $(CFLAGS) $(DEBUG_CFLAGS) -o $(EXE) src/main.c

release:
	$(CC) $(CFLAGS) $(RELEASE_CFLAGS) -o $(EXE) src/main.c

test:
	$(CC) $(CFLAGS) -o test src/test.c

help:
	@sh ./sh/view-help README.md

clean:
	@rm -f src/*.o $(EXE) $(EXE) test
