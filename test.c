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
TestPrimaryExpression()
{
        parser_function Fn = ParsePrimaryExpression;
        Test(Fn, "my_var");        /* identifier */
        Test(Fn, "4.0");           /* constant */
        Test(Fn, "\"String\"");    /* string */
        Test(Fn, "(sizeof(int))"); /* ( expression ) */
}

void
TestPostfixExpression()
{
        parser_function Fn = ParsePostfixExpression;
        Test(Fn, "my_var");       /* primary-expression */
        Test(Fn, "my_var[0]");    /* postfix-expression [ expression ] */
        Test(Fn, "my_fn(1, 2)");  /* postfix-expression ( argument-expression-list(opt) ) */
        Test(Fn, "my_struct.x");  /* postfix-expression . identifier */
        Test(Fn, "my_struct->x"); /* postfix-expression -> identifier */
        Test(Fn, "my_value++");   /* postfix-expression ++ */
        Test(Fn, "my_value--");   /* postfix-expression -- */
}

void
TestUnaryOperator()
{
        parser_function Fn = ParseUnaryOperator;
        Test(Fn, "&");
        Test(Fn, "*");
        Test(Fn, "+");
        Test(Fn, "-");
        Test(Fn, "~");
        Test(Fn, "!");
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
TestAdditiveExpression()
{
        parser_function Fn = ParseAdditiveExpression;
        Test(Fn, "4 * 4");               /* multiplicative-expression */
        Test(Fn, "(int)4.0 + (int)4.0"); /* additive-expression + multiplicative-expression */
        Test(Fn, "(int)4.0 - (int)4.0"); /* additive-expression - multiplicative-expression */
}

void
TestShiftExpression()
{
        parser_function Fn = ParseShiftExpression;
        Test(Fn, "4 + 4");          /* additive-expression */
        Test(Fn, "4 + 4 << 4 + 4"); /* shift-expression << additive-expression */
        Test(Fn, "4 + 4 >> 4 + 4"); /* shift-expression >> additive-expression */
}

void
TestRelationalExpression()
{
        parser_function Fn = ParseRelationalExpression;
        Test(Fn, "my_value << 4");                        /* shift-expression */
        Test(Fn, "(my_value << 4) < (your_value >> 2)");  /* relational-expression < shift-expression */
        Test(Fn, "(my_value >> 4) > (your_value << 2)");  /* relational-expression > shift-expression */
        Test(Fn, "(my_value >> 4) <= (your_value >> 2)"); /* relational-expression <= shift-expression */
        Test(Fn, "(my_value << 4) >= (your_value >> 2)"); /* relational-expression >= shift-expression */
}

void
TestEqualityExpression()
{
        parser_function Fn = ParseEqualityExpression;
        Test(Fn, "1 < 2");              /* relational-expression */
        Test(Fn, "(1 < 2) == (2 > 1)"); /* equality-expression == relational-expression */
        Test(Fn, "(1 < 2) != 3");       /* equality-expression != relational-expression */
}

void
TestAndExpression()
{
        parser_function Fn = ParseAndExpression;
        Test(Fn, "1 != 3");      /* equality-expression */
        Test(Fn, "1 != 3 & 24"); /* AND-expression & equality-expression */
}

void
TestExclusiveOrExpression()
{
        parser_function Fn = ParseExclusiveOrExpression;
        Test(Fn, "1 != 3 & 24");        /* AND-expression */
        Test(Fn, "(2 != 3 & 24) ^ 31"); /* exclusive-OR-expression ^ AND-expression */
}

void
TestInclusiveOrExpression()
{
        parser_function Fn = ParseInclusiveOrExpression;
        Test(Fn, "(2 != 3 & 24) ^ 31"); /* exclusive-OR-expression */
        Test(Fn, "2 | 3");              /* inclusive-OR-expression | exclusive-OR-expression */
}

void
TestLogicalAndExpression()
{
        parser_function Fn = ParseLogicalAndExpression;
        Test(Fn, "2 | 3");              /* inclusive-OR-expression */
        Test(Fn, "(2 | 3) && (2 | 3)"); /* logical-AND-expression && inclusive-OR-expression */
}

void
TestLogicalOrExpression()
{
        parser_function Fn = ParseLogicalOrExpression;
        Test(Fn, "2 && 3");           /* logical-AND-expression */
        Test(Fn, "2 && 3 || 2 && 3"); /* logical-OR-expression || logical-AND-expression */
}

void
TestConstantExpression()
{
        parser_function Fn = ParseConstantExpression;
        Test(Fn, "2 || 3 ? 4 : 5");   /* conditional-expression */
}

void
TestConditionalExpression()
{
        parser_function Fn = ParseConditionalExpression;
        Test(Fn, "2 && 3 || 2 && 3"); /* logical-OR-expression */
        Test(Fn, "2 || 3 ? 4 : 5");   /* logical-OR-expression ? expression : conditional-expression */
}

void
TestAssignmentOperator()
{
        parser_function Fn = ParseAssignmentOperator;
        Test(Fn, "=");
        Test(Fn, "*=");
        Test(Fn, "/=");
        Test(Fn, "%=");
        Test(Fn, "+=");
        Test(Fn, "-=");
        Test(Fn, "<<=");
        Test(Fn, ">>=");
        Test(Fn, "&=");
        Test(Fn, "^=");
        Test(Fn, "|=");
}

void
TestAssignmentExpression()
{
        parser_function Fn = ParseAssignmentExpression;
        Test(Fn, "2 || 3 ? 4 : 5");            /* conditional-expression */
        Test(Fn, "++Foo |= ++Foo"); /* unary-expression assignment-operator assignment-expression */
}

void
TestExpression()
{
        parser_function Fn = ParseExpression;
        Test(Fn, "++Foo |= (2 || 3 ? 4 : 5)"); /* assignment-expression */
        Test(Fn, "++Foo, Foo++");              /* expression, assignment-expression */
        Test(Fn, "i=0");
}

void
TestIdentifier()
{
        parser_function Fn = ParseIdentifier;
        Test(Fn, "_foo");
        Test(Fn, "foo123_");

        {
                char *String = "!foo";
                struct tokenizer Tokenizer = InitTokenizer((String));
                bool Result = Fn(&Tokenizer);
                AssertFn("Result should be false", Result != true, __LINE__, __func__);
                AssertFn("Tokenizer doesn't advance", Tokenizer.At == String, __LINE__, __func__);
        }

        {
                char *String = "123_foo";
                struct tokenizer Tokenizer = InitTokenizer((String));
                bool Result = Fn(&Tokenizer);
                AssertFn("Result should be false", Result != true, __LINE__, __func__);
                AssertFn("Tokenizer doesn't advance", Tokenizer.At == String, __LINE__, __func__);
        }
}

void
TestJumpStatement()
{
        parser_function Fn = ParseJumpStatement;
        Test(Fn, "goto foo;"); /* goto identifier ; */
        Test(Fn, "continue;"); /* continue ; */
        Test(Fn, "break;");    /* break ; */
        Test(Fn, "return;");   /* return expression(opt) ; */
}

void
TestIterationStatement()
{
        parser_function Fn = ParseIterationStatement;
        Test(Fn, "while(true) {}");                     /* while ( expression ) statement */
        Test(Fn, "do { foo = 2; } while(++Foo |= 3);"); /* do statement while ( expression ) ; */
        /* NOTE(AARON): This version of C doesn't support just-in-time declarations. */
        Test(Fn, "for(i=0; i<Foo; i++) { ; }");         /* for ( expression(opt) ; expression(opt) ; expression(opt) ) statement */
}

void
TestSelectionStatement()
{
        parser_function Fn = ParseSelectionStatement;
        Test(Fn, "if(true) return;");                 /* if ( expression ) statement */
        Test(Fn, "if(1 = 3) goto end; else return;"); /* if ( expression ) statement else statement */
        Test(Fn, "switch(Foo) { break; }");           /* switch ( expression ) statement */
}

void
TestStatementList()
{
        parser_function Fn = ParseStatementList;
        Test(Fn, "i = 1;");        /* statement */
        Test(Fn, "i = 1; i = 1;"); /* statement-list statement */
}

void
TestCompoundStatement()
{
        parser_function Fn = ParseCompoundStatement;
        Test(Fn, "{ int i; i = 2; i = 3; }"); /* { declaration-list(opt) statement-list(opt) } */
        Test(Fn, "{ }");
        Test(Fn, "{ int i; }");
        Test(Fn, "{ i = 3; }");
}

void
TestExpressionStatement()
{
        parser_function Fn = ParseExpressionStatement;
        Test(Fn, ";"); /* expression(opt) */
        Test(Fn, "++Foo |= (2 || 3 ? 4 : 5) ;");
        Test(Fn, "++Foo, Foo++;");
        Test(Fn, "i=0;");
}

void
TestLabeledStatement()
{
        parser_function Fn = ParseLabeledStatement;
        Test(Fn, "label: i = 0;");                    /* identifier : statement */
        Test(Fn, "case Foo: break;");                 /* case constant-expression : statement */
        Test(Fn, "default: { exit(EXIT_SUCCESS); }"); /* default : statement */
}

void
TestStatement()
{
        /* TODO(AARON): Gotta get sample strings for these. */
        parser_function Fn = ParseStatement;
        Test(Fn, ""); /* labeled-statement */
        Test(Fn, ""); /* expression-statement */
        Test(Fn, ""); /* compound-statement */
        Test(Fn, ""); /* selection-statement */
        Test(Fn, ""); /* iteration-statement */
        Test(Fn, ""); /* jump-statement */
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
        TestPrimaryExpression();
        TestPostfixExpression();
        TestUnaryOperator();
        TestCastExpression();
        TestMultiplicativeExpression();
        TestAdditiveExpression();
        TestShiftExpression();
        TestRelationalExpression();
        TestEqualityExpression();
        TestAndExpression();
        TestExclusiveOrExpression();
        TestInclusiveOrExpression();
        TestLogicalAndExpression();
        TestLogicalOrExpression();
        TestConstantExpression();
        TestConditionalExpression();
        TestAssignmentOperator();
        TestAssignmentExpression();
        TestExpression();
        TestIdentifier();
        TestJumpStatement();
        TestIterationStatement();
        TestSelectionStatement();
        TestStatementList();
        TestCompoundStatement();
        TestExpressionStatement();
        TestLabeledStatement();
        TestStatement();

        printf("All tests passed successfully\n");
        return(EXIT_SUCCESS);
}
