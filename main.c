#include <stdlib.h> /* EXIT_SUCCESS, EXIT_FAILURE */
#include <stdio.h>
#include <alloca.h>

#include "gs.h"
#include "lexer.c"
#include "parser.c"

void
Usage(char *Name)
{
        printf("Usage: %s operation file\n", Name);
        printf("  operation: One of: [parse, lex].\n");
        printf("  file: Must be a file in this directory.\n");
        printf("  Specify '-h' or '--help' for this help text.\n");
        exit(EXIT_SUCCESS);
}

int
main(int ArgCount, char **Arguments)
{
        gs_args Args;
        GSArgsInit(&Args, ArgCount, Arguments);
        if(GSArgsHelpWanted(&Args)           ||
           ArgCount != 3                     ||
           (!GSArgsIsPresent(&Args, "parse") &&
            !GSArgsIsPresent(&Args, "lex")))
                Usage(GSArgsProgramName(&Args));

        gs_buffer Buffer;
        char *Filename = GSArgsAtIndex(&Args, 2);
        size_t AllocSize = GSFileSize(Filename);
        GSBufferInit(&Buffer, alloca(AllocSize), AllocSize);

        if(!GSFileCopyToBuffer(Filename, &Buffer))
                GSAbortWithMessage("Couldn't copy entire file to buffer\n");

        if(GSArgsIsPresent(&Args, "parse"))
                Parse(&Buffer);
        else
                Lex(&Buffer);

        return(EXIT_SUCCESS);
}
