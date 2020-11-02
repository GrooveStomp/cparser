# C Parser

## Motivation
My original intent with this project, back in 2015 was to start writing a compiler.

I got to the point of a functioning C parser and then stopped.

Since then, my motivation to write a compiler has almost entirely evaporated; but I still see a lot of value in having _good_ C Parser that I understand; so I intend to maintain this.

## Caveats

Only tested in Linux.

There is no support for the C preprocessor at all.  Any preprocessor commands
are simply ignored, as are comments.

The parser does parse some C programs, like the provided sample.c.
There's a really naive parse tree implementation to aid with debugging.
The `parse' subcommand has a `--show-parse-tree' option that will display this
parse tree.  Warning! It's really big!

## Dependencies

- linux
- gcc
- bash

This might work with other shells and other C compilers, but no effort has been
made to be POSIX shell compliant, nor to follow the C standard. (Specifically
ignoring GNU extensions.)

## Installation

Clone this repo and chdir to it.

## Build / Run

    $ source env/shell
    $ build
    $ ./test # Run tests
    $ ./cparser --help
