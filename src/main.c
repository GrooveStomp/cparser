/******************************************************************************
 * File: main.c
 * Created: 2016-05-18
 * Updated: 2016-11-16
 * Package: C-Parser
 * Creator: Aaron Oman (GrooveStomp)
 * Homepage: https://git.sr.ht/~groovestomp/c-parser
 * Copyright 2016 - 2020, Aaron Oman and the C-Parser contributors
 * SPDX-License-Identifier: LGPL-3.0-only
 ******************************************************************************/
#include "gs.h"
#include "lexer.c"
#include "parser.c"
#include "ast.c"

#include <stdlib.h> /* EXIT_SUCCESS, EXIT_FAILURE */
#include <stdio.h>
#include <string.h> // strerror
#include <sys/stat.h>
#include <errno.h>

void Usage(const char *name) {
        printf("Usage: %s operation file [options]\n", name);
        puts("  operation: One of: [parse, lex].");
        puts("  file: Must be a file in this directory.");
        puts("  Specify '-h' or '--help' for this help text.");
        exit(EXIT_SUCCESS);
}

int main(int argc, char **argv) {
        const char *prog_name = argv[0];

        for (int i = 0; i < argc; i++) {
                if (gs_StringIsEqual(argv[i], "--help", 6) ||
                    gs_StringIsEqual(argv[i], "-help", 5) ||
                    gs_StringIsEqual(argv[i], "-h", 2))
                        Usage(prog_name);
        }

        if (argc < 3) Usage(prog_name);

        char *command = argv[1];

        if (!gs_StringIsEqual(command, "parse", 5) &&
            !gs_StringIsEqual(command, "lex", 3))
                Usage(prog_name);

        char *filename = argv[2];
        struct stat stat_buf;
        if (stat(filename, &stat_buf) != 0) {
                fprintf(stderr, "%s\n", strerror(errno));
                exit(EXIT_FAILURE);
        }

        char *buf_start = (char *)malloc(stat_buf.st_size);
        gs_Buffer buffer;
        gs_BufferInit(&buffer, buf_start, stat_buf.st_size);

        FILE *file = fopen(filename, "r");
        if (file == NULL) {
                fprintf(stderr, "%s\n", strerror(errno));
                exit(EXIT_FAILURE);
        }

        size_t bytes_read = fread(buffer.cursor, 1, stat_buf.st_size, file);
        buffer.length += stat_buf.st_size;
        buffer.cursor += stat_buf.st_size;
        *(buffer.cursor) = '\0';

        fclose(file);

        gs_Allocator allocator = { .malloc = malloc, .free = free, .realloc = realloc, .calloc = calloc };

        if (gs_StringIsEqual(command, "parse", 5)) {
                ParseTreeNode *parse_tree;
                Tokenizer tokenizer;
                if (Parse(allocator, &buffer, &parse_tree, &tokenizer)) {
                        ParseTreePrint(parse_tree, 0, 2, printf);
                } else {
                        printf("Input did not parse @ [%d,%d]\n", tokenizer.line, tokenizer.column);
                }
        } else {
                Token *token_stream;
                u32 num_tokens;
                if (Lex(allocator, &buffer, &token_stream, &num_tokens)) {
                        for (int i = 0; i < num_tokens; i++) {
                                Token token = token_stream[i];

                                printf("[%u,%u] Token Name: %20s, Token Text: %.*s\n",
                                       token.line + 1,
                                       token.column,
                                       TokenName(token.type),
                                       (u32)(token.text_length),
                                       token.text);
                        }
                } else {
                        fprintf(stderr, LexerErrorString());
                }
        }

        return EXIT_SUCCESS;
}
