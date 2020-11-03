.DEFAULT_GOAL := build
.PHONY: help test clean

CC=gcc
CFLAGS=-std=c99 -pedantic-errors -fextended-identifiers -g -x c -gdwarf-2 -g3 -Wno-format-security

build: $(OBJ)
	$(CC) $(CFLAGS) -o cparser src/main.c

test:
	$(CC) $(CFLAGS) -o test src/test.c

help:
	@sh ./sh/view-help README.md

clean:
	@rm -f src/*.o cparser test
