/******************************************************************************
 * File: main.c
 * Created:
 * Updated: 2016-11-03
 * Package: C-Parser
 * Creator: Aaron Oman (GrooveStomp)
 * Copyright - 2020, Aaron Oman and the C-Parser contributors
 * SPDX-License-Identifier: LGPL-3.0-only
 ******************************************************************************/
#include "gs.h"
#include "lexer.c"
#include "parser.c"

 #include <stdlib.h> /* EXIT_SUCCESS, EXIT_FAILURE */
#include <stdio.h>
#include <string.h> // strerror
#include <sys/stat.h>
#include <errno.h>

void Usage(const char *Name) {
        printf("Usage: %s operation file [options]\n", Name);
        puts("  operation: One of: [parse, lex].");
        puts("  file: Must be a file in this directory.");
        puts("  Specify '-h' or '--help' for this help text.");
        puts("");
        puts("Options:");
        puts("  --show-parse-tree: Valid with `parse' subcommand.");
        exit(EXIT_SUCCESS);
}

int main(int argc, char **argv) {
        const char *ProgName = argv[0];

        for (int i = 0; i < argc; i++) {
                if (GSStringIsEqual(argv[i], "--help", 6) ||
                    GSStringIsEqual(argv[i], "-help", 5) ||
                    GSStringIsEqual(argv[i], "-h", 2))
                        Usage(ProgName);
        }

        if (argc < 3) Usage(ProgName);

        char *Command = argv[1];

        if (!GSStringIsEqual(Command, "parse", 5) &&
            !GSStringIsEqual(Command, "lex", 3))
                Usage(ProgName);

        char *Filename = argv[2];
        struct stat StatBuf;
        if (stat(Filename, &StatBuf) != 0) {
                fprintf(stderr, strerror(errno));
                exit(EXIT_FAILURE);
        }

        char *BufStart = (char *)malloc(StatBuf.st_size);
        gs_buffer Buffer;
        GSBufferInit(&Buffer, BufStart, StatBuf.st_size);

        FILE *File = fopen(Filename, "r");
        if (File == NULL) {
                fprintf(stderr, strerror(errno));
                exit(EXIT_FAILURE);
        }

        size_t BytesRead = fread(Buffer.Cursor, 1, StatBuf.st_size, File);
        Buffer.Length += StatBuf.st_size;
        Buffer.Cursor += StatBuf.st_size;
        *(Buffer.Cursor) = '\0';

        fclose(File);

        gs_allocator Allocator = {
                .Alloc = malloc,
                .Free = free,
                .Realloc = realloc,
                .ArrayAlloc = calloc,
        };

        if (GSStringIsEqual(Command, "parse", 5)) {
                bool ShowParseTree = (argc == 4 && GSStringIsEqual(argv[3], "--show-parse-tree", 17));
                Parse(Allocator, &Buffer, ShowParseTree);
        } else {
                Lex(&Buffer);
        }

        return EXIT_SUCCESS;
}
