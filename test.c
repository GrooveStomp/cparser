#include <stdlib.h> /* EXIT_SUCCESS, EXIT_FAILURE */
#include <stdio.h>
#include <alloca.h>

#include "string.c"
#include "lexer.c"
#include "parser.c"

typedef bool (*parser_function)(struct tokenizer *);

#define Assert(Description, Expression) AssertFn((Description), (Expression), __LINE__, __func__)
#define Test(Function, String) \
        { \
                struct tokenizer Tokenizer = InitTokenizer((String)); \
                bool Result = Function(&Tokenizer); \
                AssertFn("Result should be true", Result == true, __LINE__, __func__); \
                AssertFn("Tokenizer advances to end of string", Tokenizer.At == (String) + StringLength((String)), __LINE__, __func__); \
        }

void
AssertFn(const char *Description, int Expression, int LineNumber, const char *FuncName)
{
        if(!Expression)
        {
                char *Suffix = "";
                if(Description != NULL)
                {
                        size_t AllocSize = StringLength((char *)Description) + StringLength(": ");
                        Suffix = (char *)alloca(AllocSize);
                        snprintf(Suffix, AllocSize + 1, ": %s", Description);
                }
                fprintf(stderr, "Assertion failed in %s() at line #%d%s\n", FuncName, LineNumber, Suffix);

                exit(EXIT_FAILURE);
        }
}

struct tokenizer
InitTokenizer(char *String)
{
        struct tokenizer Tokenizer;
        Tokenizer.Beginning = Tokenizer.At = String;
        Tokenizer.Line = Tokenizer.Column = 1;
        return(Tokenizer);
}

/*----------------------------------------------------------------------------
  Tests
  ----------------------------------------------------------------------------*/

void
TestConstant()
{
        parser_function Fn = ParseConstant;
        Test(Fn, "1");   /* integer-constant */
        Test(Fn, "'c'"); /* character-constant */
        Test(Fn, "2.0"); /* floating-constant */
        /* TODO(AARON): enumeration-constant */
}

void
TestArgumentExpressionList()
{
        parser_function Fn = ParseArgumentExpressionList;
        Test(Fn, "(int)4.0");           /* assignment-expression */
        Test(Fn, "(int)4.0, (int)4.0"); /* argument-expression-list , assignment-expression */
}

void
TestUnaryExpression()
{
        parser_function Fn = ParseUnaryExpression;
        Test(Fn, "\"String\"");    /* postfix-expression */
        Test(Fn, "++Foo");         /* ++ unary-expression */
        Test(Fn, "--Foo");         /* -- unary-expression */
        Test(Fn, "&(int)4.0");     /* unary-operator cast-expression */
        Test(Fn, "sizeof(Foo++)"); /* sizeof unary-expression */
        Test(Fn, "sizeof(int)");   /* sizeof ( type-name ) */
}

void
TestCastExpression()
{
        parser_function Fn = ParseCastExpression;
        Test(Fn, "sizeof(int)"); /* unary-expression */
        Test(Fn, "(int)4.0");    /* ( type-name ) cast-expression */
}

void
TestMultiplicativeExpression()
{
        parser_function Fn = ParseMultiplicativeExpression;
        Test(Fn, "(int)4.0");            /* cast-expression */
        Test(Fn, "(int)4.0 * (int)4.0"); /* multiplicative-expression * cast-expression */
        Test(Fn, "(int)4.0 / (int)4.0"); /* multiplicative-expression / cast-expression */
        Test(Fn, "(int)4.0 % (int)4.0"); /* multiplicative-expression % cast-expression */
}

void
TestPrimaryExpression()
{
        parser_function Fn = ParsePrimaryExpression;
        Test(Fn, "my_var");        /* identifier */
        Test(Fn, "4.0");           /* constant */
        Test(Fn, "\"String\"");    /* string */
        Test(Fn, "(sizeof(int))"); /* ( expression ) */
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

        TestConstant();
        TestArgumentExpressionList();
        TestUnaryExpression();
        TestCastExpression();
        TestMultiplicativeExpression();
        TestPrimaryExpression();

        printf("All tests passed successfully\n");
        return(EXIT_SUCCESS);
}
