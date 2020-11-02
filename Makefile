.DEFAULT_GOAL := help
.PHONY: help build test

CC=gcc
CFLAGS=-std=c99 -pedantic-errors -fextended-identifiers -g -x c -gdwarf-2 -g3 -Wno-format-security

build:
	$(CC) $(CFLAGS) -o cparser main.c
	$(CC) $(CFLAGS) -o test test.c

test:
	./test

help:
	sh ./sh/view-help README.md
