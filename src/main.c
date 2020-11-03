/******************************************************************************
 * File: main.c
 * Created:
 * Updated: 2016-11-03
 * Package: C-Parser
 * Creator: Aaron Oman (GrooveStomp)
 * Copyright - 2020, Aaron Oman and the C-Parser contributors
 * SPDX-License-Identifier: LGPL-3.0-only
 ******************************************************************************/
#include <stdlib.h> /* EXIT_SUCCESS, EXIT_FAILURE */
#include <stdio.h>
#include <alloca.h>

#include "gs.h"
#include "lexer.c"
#include "parser.c"

void Usage(char *Name) {
        printf("Usage: %s operation file [options]\n", Name);
        puts("  operation: One of: [parse, lex].");
        puts("  file: Must be a file in this directory.");
        puts("  Specify '-h' or '--help' for this help text.");
        puts("");
        puts("Options:");
        puts("  --show-parse-tree: Valid with `parse' subcommand.");
        exit(EXIT_SUCCESS);
}

int main(int ArgCount, char **Arguments) {
        gs_args Args;
        GSArgsInit(&Args, ArgCount, Arguments);
        if (GSArgsHelpWanted(&Args)           ||
           ArgCount < 3                      ||
           (!GSArgsIsPresent(&Args, "parse") &&
            !GSArgsIsPresent(&Args, "lex")))
                Usage(GSArgsProgramName(&Args));

        gs_buffer Buffer;
        char *Filename = GSArgsAtIndex(&Args, 2);
        size_t AllocSize = GSFileSize(Filename);
        GSBufferInit(&Buffer, alloca(AllocSize), AllocSize);

        if (!GSFileCopyToBuffer(Filename, &Buffer))
                GSAbortWithMessage("Couldn't copy entire file to buffer\n");

        if (GSArgsIsPresent(&Args, "parse"))
                Parse(&Buffer, GSArgsIsPresent(&Args, "--show-parse-tree"));
        else
                Lex(&Buffer);

        return EXIT_SUCCESS;
}
