#include <stdlib.h> /* EXIT_SUCCESS, EXIT_FAILURE */
#include <stdio.h>

#include "string.c"
#include "lexer.c"
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
        printf("Executable tests\n\n");
        printf("Usage: run\n");
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

        printf("Testing parser\n");



        return(EXIT_SUCCESS);
}
