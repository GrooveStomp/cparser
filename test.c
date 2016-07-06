#include <stdlib.h> /* EXIT_SUCCESS, EXIT_FAILURE */
#include <stdio.h>

#include "string.c"
#include "lexer.c"
#include "parser.c"

#define Assert(Description, Expression) AssertFn((Description), (Expression), __LINE__, __func__)

void AssertFn(const char *Description, int Expression, int LineNumber, const char *FuncName)
{
        if(!Expression)
        {
                if(Description != NULL)
                {
                        fprintf(stderr, "Assertion failed in %s at line #%d: %s\n", FuncName, LineNumber, Description);
                }
                else
                {
                        fprintf(stderr, "Assertion failed in %s at line #%d\n", FuncName, LineNumber);
                }
                exit(EXIT_FAILURE);
        }
}

void
AbortWithMessage(const char *msg)
{
        fprintf(stderr, "%s\n", msg);
        exit(EXIT_FAILURE);
}

struct tokenizer
InitTokenizer(char *String)
{
        struct tokenizer Tokenizer;
        Tokenizer.Beginning = Tokenizer.At = String;
        Tokenizer.Line = Tokenizer.Column = 1;
        return(Tokenizer);
}

void
TestUnaryExpression()
{
        struct tokenizer Tokenizer;
        bool Result;
        char *String;

        String = "sizeof ( int )"; /* identifier | constant | string )"; */
        Tokenizer = InitTokenizer(String);
        Result = ParseUnaryExpression(&Tokenizer);
        Assert("Result should be true", Result == true);
        Assert("Tokenizer advances to end of string", Tokenizer.At == String + StringLength(String));
}

void
TestPrimaryExpression()
{
        struct tokenizer Tokenizer;
        bool Result;
        char *String;

        String = "my_var";
        Tokenizer = InitTokenizer(String);
        Result = ParsePrimaryExpression(&Tokenizer);
        Assert("Result should be true", Result == true);
        Assert("Tokenizer advances to end of string", Tokenizer.At == String + StringLength(String));

        String = "4.0";
        Tokenizer = InitTokenizer(String);
        Result = ParsePrimaryExpression(&Tokenizer);
        Assert("Result should be true", Result == true);
        Assert("Tokenizer advances to end of string", Tokenizer.At == String + StringLength(String));

        String = "\"This is a string\"";
        Tokenizer = InitTokenizer(String);
        Result = ParsePrimaryExpression(&Tokenizer);
        Assert("Result should be true", Result == true);
        Assert("Tokenizer advances to end of string", Tokenizer.At == String + StringLength(String));

        String = "( sizeof(int) )";
        Tokenizer = InitTokenizer(String);
        Result = ParsePrimaryExpression(&Tokenizer);
        Assert("Result should be true", Result == true);
        Assert("Tokenizer advances to end of string", Tokenizer.At == String + StringLength(String));
}

/*----------------------------------------------------------------------------
  Main Entrypoint
  ----------------------------------------------------------------------------*/

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

        TestUnaryExpression();
        TestPrimaryExpression();

        printf("All tests passed successfully\n");
        return(EXIT_SUCCESS);
}
