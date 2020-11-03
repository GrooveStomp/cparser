.DEFAULT_GOAL := help
.PHONY: help test clean

CC=gcc
CFLAGS=-std=c99 -pedantic-errors -fextended-identifiers -g -x c -gdwarf-2 -g3 -Wno-format-security

build: $(OBJ)
	$(CC) $(CFLAGS) -o cparser main.c

test:
	./test

help:
	sh ./sh/view-help README.md

clean:
	rm *.o
	rm cparser
	rm test
