#include <stdlib.h> /* EXIT_SUCCESS, EXIT_FAILURE */
#include <stdio.h>
#include <alloca.h>
#include "parser.c"

void
AbortWithMessage(const char *msg)
{
        fprintf(stderr, "%s\n", msg);
        exit(EXIT_FAILURE);
}

void
Usage()
{
        printf("Usage: run operation file\n");
        printf("  operation: One of: [parse, lex].\n");
        printf("  file: Must be a file in this directory.\n");
        printf("  Specify '-h' or '--help' for this help text.\n");
        exit(EXIT_SUCCESS);
}

int
main(int ArgCount, char **Args)
{
        for(int Index = 0; Index < ArgCount; ++Index)
        {
                if(IsStringEqual(Args[Index], "-h", StringLength("-h")) ||
                   IsStringEqual(Args[Index], "--help", StringLength("--help")))
                {
                        Usage();
                }
        }
        if(ArgCount != 3) Usage();

        if(!IsStringEqual(Args[1], "parse", StringLength("parse")) &&
           !IsStringEqual(Args[1], "lex", StringLength("lex")))
        {
                Usage();
        }

        size_t AllocSize = FileSize(Args[2]);
        struct buffer FileContents;

        /* Allocate space on the stack. */
        BufferSet(&FileContents, (char *)alloca(AllocSize), 0, AllocSize);

        if(!CopyFileIntoBuffer(Args[2], &FileContents))
        {
                AbortWithMessage("Couldn't copy entire file to buffer");
        }

        if(IsStringEqual(Args[1], "parse", StringLength("parse")))
        {
                Parse(&FileContents);
        }
        else
        {
                Lex(&FileContents);
        }

        return(EXIT_SUCCESS);
}
