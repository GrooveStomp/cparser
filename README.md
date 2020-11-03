# C Parser

## Motivation
Provide a library to interact with C source code.  This might include writing a
full-blown compiler, building a language server, or manipulating source code,
integrating with an IDE, better debugging facilities, or any of a number of
other possibilities.

## Caveats
Only tested in Linux.

There is no support for the C preprocessor at all.  Any preprocessor commands
are simply ignored, as are comments.

The parser does parse some C programs, like the provided sample.c.

## Dependencies
- gcc
  This is explicitly invoked in the Makefile, but I also test with tcc and pcc.

- posix-compliant shell

## Build
These commands output an executable named `cparser`

    $ make debug
    $ make release

Other targets:

    $ make test # Build test executable
    $ make help # Show this help on the CLI
