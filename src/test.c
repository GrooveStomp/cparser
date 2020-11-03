#include <stdlib.h> /* EXIT_SUCCESS, EXIT_FAILURE */
#include <stdio.h>
#include <alloca.h>

#include "gs.h"
#include "gstest.h"
#include "lexer.c"
#include "parser.c"
#include "tree.c"

typedef gs_bool (*parser_function)(struct tokenizer *, parse_tree_node *);

#define Accept(Function, String) \
        { \
                parse_tree_node *ParseTree = ParseTreeNew(); \
                struct tokenizer Tokenizer = InitTokenizer((String)); \
                gs_bool Result = Function(&Tokenizer, ParseTree); \
                ParseTreeFree(ParseTree); \
                GSTestAssert(Result == true, "Result should be true\n"); \
                GSTestAssert(Tokenizer.At == (String) + GSStringLength((String)), "Tokenizer advances to end of string\n"); \
        }
#define Reject(Function, String) \
        { \
                parse_tree_node *ParseTree = ParseTreeNew(); \
                struct tokenizer Tokenizer = InitTokenizer((String)); \
                gs_bool Result = Function(&Tokenizer, ParseTree); \
                ParseTreeFree(ParseTree); \
                GSTestAssert(Result != true, "Result should be false\n"); \
                GSTestAssert(Tokenizer.At == (String), "Tokenizer doesn't advance\n"); \
        }

struct tokenizer InitTokenizer(char *String) {
        struct tokenizer Tokenizer;
        Tokenizer.Beginning = Tokenizer.At = String;
        Tokenizer.Line = Tokenizer.Column = 1;
        return Tokenizer;
}

/*----------------------------------------------------------------------------
  Tests
  ----------------------------------------------------------------------------*/

void TestConstant() {
        parser_function Fn = ParseConstant;
        Accept(Fn, "1");   /* integer-constant */
        Accept(Fn, "'c'"); /* character-constant */
        Accept(Fn, "2.0"); /* floating-constant */
        /* TODO(AARON): enumeration-constant */
        Reject(Fn, "foo");
        Reject(Fn, "sizeof(int)");
}

void TestArgumentExpressionList() {
        parser_function Fn = ParseArgumentExpressionList;
        Accept(Fn, "(int)4.0");           /* assignment-expression */
        Accept(Fn, "(int)4.0, (int)4.0"); /* argument-expression-list , assignment-expression */
}

void TestPrimaryExpression() {
        parser_function Fn = ParsePrimaryExpression;
        Accept(Fn, "my_var");        /* identifier */
        Accept(Fn, "4.0");           /* constant */
        Accept(Fn, "\"String\"");    /* string */
        Accept(Fn, "(sizeof(int))"); /* ( expression ) */
}

void TestPostfixExpression() {
        parser_function Fn = ParsePostfixExpression;
        Accept(Fn, "my_var");       /* primary-expression */
        Accept(Fn, "my_var[0]");    /* postfix-expression [ expression ] */
        Accept(Fn, "my_fn(1, 2)");  /* postfix-expression ( argument-expression-list(opt) ) */
        Accept(Fn, "my_struct.x");  /* postfix-expression . identifier */
        Accept(Fn, "my_struct->x"); /* postfix-expression -> identifier */
        Accept(Fn, "my_value++");   /* postfix-expression ++ */
        Accept(Fn, "my_value--");   /* postfix-expression -- */
        Reject(Fn, "--my_value");
}

void TestUnaryOperator() {
        parser_function Fn = ParseUnaryOperator;
        Accept(Fn, "&");
        Accept(Fn, "*");
        Accept(Fn, "+");
        Accept(Fn, "-");
        Accept(Fn, "~");
        Accept(Fn, "!");
}

void TestUnaryExpression() {
        parser_function Fn = ParseUnaryExpression;
        Accept(Fn, "\"String\"");    /* postfix-expression */
        Accept(Fn, "++Foo");         /* ++ unary-expression */
        Accept(Fn, "--Foo");         /* -- unary-expression */
        Accept(Fn, "&(int)4.0");     /* unary-operator cast-expression */
        Accept(Fn, "sizeof(Foo++)"); /* sizeof unary-expression */
        Accept(Fn, "sizeof(int)");   /* sizeof ( type-name ) */
}

void TestCastExpression() {
        parser_function Fn = ParseCastExpression;
        Accept(Fn, "sizeof(int)"); /* unary-expression */
        Accept(Fn, "(int)4.0");    /* ( type-name ) cast-expression */
}

void TestMultiplicativeExpression() {
        parser_function Fn = ParseMultiplicativeExpression;
        Accept(Fn, "(int)4.0");            /* cast-expression */
        Accept(Fn, "(int)4.0 * (int)4.0"); /* multiplicative-expression * cast-expression */
        Accept(Fn, "(int)4.0 / (int)4.0"); /* multiplicative-expression / cast-expression */
        Accept(Fn, "(int)4.0 % (int)4.0"); /* multiplicative-expression % cast-expression */
}

void TestAdditiveExpression() {
        parser_function Fn = ParseAdditiveExpression;
        Accept(Fn, "4 * 4");               /* multiplicative-expression */
        Accept(Fn, "(int)4.0 + (int)4.0"); /* additive-expression + multiplicative-expression */
        Accept(Fn, "(int)4.0 - (int)4.0"); /* additive-expression - multiplicative-expression */
}

void TestShiftExpression() {
        parser_function Fn = ParseShiftExpression;
        Accept(Fn, "4 + 4");          /* additive-expression */
        Accept(Fn, "4 + 4 << 4 + 4"); /* shift-expression << additive-expression */
        Accept(Fn, "4 + 4 >> 4 + 4"); /* shift-expression >> additive-expression */
}

void TestRelationalExpression() {
        parser_function Fn = ParseRelationalExpression;
        Accept(Fn, "my_value << 4");                        /* shift-expression */
        Accept(Fn, "(my_value << 4) < (your_value >> 2)");  /* relational-expression < shift-expression */
        Accept(Fn, "(my_value >> 4) > (your_value << 2)");  /* relational-expression > shift-expression */
        Accept(Fn, "(my_value >> 4) <= (your_value >> 2)"); /* relational-expression <= shift-expression */
        Accept(Fn, "(my_value << 4) >= (your_value >> 2)"); /* relational-expression >= shift-expression */
}

void TestEqualityExpression() {
        parser_function Fn = ParseEqualityExpression;
        Accept(Fn, "1 < 2");              /* relational-expression */
        Accept(Fn, "(1 < 2) == (2 > 1)"); /* equality-expression == relational-expression */
        Accept(Fn, "(1 < 2) != 3");       /* equality-expression != relational-expression */
}

void TestAndExpression() {
        parser_function Fn = ParseAndExpression;
        Accept(Fn, "1 != 3");      /* equality-expression */
        Accept(Fn, "1 != 3 & 24"); /* AND-expression & equality-expression */
}

void TestExclusiveOrExpression() {
        parser_function Fn = ParseExclusiveOrExpression;
        Accept(Fn, "1 != 3 & 24");        /* AND-expression */
        Accept(Fn, "(2 != 3 & 24) ^ 31"); /* exclusive-OR-expression ^ AND-expression */
}

void TestInclusiveOrExpression() {
        parser_function Fn = ParseInclusiveOrExpression;
        Accept(Fn, "(2 != 3 & 24) ^ 31"); /* exclusive-OR-expression */
        Accept(Fn, "2 | 3");              /* inclusive-OR-expression | exclusive-OR-expression */
}

void TestLogicalAndExpression() {
        parser_function Fn = ParseLogicalAndExpression;
        Accept(Fn, "2 | 3");              /* inclusive-OR-expression */
        Accept(Fn, "(2 | 3) && (2 | 3)"); /* logical-AND-expression && inclusive-OR-expression */
}

void TestLogicalOrExpression() {
        parser_function Fn = ParseLogicalOrExpression;
        Accept(Fn, "2 && 3");           /* logical-AND-expression */
        Accept(Fn, "2 && 3 || 2 && 3"); /* logical-OR-expression || logical-AND-expression */
}

void TestConstantExpression() {
        parser_function Fn = ParseConstantExpression;
        Accept(Fn, "2 || 3 ? 4 : 5");   /* conditional-expression */
}

void TestConditionalExpression() {
        parser_function Fn = ParseConditionalExpression;
        Accept(Fn, "2 && 3 || 2 && 3"); /* logical-OR-expression */
        Accept(Fn, "2 || 3 ? 4 : 5");   /* logical-OR-expression ? expression : conditional-expression */
}

void TestAssignmentOperator() {
        parser_function Fn = ParseAssignmentOperator;
        Accept(Fn, "=");
        Accept(Fn, "*=");
        Accept(Fn, "/=");
        Accept(Fn, "%=");
        Accept(Fn, "+=");
        Accept(Fn, "-=");
        Accept(Fn, "<<=");
        Accept(Fn, ">>=");
        Accept(Fn, "&=");
        Accept(Fn, "^=");
        Accept(Fn, "|=");
}

void TestAssignmentExpression() {
        parser_function Fn = ParseAssignmentExpression;
        Accept(Fn, "2 || 3 ? 4 : 5"); /* conditional-expression */
        Accept(Fn, "++Foo |= ++Foo"); /* unary-expression assignment-operator assignment-expression */
}

void TestExpression() {
        parser_function Fn = ParseExpression;
        Accept(Fn, "++Foo |= (2 || 3 ? 4 : 5)"); /* assignment-expression */
        Accept(Fn, "++Foo, Foo++");              /* expression, assignment-expression */
        Accept(Fn, "i=0");
}

void TestIdentifier() {
        parser_function Fn = ParseIdentifier;
        Accept(Fn, "_foo");
        Accept(Fn, "foo123_");
        Reject(Fn, "!foo");
        Reject(Fn, "123_foo");
}

void TestJumpStatement() {
        parser_function Fn = ParseJumpStatement;
        Accept(Fn, "goto foo;"); /* goto identifier ; */
        Accept(Fn, "continue;"); /* continue ; */
        Accept(Fn, "break;");    /* break ; */
        Accept(Fn, "return;");   /* return expression(opt) ; */
}

void TestIterationStatement() {
        parser_function Fn = ParseIterationStatement;
        Accept(Fn, "while(true) {}");                     /* while ( expression ) statement */
        Accept(Fn, "do { foo = 2; } while(++Foo |= 3);"); /* do statement while ( expression ) ; */
        /* NOTE(AARON): This version of C doesn't support just-in-time declarations. */
        Accept(Fn, "for(i=0; i<Foo; i++) { ; }");         /* for ( expression(opt) ; expression(opt) ; expression(opt) ) statement */
}

void TestSelectionStatement() {
        parser_function Fn = ParseSelectionStatement;
        Accept(Fn, "if(true) return;");                 /* if ( expression ) statement */
        Accept(Fn, "if(1 = 3) goto end; else return;"); /* if ( expression ) statement else statement */
        Accept(Fn, "switch(Foo) { break; }");           /* switch ( expression ) statement */
}

void TestStatementList() {
        parser_function Fn = ParseStatementList;
        Accept(Fn, "i = 1;");        /* statement */
        Accept(Fn, "i = 1; i = 1;"); /* statement-list statement */
}

void TestCompoundStatement() {
        parser_function Fn = ParseCompoundStatement;
        Accept(Fn, "{ int i; i = 2; i = 3; }"); /* { declaration-list(opt) statement-list(opt) } */
        Accept(Fn, "{ }");
        Accept(Fn, "{ int i; }");
        Accept(Fn, "{ i = 3; }");
}

void TestExpressionStatement() {
        parser_function Fn = ParseExpressionStatement;
        Accept(Fn, ";"); /* expression(opt) */
        Accept(Fn, "++Foo |= (2 || 3 ? 4 : 5) ;");
        Accept(Fn, "++Foo, Foo++;");
        Accept(Fn, "i=0;");
}

void TestLabeledStatement() {
        parser_function Fn = ParseLabeledStatement;
        Accept(Fn, "label: i = 0;");                    /* identifier : statement */
        Accept(Fn, "case Foo: break;");                 /* case constant-expression : statement */
        Accept(Fn, "default: { exit(EXIT_SUCCESS); }"); /* default : statement */
}

void TestStatement() {
        parser_function Fn = ParseStatement;
        Accept(Fn, "default: { exit(EXIT_SUCCESS); }"); /* labeled-statement */
        Accept(Fn, "++Foo |= (2 || 3 ? 4 : 5);");       /* expression-statement */
        Accept(Fn, "{ int i; i = 2; }");                /* compound-statement */
        Accept(Fn, "switch(Foo) { break; }");           /* selection-statement */
        Accept(Fn, "for(i=0; i<Foo; i++) { ; }");       /* iteration-statement */
        Accept(Fn, "goto foo;");                        /* jump-statement */
}

void TestTypedefName() {
        parser_function Fn = ParseTypedefName;
        Reject(Fn, "my_type");
        TypedefInit();
        TypedefAddName("my_type");
        Accept(Fn, "my_type"); /* identifier */
        TypedefClear();
}

void TestDirectAbstractDeclarator() {
        parser_function Fn = ParseDirectAbstractDeclarator;
        Accept(Fn, "(int i)");                   /* ( abstract-declarator ) */
        Accept(Fn, "(int i[2])");                /* direct-abstract-declarator(opt) [ constant-expression(opt) ] */
        Accept(Fn, "(int i[])");                 /* direct-abstract-declarator(opt) [ constant-expression(opt) ] */
        Accept(Fn, "([2])");                     /* direct-abstract-declarator(opt) [ constant-expression(opt) ] */
        Accept(Fn, "([])");                      /* direct-abstract-declarator(opt) [ constant-expression(opt) ] */
/*        Accept(Fn, "(foo())");                   /* direct-abstract-declarator(opt) ( parameter-type-list(opt) ) */
/*        Accept(Fn, "(foo(int arg1, int arg2))"); /* direct-abstract-declarator(opt) ( parameter-type-list(opt) ) */
        Accept(Fn, "((int arg1, int arg2))");    /* direct-abstract-declarator(opt) ( parameter-type-list(opt) ) */
        Accept(Fn, "(())");                      /* direct-abstract-declarator(opt) ( parameter-type-list(opt) ) */
}

void TestAbstractDeclarator() {
        parser_function Fn = ParseAbstractDeclarator;
        Accept(Fn, "*");                              /* pointer */
        Accept(Fn, "* volatile");                     /* pointer */
        Accept(Fn, "((int arg1, int arg2))");         /* pointer(opt) direct-abstract-declarator */
        Accept(Fn, "* ((int arg1, int arg2))");       /* pointer(opt) direct-abstract-declarator */
        Accept(Fn, "* const ((int arg1, int arg2))"); /* pointer(opt) direct-abstract-declarator */
}

void TestTypeName() {
        parser_function Fn = ParseTypeName;
        Accept(Fn, "volatile int"); /* specifier-qualifier-list abstract-declarator(opt) */
        Accept(Fn, "const void");   /* specifier-qualifier-list abstract-declarator(opt) */
        Accept(Fn, "short");
        Reject(Fn, "my_type");
        TypedefInit();
        TypedefAddName("my_type");
        Accept(Fn, "my_type");
        Accept(Fn, "const my_type");
        TypedefClear();
}

void TestInitializerList() {
        parser_function Fn = ParseInitializerList;
        Accept(Fn, "++Foo |= --Foo"); /* initializer */
        Accept(Fn, "++Foo |= --Foo, ++Bar |= --Bar"); /* initializer-list , initializer */
}

void TestInitializer() {
        parser_function Fn = ParseInitializer;
        Accept(Fn, "++Foo |= --Foo");           /* assignment-expression */
        Accept(Fn, "{ i |= --i, ++j |= j }");   /* { initializer-list } */
        Accept(Fn, "{ i |= --i, ++j |= j,  }"); /* { initializer-list , } */
}

void TestIdentifierList() {
        parser_function Fn = ParseIdentifierList;
        Accept(Fn, "foo");           /* identifier */
        Accept(Fn, "foo, bar, baz"); /* identifier-list , identifier */
}

void TestParameterDeclaration() {
        parser_function Fn = ParseParameterDeclaration;
        Accept(Fn, "auto int foo");    /* declaration-specifiers declarator */
        Accept(Fn, "register char *"); /* declaration-specifiers abstract-declarator(opt) */
        Accept(Fn, "extern float");    /* declaration-specifiers abstract-declarator(opt) */
}

void TestParameterList() {
        parser_function Fn = ParseParameterList;
        Accept(Fn, "extern float");                  /* parameter-declaration */
        Accept(Fn, "extern float, register char *"); /* parameter-list, parameter-declaration */
}

void TestParameterTypeList() {
        parser_function Fn = ParseParameterTypeList;
        Accept(Fn, "extern float, register char *"); /* parameter-list */
        Accept(Fn, "float, char, ...");              /* parameter-list , ... */
}

void TestTypeQualifierList() {
        parser_function Fn = ParseTypeQualifierList;
        Accept(Fn, "const");          /* type-qualifier */
        Accept(Fn, "const volatile"); /* type-qualifier-list type-qualifier */
}

void TestPointer() {
        parser_function Fn = ParsePointer;
        Accept(Fn, "*");                  /* type-qualifier-list(opt) */
        Accept(Fn, "* const");            /* type-qualifier-list(opt) */
        Accept(Fn, "**");                 /* type-qualifier-list(opt) pointer */
        Accept(Fn, "* const *");          /* type-qualifier-list(opt) pointer */
        Accept(Fn, "* const * volatile"); /* type-qualifier-list(opt) pointer */
}

void TestDirectDeclarator() {
        parser_function Fn = ParseDirectDeclarator;
        Accept(Fn, "foo");                  /* identifier */
        Accept(Fn, "(foo)");                /* ( declarator ) */
        Accept(Fn, "(foo)[2]");             /* direct-declarator [ constant-expression(opt) ] */
        Accept(Fn, "(foo)[]");              /* direct-declarator [ constant-expression(opt) ] */
        Accept(Fn, "(foo)(int i, char c)"); /* direct-declarator ( parameter-type-list ) */
        Accept(Fn, "(foo)()");              /* direct-declarator ( identifier-list(opt) ) */
}

void TestDeclarator() {
        parser_function Fn = ParseDeclarator;
        Accept(Fn, "* const * volatile (foo)(int i, char c)"); /* pointer(opt) direct-declarator */
        Accept(Fn, "(foo)(int i, char c)");                    /* pointer(opt) direct-declarator */
}

void TestEnumerator() {
        parser_function Fn = ParseEnumerator;
        Accept(Fn, "foo");     /* identifier */
        Accept(Fn, "foo = 2"); /* identifier = constant-expression */
}

void TestEnumeratorList() {
        parser_function Fn = ParseEnumeratorList;
        Accept(Fn, "foo = 2");          /* enumerator */
        Accept(Fn, "foo = 2, bar = 3"); /* enumerator-list , enumerator */
}

void TestEnumSpecifier() {
        parser_function Fn = ParseEnumSpecifier;
        Accept(Fn, "enum { foo = 2, bar = 3 }");        /* enum identifier(opt) { enumerator-list } */
        Accept(Fn, "enum MyEnum { foo = 2, bar = 3 }"); /* enum identifier(opt) { enumerator-list } */
        Accept(Fn, "enum foo");                         /* enum identifier */
}

void TestStructDeclarator() {
        parser_function Fn = ParseStructDeclarator;
        Accept(Fn, "(foo)(int i, char c)");    /* declarator */
        Accept(Fn, "(foo)(int i, char c): 2"); /* declarator(opt) : constant-expression */
}

void TestStructDeclaratorList() {
        parser_function Fn = ParseStructDeclaratorList;
        Accept(Fn, "(foo)(int i, char c)"); /* struct-declarator */
        Accept(Fn, "x: 2, y: 3");           /* struct-declarator-list , struct-declarator */
}

void TestSpecifierQualifierList() {
        parser_function Fn = ParseSpecifierQualifierList;
        Accept(Fn, "void char short");                      /* type-specifier specifier-qualifier-list(opt) */
        Accept(Fn, "int");                                  /* type-specifier specifier-qualifier-list(opt) */
        Accept(Fn, "const");                                /* type-qualifier specifier-qualifier-list(opt) */
        Accept(Fn, "volatile double");                      /* type-qualifier specifier-qualifier-list(opt) */
        Accept(Fn, "void const short volatile double int"); /* type-qualifier specifier-qualifier-list(opt) */
}

void TestStructDeclaration() {
        parser_function Fn = ParseStructDeclaration;
        Accept(Fn, "volatile double x: 2, y: 3;"); /* specifier-qualifier-list struct-declarator-list ; */
}

void TestInitDeclarator() {
        parser_function Fn = ParseInitDeclarator;
        Accept(Fn, "* const * volatile (foo)(int i, char c)"); /* declarator */
        Accept(Fn, "(foo)(int i, char c) = ++Foo |= --Foo");   /* declarator = initializer */
}

void TestInitDeclaratorList() {
        parser_function Fn = ParseInitDeclaratorList;
        Accept(Fn, "foo = 2");          /* init-declarator */
        Accept(Fn, "foo = 2, bar = 3"); /* init-declarator-list , init-declarator */
}

void TestStructDeclarationList() {
        parser_function Fn = ParseStructDeclarationList;
        Accept(Fn, "volatile double x: 2, y: 3;"); /* struct-declaration */
        Accept(Fn, "int x: 2; int y: 3;");         /* struct-declaration-list struct-declaration */
}

void TestStructOrUnion() {
        parser_function Fn = ParseStructOrUnion;
        Accept(Fn, "struct");
        Accept(Fn, "union");
}

void TestStructOrUnionSpecifier() {
        parser_function Fn = ParseStructOrUnionSpecifier;
        Accept(Fn, "struct foo { int x: 2; float y: 3.0; }"); /* struct-or-union identifier(opt) { struct-declaration-list } */
        Accept(Fn, "union { int x: 2; float y: 3.0; }");      /* struct-or-union identifier(opt) { struct-declaration-list } */
        Accept(Fn, "struct foo");                             /* struct-or-union identifier */
}

void TestTypeQualifier() {
        parser_function Fn = ParseTypeQualifier;
        Accept(Fn, "const");
        Accept(Fn, "volatile");
}

void TestTypeSpecifier() {
        parser_function Fn = ParseTypeSpecifier;
        Accept(Fn, "void");
        Accept(Fn, "char");
        Accept(Fn, "short");
        Accept(Fn, "int");
        Accept(Fn, "long");
        Accept(Fn, "float");
        Accept(Fn, "double");
        Accept(Fn, "signed");
        Accept(Fn, "unsigned");
        Accept(Fn, "struct foo");                       /* struct-or-union-specifier */
        Accept(Fn, "enum MyEnum { foo = 2, bar = 3 }"); /* enum-specifier */
        Reject(Fn, "foo");                              /* typedef-name */
}

void TestStorageClassSpecifier() {
        parser_function Fn = ParseStorageClassSpecifier;
        Accept(Fn, "auto");
        Accept(Fn, "register");
        Accept(Fn, "static");
        Accept(Fn, "extern");
        Accept(Fn, "typedef");
}

void TestDeclarationSpecifiers() {
        parser_function Fn = ParseDeclarationSpecifiers;
        Accept(Fn, "static static");        /* storage-class-specifier declaration-specifers(opt) */
        Accept(Fn, "static");               /* storage-class-specifier declaration-specifers(opt) */
        Accept(Fn, "void void");            /* type-specifier declaration-specifiers(opt) */
        Accept(Fn, "int float");            /* type-specifier declaration-specifiers(opt) */
        Accept(Fn, "char static volatile"); /* type-specifier declaration-specifiers(opt) */
        Accept(Fn, "static void");          /* type-specifier declaration-specifiers(opt) */
        Accept(Fn, "const");                /* type-qualifier declaration-specifiers(opt) */
        Accept(Fn, "const volatile void");  /* type-qualifier declaration-specifiers(opt) */
        Accept(Fn, "const volatile");       /* type-qualifier declaration-specifiers(opt) */
}

void TestDeclarationList() {
        parser_function Fn = ParseDeclarationList;
        Accept(Fn, "const volatile foo;");                /* declaration */
        Accept(Fn, "volatile short foo; const int bar;"); /* declaration-list declaration */
}

void TestDeclaration() {
        parser_function Fn = ParseDeclaration;
        Accept(Fn, "const volatile;");     /* declaration-specifiers init-declarator-list(opt) ; */
        Accept(Fn, "const volatile foo;"); /* declaration-specifiers init-declarator-list(opt) ; */
        Accept(Fn, "volatile short foo = 2;");
        Accept(Fn, "const volatile foo = 2, bar = 3;");
}

void TestFunctionDefinition() {
        parser_function Fn = ParseFunctionDefinition;
        /* declaration-specifiers(opt) declarator declaration-list(opt) compound-statement */
        Accept(Fn, "int main(){}");
        Accept(Fn, "main(int a, int b) { }"); /* declarator declaration-list compound-statement */
        Accept(Fn, "main() { }");             /* declarator declaration-list compound-statement */
        Accept(Fn, "main { }");               /* declarator compound-statement */

}

void TestExternalDeclaration() {
        parser_function Fn = ParseExternalDeclaration;
        Accept(Fn, "main() {}");                   /* function-definition */
        Accept(Fn, "const volatile int foo = 2;"); /* declaration */
}

void TestTranslationUnit() {
        parser_function Fn = ParseTranslationUnit;
        Accept(Fn, "int global = 1; int main(){}");
        Accept(Fn, "int main(int argc, char **argv) { int i = 2; return(i); }");
}

/*----------------------------------------------------------------------------
  Main Entrypoint
  ----------------------------------------------------------------------------*/

int main(int ArgCount, char **Arguments) {
        gs_args Args;
        GSArgsInit(&Args, ArgCount, Arguments);

        if(GSArgsHelpWanted(&Args))
        {
                printf("Executable tests\n\n");
                printf("Usage: run\n");
                printf("  Specify '-h' or '--help' for this help text.\n");
                exit(EXIT_SUCCESS);
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
        TestTypedefName();
        TestDirectAbstractDeclarator();
        TestAbstractDeclarator();
        TestTypeName();
        TestInitializerList();
        TestInitializer();
        TestIdentifierList();
        TestParameterDeclaration();
        TestParameterList();
        TestTypeQualifierList();
        TestPointer();
        TestDirectDeclarator();
        TestDeclarator();
        TestEnumerator();
        TestEnumeratorList();
        TestEnumSpecifier();
        TestStructDeclarator();
        TestStructDeclaratorList();
        TestSpecifierQualifierList();
        TestStructDeclaration();
        TestInitDeclarator();
        TestInitDeclaratorList();
        TestStructDeclarationList();
        TestStructOrUnion();
        TestStructOrUnionSpecifier();
        TestTypeQualifier();
        TestTypeSpecifier();
        TestStorageClassSpecifier();
        TestDeclarationSpecifiers();
        TestDeclarationList();
        TestDeclaration();
        TestFunctionDefinition();
        TestExternalDeclaration();
        TestTranslationUnit();

        printf("All tests successful\n");
        return EXIT_SUCCESS;
}
