#ifndef _PARSER_C
#define _PARSER_C

#include <stdio.h>
#include "bool.c"
#include "lexer.c"

/* NOTE(AARON): Debug include - tree.c */
#include "tree.c"

#define PARSE_TREE_UPDATE(Name, NumChildren) \
        char *NodeName = (Name); \
        ParseTreeSetName(ParseTree, (Name)); \
        if(NumChildren > 0) \
        { \
                ParseTreeNewChildren(ParseTree, (NumChildren)); \
        }

#define PARSE_TREE_CLEAR_CHILDREN() \
        for(int i=0; i<ParseTree->NumChildren; i++) \
        { \
                ParseTreeSetName(&ParseTree->Children[i], "Empty");  \
        }

bool ParseAssignmentExpression(struct tokenizer *Tokenizer, parse_tree_node *ParseTree);
bool ParseExpression(struct tokenizer *Tokenizer, parse_tree_node *ParseTree);
bool ParseTypeName(struct tokenizer *Tokenizer, parse_tree_node *ParseTree);
bool ParseCastExpression(struct tokenizer *Tokenizer, parse_tree_node *ParseTree);
bool ParseConditionalExpression(struct tokenizer *Tokenizer, parse_tree_node *ParseTree);
bool ParseStatement(struct tokenizer *Tokenizer, parse_tree_node *ParseTree);
bool ParseDeclarationList(struct tokenizer *Tokenizer, parse_tree_node *ParseTree);
bool ParseParameterTypeList(struct tokenizer *Tokenizer, parse_tree_node *ParseTree);
bool ParseAbstractDeclarator(struct tokenizer *Tokenizer, parse_tree_node *ParseTree);
bool ParsePointer(struct tokenizer *Tokenizer, parse_tree_node *ParseTree);
bool ParseSpecifierQualifierList(struct tokenizer *Tokenizer, parse_tree_node *ParseTree);
bool ParseInitializer(struct tokenizer *Tokenizer, parse_tree_node *ParseTree);
bool ParseDeclarationSpecifiers(struct tokenizer *Tokenizer, parse_tree_node *ParseTree);
bool ParseDeclarator(struct tokenizer *Tokenizer, parse_tree_node *ParseTree);
bool ParseTypeQualifier(struct tokenizer *Tokneizer, parse_tree_node *ParseTree);
bool ParseTypeSpecifier(struct tokenizer *Tokenizer, parse_tree_node *ParseTree);
bool ParseDeclaration(struct tokenizer *Tokenizer, parse_tree_node *ParseTree);

struct typedef_names
{
        char *Name;
        int *NameIndex;
        int Capacity; /* Total allocated space for Name */
        int NumNames; /* Number of Name */
};
struct typedef_names TypedefNames;

void
TypedefClear()
{
        free((void *)TypedefNames.Name);
        free((void *)TypedefNames.NameIndex);
        TypedefNames.Capacity = 0;
        TypedefNames.NumNames = 0;
}

void
TypedefInit()
{
        char *Memory = (char *)malloc(1024);
        TypedefNames.Name = Memory;
        TypedefNames.Capacity = 1024;
        TypedefNames.NameIndex = malloc(1024 / 2 * sizeof(int));
        TypedefNames.NumNames = 0;
}

bool
TypedefIsName(struct token Token)
{
        for(int i = 0; i < TypedefNames.NumNames; i++)
        {
                if(GSStringIsEqual(Token.Text, &TypedefNames.Name[TypedefNames.NameIndex[i]], Token.TextLength))
                {
                        return(true);
                }
        }
        return(false);
}

bool
TypedefAddName(char *Name)
{
        if(TypedefNames.NumNames == 0)
        {
                GSStringCopy(Name, TypedefNames.Name, GSStringLength(Name));
                TypedefNames.NumNames++;
                return(true);
        }

        int CurrentNameIndex = TypedefNames.NameIndex[TypedefNames.NumNames - 1];
        /* NameLength doesn't account for trailing NULL */
        int NameLength = GSStringLength(&TypedefNames.Name[CurrentNameIndex]);
        int UsedSpace = &TypedefNames.Name[CurrentNameIndex] - TypedefNames.Name + NameLength + 1;
        int RemainingCapacity = TypedefNames.Capacity - UsedSpace;

        int NewNameLength = GSStringLength(Name);
        if(NewNameLength + 1 > RemainingCapacity)
        {
                return(false);
        }

        GSStringCopy(Name, &TypedefNames.Name[CurrentNameIndex] + NameLength + 1, GSStringLength(Name));
        TypedefNames.NumNames++;
        return(true);
}

/*
  constant:
          integer-constant
          character-constant
          floating-constant
          enumeration-constant
*/
bool
ParseConstant(struct tokenizer *Tokenizer, parse_tree_node *ParseTree)
{
        struct tokenizer Start = *Tokenizer;
        struct token Token = GetToken(Tokenizer);

        switch(Token.Type)
        {
                case Token_Integer:
                case Token_Character:
                case Token_PrecisionNumber:
/*      TODO:   case Token_Enumeration:*/
                {
                        ParseTreeSet(ParseTree, "Constant", Token);
                        return(true);
                } break;
                default:
                {
                        *Tokenizer = Start;
                        return(false);
                }
        }
}

bool
ParseArgumentExpressionListI(struct tokenizer *Tokenizer, parse_tree_node *ParseTree)
{
        struct tokenizer Start = *Tokenizer;
        struct token Token;

        PARSE_TREE_UPDATE("Argument Expression List'", 3);

        if(Token_Comma == (Token = GetToken(Tokenizer)).Type &&
           ParseAssignmentExpression(Tokenizer, &ParseTree->Children[1]) &&
           ParseArgumentExpressionListI(Tokenizer, &ParseTree->Children[2]))
        {
                ParseTreeSet(&ParseTree->Children[0], "Symbol", Token);
                return(true);
        }

        *Tokenizer = Start;
        return(true);
}

/*
  argument-expression-list:
          assignment-expression
          argument-expression-list , assignment-expression
*/
bool
ParseArgumentExpressionList(struct tokenizer *Tokenizer, parse_tree_node *ParseTree)
{
        struct tokenizer Start = *Tokenizer;

        PARSE_TREE_UPDATE("Argument Expresion List", 2);

        if(ParseAssignmentExpression(Tokenizer, &ParseTree->Children[0]) &&
           ParseArgumentExpressionListI(Tokenizer, &ParseTree->Children[1]))
        {
                return(true);
        }

        *Tokenizer = Start;
        return(false);
}

/*
  primary-expression:
          identifier
          constant
          string
          ( expression )
*/
bool
ParsePrimaryExpression(struct tokenizer *Tokenizer, parse_tree_node *ParseTree)
{
        struct tokenizer Start = *Tokenizer;
        struct token Tokens[2];

        PARSE_TREE_UPDATE("Primary Expression", 3);

        if(Token_Identifier == (Tokens[0] = GetToken(Tokenizer)).Type)
        {
                ParseTreeSet(&ParseTree->Children[0], "Identifier", Tokens[0]);
                return(true);
        }

        *Tokenizer = Start;
        if(ParseConstant(Tokenizer, &ParseTree->Children[0])) return(true);

        *Tokenizer = Start;
        if(Token_String == (Tokens[0] = GetToken(Tokenizer)).Type)
        {
                ParseTreeSet(&ParseTree->Children[0], "String", Tokens[0]);
                return(true);
        }

        *Tokenizer = Start;
        if(Token_OpenParen == (Tokens[0] = GetToken(Tokenizer)).Type &&
           ParseExpression(Tokenizer, &ParseTree->Children[1]) &&
           Token_CloseParen == (Tokens[1] = GetToken(Tokenizer)).Type)
        {
                ParseTreeSet(&ParseTree->Children[0], "Symbol", Tokens[0]);
                ParseTreeSet(&ParseTree->Children[2], "Symbol", Tokens[1]);
                return true;
        }

        *Tokenizer = Start;
        return(false);
}

bool
ParsePostfixExpressionI(struct tokenizer *Tokenizer, parse_tree_node *ParseTree)
{
        struct tokenizer Start = *Tokenizer;
        struct token Tokens[2];

        PARSE_TREE_UPDATE("Postfix Expression'", 4);

        if(Token_OpenBracket == (Tokens[0] = GetToken(Tokenizer)).Type &&
           ParseExpression(Tokenizer, &ParseTree->Children[1]) &&
           Token_CloseBracket == (Tokens[0] = GetToken(Tokenizer)).Type &&
           ParsePostfixExpressionI(Tokenizer, &ParseTree->Children[3]))
        {
                ParseTreeSet(&ParseTree->Children[0], "Symbol", Tokens[0]);
                ParseTreeSet(&ParseTree->Children[2], "Symbol", Tokens[1]);
                return(true);
        }

        *Tokenizer = Start;
        if(Token_OpenParen == (Tokens[0] = GetToken(Tokenizer)).Type &&
           ParseArgumentExpressionList(Tokenizer, &ParseTree->Children[1]) &&
           Token_CloseParen == (Tokens[1] = GetToken(Tokenizer)).Type &&
           ParsePostfixExpressionI(Tokenizer, &ParseTree->Children[3]))
        {
                ParseTreeSet(&ParseTree->Children[0], "Symbol", Tokens[0]);
                ParseTreeSet(&ParseTree->Children[2], "Symbol", Tokens[1]);
                return(true);
        }

        PARSE_TREE_CLEAR_CHILDREN();

        *Tokenizer = Start;
        if(Token_OpenParen == (Tokens[0] = GetToken(Tokenizer)).Type &&
           Token_CloseParen == (Tokens[1] = GetToken(Tokenizer)).Type &&
           ParsePostfixExpressionI(Tokenizer, &ParseTree->Children[2]))
        {
                ParseTreeSet(&ParseTree->Children[0], "Symbol", Tokens[0]);
                ParseTreeSet(&ParseTree->Children[1], "Symbol", Tokens[1]);
                return(true);
        }

        *Tokenizer = Start;
        if(Token_Dot == (Tokens[0] = GetToken(Tokenizer)).Type &&
           Token_Identifier == (Tokens[1] = GetToken(Tokenizer)).Type &&
           ParsePostfixExpressionI(Tokenizer, &ParseTree->Children[2]))
        {
                ParseTreeSet(&ParseTree->Children[0], "Symbol", Tokens[0]);
                ParseTreeSet(&ParseTree->Children[1], "Identifier", Tokens[1]);
                return(true);
        }

        *Tokenizer = Start;
        if(Token_Arrow == (Tokens[0] = GetToken(Tokenizer)).Type &&
           Token_Identifier == (Tokens[1] = GetToken(Tokenizer)).Type &&
           ParsePostfixExpressionI(Tokenizer, &ParseTree->Children[2]))
        {
                ParseTreeSet(&ParseTree->Children[0], "Symbol", Tokens[0]);
                ParseTreeSet(&ParseTree->Children[1], "Identifier", Tokens[1]);
                return(true);
        }

        PARSE_TREE_CLEAR_CHILDREN();

        *Tokenizer = Start;
        if(Token_PlusPlus == (Tokens[0] = GetToken(Tokenizer)).Type &&
           ParsePostfixExpressionI(Tokenizer, &ParseTree->Children[1]))
        {
                ParseTreeSet(&ParseTree->Children[0], "Symbol", Tokens[0]);
                return(true);
        }

        *Tokenizer = Start;
        if(Token_MinusMinus == (Tokens[0] = GetToken(Tokenizer)).Type &&
           ParsePostfixExpressionI(Tokenizer, &ParseTree->Children[1]))
        {
                ParseTreeSet(&ParseTree->Children[0], "Symbol", Tokens[0]);
                return(true);
        }

        *Tokenizer = Start;
        return(true);
}

/*
  postfix-expression:
          primary-expression
          postfix-expression [ expression ]
          postfix-expression ( argument-expression-list(opt) )
          postfix-expression . identifier
          postfix-expression -> identifier
          postfix-expression ++
          postfix-expression --
*/
bool
ParsePostfixExpression(struct tokenizer *Tokenizer, parse_tree_node *ParseTree)
{
        struct tokenizer Start = *Tokenizer;

        PARSE_TREE_UPDATE("Postfix Expression", 2);

        if(ParsePrimaryExpression(Tokenizer, &ParseTree->Children[0]) &&
           ParsePostfixExpressionI(Tokenizer, &ParseTree->Children[1]))
        {
                return(true);
        }

        *Tokenizer = Start;
        return(false);
}

bool
ParseUnaryOperator(struct tokenizer *Tokenizer, parse_tree_node *ParseTree)
{
        struct tokenizer Start = *Tokenizer;
        struct token Token = GetToken(Tokenizer);
        switch(Token.Type)
        {
                case Token_Ampersand:
                case Token_Asterisk:
                case Token_Cross:
                case Token_Dash:
                case Token_Tilde:
                case Token_Bang:
                {
                        ParseTreeSet(ParseTree, "Unary Operator", Token);
                        return(true);
                }
                default:
                {
                        *Tokenizer = Start;
                        return(false);
                }
        }
}

/*
  unary-expression:
          postfix-expression
          ++ unary-expression
          -- unary-expression
          unary-operator cast-expression
          sizeof unary-expression
          sizeof ( type-name )
*/
bool
ParseUnaryExpression(struct tokenizer *Tokenizer, parse_tree_node *ParseTree)
{
        struct tokenizer Start = *Tokenizer;
        struct token Tokens[2];

        PARSE_TREE_UPDATE("Unary Expression", 4);

        if(ParsePostfixExpression(Tokenizer, &ParseTree->Children[0]))
        {
                return(true);
        }

        PARSE_TREE_CLEAR_CHILDREN();

        *Tokenizer = Start;
        if(Token_PlusPlus == (Tokens[0] = GetToken(Tokenizer)).Type &&
           ParseUnaryExpression(Tokenizer, &ParseTree->Children[1]))
        {
                ParseTreeSet(&ParseTree->Children[0], "Symbol", Tokens[0]);
                return(true);
        }

        PARSE_TREE_CLEAR_CHILDREN();

        *Tokenizer = Start;
        if(Token_MinusMinus == (Tokens[0] = GetToken(Tokenizer)).Type &&
           ParseUnaryExpression(Tokenizer, &ParseTree->Children[1]))
        {
                ParseTreeSet(&ParseTree->Children[0], "Symbol", Tokens[0]);
                return(true);
        }

        PARSE_TREE_CLEAR_CHILDREN();

        *Tokenizer = Start;
        if(ParseUnaryOperator(Tokenizer, &ParseTree->Children[0]) &&
           ParseCastExpression(Tokenizer, &ParseTree->Children[1]))
        {
                return(true);
        }

        PARSE_TREE_CLEAR_CHILDREN();

        *Tokenizer = Start;
        Tokens[0] = GetToken(Tokenizer);
        if(Token_Keyword == Tokens[0].Type &&
           GSStringIsEqual("sizeof", Tokens[0].Text, GSStringLength("sizeof")))
        {
                ParseTreeSet(&ParseTree->Children[0], "Keyword", Tokens[0]);

                struct tokenizer Previous = *Tokenizer;
                if(ParseUnaryExpression(Tokenizer, &ParseTree->Children[1]))
                {
                        return(true);
                }

                *Tokenizer = Previous;
                if(Token_OpenParen == (Tokens[0] = GetToken(Tokenizer)).Type &&
                   ParseTypeName(Tokenizer, &ParseTree->Children[2]) &&
                   Token_CloseParen == (Tokens[1] = GetToken(Tokenizer)).Type)
                {
                        ParseTreeSet(&ParseTree->Children[1], "Symbol", Tokens[0]);
                        ParseTreeSet(&ParseTree->Children[3], "Symbol", Tokens[1]);
                        return(true);
                }
        }

        *Tokenizer = Start;
        return(false);
}

/*
  cast-expression:
          unary-expression
          ( type-name ) cast-expression
*/
bool
ParseCastExpression(struct tokenizer *Tokenizer, parse_tree_node *ParseTree)
{
        struct tokenizer Start = *Tokenizer;
        struct token Tokens[2];

        PARSE_TREE_UPDATE("Cast Expression", 4);

        if(ParseUnaryExpression(Tokenizer, &ParseTree->Children[0]))
        {
                return(true);
        }

        *Tokenizer = Start;
        if(Token_OpenParen == (Tokens[0] = GetToken(Tokenizer)).Type &&
           ParseTypeName(Tokenizer, &ParseTree->Children[1]) &&
           Token_CloseParen == (Tokens[1] = GetToken(Tokenizer)).Type &&
           ParseCastExpression(Tokenizer, &ParseTree->Children[3]))
        {
                ParseTreeSet(&ParseTree->Children[0], "Symbol", Tokens[0]);
                ParseTreeSet(&ParseTree->Children[2], "Symbol", Tokens[1]);
                return(true);
        }

        *Tokenizer = Start;
        return(false);
}

bool
ParseMultiplicativeExpressionI(struct tokenizer *Tokenizer, parse_tree_node *ParseTree)
{
        struct tokenizer Start = *Tokenizer;
        struct token Token;

        PARSE_TREE_UPDATE("Multiplicative Expression", 3);

        if(Token_Asterisk == (Token = GetToken(Tokenizer)).Type &&
           ParseCastExpression(Tokenizer, &ParseTree->Children[1]) &&
           ParseMultiplicativeExpressionI(Tokenizer, &ParseTree->Children[2]))
        {
                ParseTreeSet(&ParseTree->Children[0], "Symbol", Token);
                return(true);
        }

        *Tokenizer = Start;
        if(Token_Slash == (Token = GetToken(Tokenizer)).Type &&
           ParseCastExpression(Tokenizer, &ParseTree->Children[1]) &&
           ParseMultiplicativeExpressionI(Tokenizer, &ParseTree->Children[2]))
        {
                ParseTreeSet(&ParseTree->Children[0], "Symbol", Token);
                return(true);
        }

        *Tokenizer = Start;
        if(Token_PercentSign == (Token = GetToken(Tokenizer)).Type &&
           ParseCastExpression(Tokenizer, &ParseTree->Children[1]) &&
           ParseMultiplicativeExpressionI(Tokenizer, &ParseTree->Children[2]))
        {
                ParseTreeSet(&ParseTree->Children[0], "Symbol", Token);
                return(true);
        }

        *Tokenizer = Start;
        return(true);
}

/*
  multiplicative-expression:
          cast-expression
          multiplicative-expression * cast-expression
          multiplicative-expression / cast-expression
          multiplicative-expression % cast-expression
*/
bool
ParseMultiplicativeExpression(struct tokenizer *Tokenizer, parse_tree_node *ParseTree)
{
        struct tokenizer Start = *Tokenizer;

        PARSE_TREE_UPDATE("Multiplicative Expression", 2);

        if(ParseCastExpression(Tokenizer, &ParseTree->Children[0]) &&
           ParseMultiplicativeExpressionI(Tokenizer, &ParseTree->Children[1]))
        {
                return(true);
        }

        *Tokenizer = Start;
        return(false);
}

bool
ParseAdditiveExpressionI(struct tokenizer *Tokenizer, parse_tree_node *ParseTree)
{
        struct tokenizer Start = *Tokenizer;
        struct token Token;

        PARSE_TREE_UPDATE("Additive Expression'", 3);

        if(Token_Cross == (Token = GetToken(Tokenizer)).Type &&
           ParseMultiplicativeExpression(Tokenizer, &ParseTree->Children[1]) &&
           ParseAdditiveExpressionI(Tokenizer, &ParseTree->Children[2]))
        {
                ParseTreeSet(&ParseTree->Children[0], "Symbol", Token);
                return(true);
        }

        *Tokenizer = Start;
        if(Token_Dash == (Token = GetToken(Tokenizer)).Type &&
           ParseMultiplicativeExpression(Tokenizer, &ParseTree->Children[1]) &&
           ParseAdditiveExpressionI(Tokenizer, &ParseTree->Children[2]))
        {
                ParseTreeSet(&ParseTree->Children[0], "Symbol", Token);
                return(true);
        }

        *Tokenizer = Start;
        return(true);
}

/*
  additive-expression:
          multiplicative-expression
          additive-expression + multiplicative-expression
          additive-expression - multiplicative-expression
*/
bool
ParseAdditiveExpression(struct tokenizer *Tokenizer, parse_tree_node *ParseTree)
{
        struct tokenizer Start = *Tokenizer;

        PARSE_TREE_UPDATE("Additive Expression", 2);

        if(ParseMultiplicativeExpression(Tokenizer, &ParseTree->Children[0]) &&
           ParseAdditiveExpressionI(Tokenizer, &ParseTree->Children[1]))
        {
                return(true);
        }

        *Tokenizer = Start;
        return(false);
}

bool
ParseShiftExpressionI(struct tokenizer *Tokenizer, parse_tree_node *ParseTree)
{
        struct tokenizer Start = *Tokenizer;
        struct token Token;

        PARSE_TREE_UPDATE("Shift Expression'", 3);

        if(Token_BitShiftLeft == (Token = GetToken(Tokenizer)).Type &&
           ParseAdditiveExpression(Tokenizer, &ParseTree->Children[1]) &&
           ParseShiftExpressionI(Tokenizer, &ParseTree->Children[2]))
        {
                ParseTreeSet(&ParseTree->Children[0], "Symbol", Token);
                return(true);
        }

        *Tokenizer = Start;
        if(Token_BitShiftRight == (Token = GetToken(Tokenizer)).Type &&
           ParseAdditiveExpression(Tokenizer, &ParseTree->Children[1]) &&
           ParseShiftExpressionI(Tokenizer, &ParseTree->Children[2]))
        {
                ParseTreeSet(&ParseTree->Children[0], "Symbol", Token);
                return(true);
        }

        *Tokenizer = Start;
        return(true);
}

/*
  shift-expression:
          additive-expression
          shift-expression << additive-expression
          shift-expression >> additive-expression
*/
bool
ParseShiftExpression(struct tokenizer *Tokenizer, parse_tree_node *ParseTree)
{
        struct tokenizer Start = *Tokenizer;

        PARSE_TREE_UPDATE("Shift Expression", 2);

        if(ParseAdditiveExpression(Tokenizer, &ParseTree->Children[0]) &&
           ParseShiftExpressionI(Tokenizer, &ParseTree->Children[1]))
        {
                return(true);
        }

        *Tokenizer = Start;
        return(false);
}

bool
ParseRelationalExpressionI(struct tokenizer *Tokenizer, parse_tree_node *ParseTree)
{
        struct tokenizer Start = *Tokenizer;
        struct token Token;

        PARSE_TREE_UPDATE("Relational Expression'", 3);

        if(Token_LessThan == (Token = GetToken(Tokenizer)).Type &&
           ParseShiftExpression(Tokenizer, &ParseTree->Children[1]) &&
           ParseRelationalExpressionI(Tokenizer, &ParseTree->Children[2]))
        {
                ParseTreeSet(&ParseTree->Children[0], "Symbol", Token);
                return(true);
        }

        *Tokenizer = Start;
        if(Token_GreaterThan == (Token = GetToken(Tokenizer)).Type &&
           ParseShiftExpression(Tokenizer, &ParseTree->Children[1]) &&
           ParseRelationalExpressionI(Tokenizer, &ParseTree->Children[2]))
        {
                ParseTreeSet(&ParseTree->Children[0], "Symbol", Token);
                return(true);
        }

        *Tokenizer = Start;
        if(Token_LessThanEqual == (Token = GetToken(Tokenizer)).Type &&
           ParseShiftExpression(Tokenizer, &ParseTree->Children[1]) &&
           ParseRelationalExpressionI(Tokenizer, &ParseTree->Children[2]))
        {
                ParseTreeSet(&ParseTree->Children[0], "Symbol", Token);
                return(true);
        }

        *Tokenizer = Start;
        if(Token_GreaterThanEqual == (Token = GetToken(Tokenizer)).Type &&
           ParseShiftExpression(Tokenizer, &ParseTree->Children[1]) &&
           ParseRelationalExpressionI(Tokenizer, &ParseTree->Children[2]))
        {
                ParseTreeSet(&ParseTree->Children[0], "Symbol", Token);
                return(true);
        }

        *Tokenizer = Start;
        return(true);
}

/*
  relational-expression:
          shift-expression
          relational-expression < shift-expression
          relational-expression > shift-expression
          relational-expression <= shift-exression
          relational-expression >= shift-expression
*/
bool
ParseRelationalExpression(struct tokenizer *Tokenizer, parse_tree_node *ParseTree)
{
        struct tokenizer Start = *Tokenizer;

        PARSE_TREE_UPDATE("Relational Expressoin", 2);

        if(ParseShiftExpression(Tokenizer, &ParseTree->Children[0]) &&
           ParseRelationalExpressionI(Tokenizer, &ParseTree->Children[1]))
        {
                return(true);
        }

        *Tokenizer = Start;
        return(false);
}

bool
ParseEqualityExpressionI(struct tokenizer *Tokenizer, parse_tree_node *ParseTree)
{
        struct tokenizer Start = *Tokenizer;
        struct token Token;

        PARSE_TREE_UPDATE("Equality Expression'", 3);

        if(Token_LogicalEqual == (Token = GetToken(Tokenizer)).Type &&
           ParseRelationalExpression(Tokenizer, &ParseTree->Children[1]) &&
           ParseEqualityExpressionI(Tokenizer, &ParseTree->Children[2]))
        {
                ParseTreeSet(&ParseTree->Children[0], "Symbol", Token);
                return(true);
        }

        *Tokenizer = Start;
        if(Token_NotEqual == (Token = GetToken(Tokenizer)).Type &&
           ParseRelationalExpression(Tokenizer, &ParseTree->Children[1]) &&
           ParseEqualityExpressionI(Tokenizer, &ParseTree->Children[2]))
        {
                ParseTreeSet(&ParseTree->Children[0], "Symbol", Token);
                return(true);
        }

        *Tokenizer = Start;
        return(true);
}

/*
  equality-expression:
          relational-expression
          equality-expression == relational-expression
          equality-expression != relational-expression
*/
bool
ParseEqualityExpression(struct tokenizer *Tokenizer, parse_tree_node *ParseTree)
{
        struct tokenizer Start = *Tokenizer;

        PARSE_TREE_UPDATE("Equality Expression", 2);

        if(ParseRelationalExpression(Tokenizer, &ParseTree->Children[0]) &&
           ParseEqualityExpressionI(Tokenizer, &ParseTree->Children[1]))
        {
                return(true);
        }

        *Tokenizer = Start;
        return(false);
}

bool
ParseAndExpressionI(struct tokenizer *Tokenizer, parse_tree_node *ParseTree)
{
        struct tokenizer Start = *Tokenizer;
        struct token Token;

        PARSE_TREE_UPDATE("And Expression'", 3);

        if(Token_Ampersand == (Token = GetToken(Tokenizer)).Type &&
           ParseEqualityExpression(Tokenizer, &ParseTree->Children[1]) &&
           ParseAndExpressionI(Tokenizer, &ParseTree->Children[2]))
        {
                ParseTreeSet(&ParseTree->Children[0], "Symbol", Token);
                return(true);
        }

        *Tokenizer = Start;
        return(true);
}

/*
  AND-expression:
          equality-expression
          AND-expression & equality-expression
*/
bool
ParseAndExpression(struct tokenizer *Tokenizer, parse_tree_node *ParseTree)
{
        struct tokenizer Start = *Tokenizer;

        PARSE_TREE_UPDATE("And Expression", 2);

        if(ParseEqualityExpression(Tokenizer, &ParseTree->Children[0]) &&
           ParseAndExpressionI(Tokenizer, &ParseTree->Children[1]))
        {
                return(true);
        }

        *Tokenizer = Start;
        return(false);
}

bool
ParseExclusiveOrExpressionI(struct tokenizer *Tokenizer, parse_tree_node *ParseTree)
{
        struct tokenizer Start = *Tokenizer;
        struct token Token;

        PARSE_TREE_UPDATE("Exclusive Or Expression'", 3);

        if(Token_Carat == (Token = GetToken(Tokenizer)).Type &&
           ParseAndExpression(Tokenizer, &ParseTree->Children[1]) &&
           ParseExclusiveOrExpressionI(Tokenizer, &ParseTree->Children[2]))
        {
                ParseTreeSet(&ParseTree->Children[0], "Symbol", Token);
                return(true);
        }

        *Tokenizer = Start;
        return(true);
}

/*
  exclusive-OR-expression
          AND-expression
          exclusive-OR-expression ^ AND-expression
 */
bool
ParseExclusiveOrExpression(struct tokenizer *Tokenizer, parse_tree_node *ParseTree)
{
        struct tokenizer Start = *Tokenizer;

        PARSE_TREE_UPDATE("Exclusive Or Expression", 2);

        if(ParseAndExpression(Tokenizer, &ParseTree->Children[0]) &&
           ParseExclusiveOrExpressionI(Tokenizer, &ParseTree->Children[1]))
        {
                return(true);
        }

        *Tokenizer = Start;
        return(false);
}

bool
ParseInclusiveOrExpressionI(struct tokenizer *Tokenizer, parse_tree_node *ParseTree)
{
        struct tokenizer Start = *Tokenizer;
        struct token Token;

        PARSE_TREE_UPDATE("Inclusive Or Expression'", 3);

        if(Token_Pipe == (Token = GetToken(Tokenizer)).Type &&
           ParseExclusiveOrExpression(Tokenizer, &ParseTree->Children[1]) &&
           ParseInclusiveOrExpressionI(Tokenizer, &ParseTree->Children[2]))
        {
                ParseTreeSet(&ParseTree->Children[0], "Symbol", Token);
                return(true);
        }

        *Tokenizer = Start;
        return(true);
}

/*
  inclusive-OR-expression:
          exclusive-OR-expression
          inclusive-OR-expression | exclusive-OR-expression
*/
bool
ParseInclusiveOrExpression(struct tokenizer *Tokenizer, parse_tree_node *ParseTree)
{
        struct tokenizer Start = *Tokenizer;

        PARSE_TREE_UPDATE("Inclusive Or Expression", 2);

        if(ParseExclusiveOrExpression(Tokenizer, &ParseTree->Children[0]) &&
           ParseInclusiveOrExpressionI(Tokenizer, &ParseTree->Children[1]))
        {
                return(true);
        }

        *Tokenizer = Start;
        return(false);
}

bool
ParseLogicalAndExpressionI(struct tokenizer *Tokenizer, parse_tree_node *ParseTree)
{
        struct tokenizer Start = *Tokenizer;
        struct token Token;

        PARSE_TREE_UPDATE("Logical And Expression'", 3);

        if(Token_LogicalAnd == (Token = GetToken(Tokenizer)).Type &&
           ParseInclusiveOrExpression(Tokenizer, &ParseTree->Children[1]) &&
           ParseLogicalAndExpressionI(Tokenizer, &ParseTree->Children[2]))
        {
                ParseTreeSet(&ParseTree->Children[0], "Symbol", Token);
                return(true);
        }

        *Tokenizer = Start;
        return(true);
}

/*
  logical-AND-expression:
          inclusive-OR-expression
          logical-AND-expression && inclusive-OR-expression
*/
bool
ParseLogicalAndExpression(struct tokenizer *Tokenizer, parse_tree_node *ParseTree)
{
        struct tokenizer Start = *Tokenizer;

        PARSE_TREE_UPDATE("Logical And Expression", 2);

        if(ParseInclusiveOrExpression(Tokenizer, &ParseTree->Children[0]) &&
           ParseLogicalAndExpressionI(Tokenizer, &ParseTree->Children[1]))
        {
                return(true);
        }

        *Tokenizer = Start;
        return(false);
}

bool
ParseLogicalOrExpressionI(struct tokenizer *Tokenizer, parse_tree_node *ParseTree)
{
        struct tokenizer Start = *Tokenizer;
        struct token Token;

        PARSE_TREE_UPDATE("Logical Or Expression'", 3);

        if(Token_LogicalOr == (Token = GetToken(Tokenizer)).Type &&
           ParseLogicalAndExpression(Tokenizer, &ParseTree->Children[1]) &&
           ParseLogicalOrExpressionI(Tokenizer, &ParseTree->Children[2]))
        {
                ParseTreeSet(&ParseTree->Children[0], "Symbol", Token);
                return(true);
        }

        *Tokenizer = Start;
        return(true);
}

/*
  logical-OR-expression:
          logical-AND-expression
          logical-OR-expression || logical-AND-expression
*/
bool
ParseLogicalOrExpression(struct tokenizer *Tokenizer, parse_tree_node *ParseTree)
{
        struct tokenizer Start = *Tokenizer;

        PARSE_TREE_UPDATE("Logical Or Expression", 2);

        if(ParseLogicalAndExpression(Tokenizer, &ParseTree->Children[0]) &&
           ParseLogicalOrExpressionI(Tokenizer, &ParseTree->Children[1]))
        {
                return(true);
        }

        *Tokenizer = Start;
        return(false);
}

/*
  constant-expression:
          conditional-expression
*/
bool
ParseConstantExpression(struct tokenizer *Tokenizer, parse_tree_node *ParseTree)
{
        struct tokenizer Start = *Tokenizer;

        PARSE_TREE_UPDATE("Constant Expression", 1);

        if(ParseConditionalExpression(Tokenizer, &ParseTree->Children[0]))
        {
                return(true);
        }

        *Tokenizer = Start;
        return(false);
}

/*
  conditional-expression:
          logical-OR-expression
          logical-OR-expression ? expression : conditional-expression
*/
bool
ParseConditionalExpression(struct tokenizer *Tokenizer, parse_tree_node *ParseTree)
{
        struct tokenizer Start = *Tokenizer;
        struct token Tokens[2];

        PARSE_TREE_UPDATE("Conditional Expression", 5);

        if(ParseLogicalOrExpression(Tokenizer, &ParseTree->Children[0]) &&
           Token_QuestionMark == (Tokens[0] = GetToken(Tokenizer)).Type &&
           ParseExpression(Tokenizer, &ParseTree->Children[2]) &&
           Token_Colon == (Tokens[1] = GetToken(Tokenizer)).Type &&
           ParseConditionalExpression(Tokenizer, &ParseTree->Children[4]))
        {
                ParseTreeSet(&ParseTree->Children[1], "Symbol", Tokens[0]);
                ParseTreeSet(&ParseTree->Children[3], "Symbol", Tokens[1]);
                return(true);
        }

        PARSE_TREE_CLEAR_CHILDREN();

        *Tokenizer = Start;
        if(ParseLogicalOrExpression(Tokenizer, &ParseTree->Children[0]))
        {
                return(true);
        }

        *Tokenizer = Start;
        return(false);
}

/*
  assignment-operator:
  one of: = *= /= %= += -= <<= >>= &= ^= |=
*/
bool
ParseAssignmentOperator(struct tokenizer *Tokenizer, parse_tree_node *ParseTree)
{
        struct tokenizer Start = *Tokenizer;
        struct token Token = GetToken(Tokenizer);

        switch(Token.Type)
        {
                case Token_EqualSign:
                case Token_MultiplyEquals:
                case Token_DivideEquals:
                case Token_ModuloEquals:
                case Token_PlusEquals:
                case Token_MinusEquals:
                case Token_DoubleLessThanEquals:
                case Token_DoubleGreaterThanEquals:
                case Token_AmpersandEquals:
                case Token_CaratEquals:
                case Token_PipeEquals:
                {
                        ParseTreeSet(ParseTree, "Assignment Operator", Token);
                        return(true);
                }
        }

        *Tokenizer = Start;
        return(false);
}

/*
  assignment-expression:
          conditional-expression
          unary-expression assignment-operator assignment-expression
*/
bool
ParseAssignmentExpression(struct tokenizer *Tokenizer, parse_tree_node *ParseTree)
{
        struct tokenizer Start = *Tokenizer;

        PARSE_TREE_UPDATE("Assignment Expression", 3);

        if(ParseUnaryExpression(Tokenizer, &ParseTree->Children[0]) &&
           ParseAssignmentOperator(Tokenizer, &ParseTree->Children[1]) &&
           ParseAssignmentExpression(Tokenizer, &ParseTree->Children[2]))
        {
                return(true);
        }

        PARSE_TREE_CLEAR_CHILDREN();

        *Tokenizer = Start;
        if(ParseConditionalExpression(Tokenizer, &ParseTree->Children[0]))
        {
                return(true);
        }

        *Tokenizer = Start;
        return(false);
}

bool
ParseExpressionI(struct tokenizer *Tokenizer, parse_tree_node *ParseTree)
{
        struct tokenizer Start = *Tokenizer;
        struct token Token;

        PARSE_TREE_UPDATE("Expression'", 3);

        if(Token_Comma == (Token = GetToken(Tokenizer)).Type &&
           ParseAssignmentExpression(Tokenizer, &ParseTree->Children[1]) &&
           ParseExpressionI(Tokenizer, &ParseTree->Children[2]))
        {
                ParseTreeSet(&ParseTree->Children[0], "Symbol", Token);
                return(true);
        }

        *Tokenizer = Start;
        return(true);
}

/*
  expression:
          assignment-expression
          expression , assignment-expression
*/
bool
ParseExpression(struct tokenizer *Tokenizer, parse_tree_node *ParseTree)
{
        struct tokenizer Start = *Tokenizer;

        PARSE_TREE_UPDATE("Expression", 2);

        if(ParseAssignmentExpression(Tokenizer, &ParseTree->Children[0]) &&
           ParseExpressionI(Tokenizer, &ParseTree->Children[1]))
        {
                return(true);
        }

        *Tokenizer = Start;
        return(false);
}

bool
ParseIdentifier(struct tokenizer *Tokenizer, parse_tree_node *ParseTree)
{
        struct tokenizer Start = *Tokenizer;
        struct token Token;

        if(Token_Identifier == (Token = GetToken(Tokenizer)).Type)
        {
                ParseTreeSet(ParseTree, "Identifier", Token);
                return(true);
        }

        *Tokenizer = Start;
        return(false);
}

/*
  jump-statement:
          goto identifier ;
          continue ;
          break ;
          return expression(opt) ;
*/
bool
ParseJumpStatement(struct tokenizer *Tokenizer, parse_tree_node *ParseTree)
{
        struct tokenizer Start = *Tokenizer;
        struct token Tokens[2];
        Tokens[0] = GetToken(Tokenizer);
        struct tokenizer AtToken = *Tokenizer;

        PARSE_TREE_UPDATE("Jump Statement", 3);

        if(Token_Keyword == Tokens[0].Type &&
           GSStringIsEqual("goto", Tokens[0].Text, Tokens[0].TextLength) &&
           ParseIdentifier(Tokenizer, &ParseTree->Children[1]) &&
           Token_SemiColon == (Tokens[1] = GetToken(Tokenizer)).Type)
        {
                ParseTreeSet(&ParseTree->Children[0], "Keyword", Tokens[0]);
                ParseTreeSet(&ParseTree->Children[2], "Symbol", Tokens[1]);
                return(true);
        }

        PARSE_TREE_CLEAR_CHILDREN();

        *Tokenizer = AtToken;
        if(Token_Keyword == Tokens[0].Type &&
           GSStringIsEqual("continue", Tokens[0].Text, Tokens[0].TextLength) &&
           Token_SemiColon == (Tokens[1] = GetToken(Tokenizer)).Type)
        {
                ParseTreeSet(&ParseTree->Children[0], "Keyword", Tokens[0]);
                ParseTreeSet(&ParseTree->Children[1], "Symbol", Tokens[1]);
                return(true);
        }

        PARSE_TREE_CLEAR_CHILDREN();

        *Tokenizer = AtToken;
        if(Token_Keyword == Tokens[0].Type &&
           GSStringIsEqual("break", Tokens[0].Text, Tokens[0].TextLength) &&
           Token_SemiColon == (Tokens[1] = GetToken(Tokenizer)).Type)
        {
                ParseTreeSet(&ParseTree->Children[0], "Keyword", Tokens[0]);
                ParseTreeSet(&ParseTree->Children[1], "Symbol", Tokens[1]);
                return(true);
        }

        PARSE_TREE_CLEAR_CHILDREN();

        *Tokenizer = AtToken;
        if(Token_Keyword == Tokens[0].Type &&
           GSStringIsEqual("return", Tokens[0].Text, Tokens[0].TextLength))
        {
                ParseTreeSet(&ParseTree->Children[0], "Keyword", Tokens[0]);
                int ChildIndex = 1;
                struct tokenizer Previous = *Tokenizer;

                if(!ParseExpression(Tokenizer, &ParseTree->Children[ChildIndex++]))
                {
                        --ChildIndex;
                        *Tokenizer = Previous;
                }

                if(Token_SemiColon == (Tokens[1] = GetToken(Tokenizer)).Type)
                {
                        ParseTreeSet(&ParseTree->Children[ChildIndex], "Symbol", Tokens[1]);
                        return(true);
                }
        }

        *Tokenizer = Start;
        return(false);
}

/*
  iteration-statement:
          while ( expression ) statement
          do statement while ( expression) ;
          for ( expression(opt) ; expression(opt) ; expression(opt) ) statement
*/
bool
ParseIterationStatement(struct tokenizer *Tokenizer, parse_tree_node *ParseTree)
{
        struct tokenizer Start = *Tokenizer;
        struct token Tokens[5];
        Tokens[0] = GetToken(Tokenizer);
        struct tokenizer AtToken = *Tokenizer;

        PARSE_TREE_UPDATE("Iteration Statement", 10); /* TODO(AARON): Magic Number! */

        if(Token_Keyword == Tokens[0].Type &&
           GSStringIsEqual("while", Tokens[0].Text, Tokens[0].TextLength) &&
           Token_OpenParen == (Tokens[1] = GetToken(Tokenizer)).Type &&
           ParseExpression(Tokenizer, &ParseTree->Children[2]) &&
           Token_CloseParen == (Tokens[2] = GetToken(Tokenizer)).Type &&
           ParseStatement(Tokenizer, &ParseTree->Children[4]))
        {
                ParseTreeSet(&ParseTree->Children[0], "Keyword", Tokens[0]);
                ParseTreeSet(&ParseTree->Children[1], "Symbol", Tokens[1]);
                ParseTreeSet(&ParseTree->Children[3], "Symbol", Tokens[2]);
                return(true);
        }

        PARSE_TREE_CLEAR_CHILDREN();

        *Tokenizer = AtToken;
        if(Token_Keyword == Tokens[0].Type &&
           GSStringIsEqual("do", Tokens[0].Text, Tokens[0].TextLength) &&
           ParseStatement(Tokenizer, &ParseTree->Children[1]))
        {
                parse_tree_node *ChildNode = &ParseTree->Children[0];
                ParseTreeSet(ChildNode, "Keyword", Tokens[0]);

                Tokens[1] = GetToken(Tokenizer);
                if(Token_Keyword == Tokens[1].Type &&
                   GSStringIsEqual("while", Tokens[1].Text, Tokens[1].TextLength) &&
                   Token_OpenParen == (Tokens[2] = GetToken(Tokenizer)).Type &&
                   ParseExpression(Tokenizer, &ParseTree->Children[4]) &&
                   Token_CloseParen == (Tokens[3] = GetToken(Tokenizer)).Type &&
                   Token_SemiColon == (Tokens[4] = GetToken(Tokenizer)).Type)
                {
                        ParseTreeSet(++ChildNode, "Keyword", Tokens[1]);
                        ParseTreeSet(++ChildNode, "Symbol", Tokens[2]);
                        ++ChildNode; // Child #4 is set in the `if' above.
                        ParseTreeSet(++ChildNode, "Symbol", Tokens[3]);
                        ParseTreeSet(++ChildNode, "Symbol", Tokens[4]);
                        return(true);
                }
        }

        PARSE_TREE_CLEAR_CHILDREN();

        *Tokenizer = AtToken;
        if(Token_Keyword == Tokens[0].Type &&
           GSStringIsEqual("for", Tokens[0].Text, Tokens[0].TextLength) &&
           Token_OpenParen == (Tokens[1] = GetToken(Tokenizer)).Type)
        {
                ParseTreeSet(&ParseTree->Children[0], "Keyword", Tokens[0]);
                ParseTreeSet(&ParseTree->Children[1], "Symbol", Tokens[1]);

                int ChildIndex = 2;

                struct tokenizer Previous = *Tokenizer;
                if(!ParseExpression(Tokenizer, &ParseTree->Children[ChildIndex]))
                {
                        *Tokenizer = Previous;
                }
                else
                {
                        ChildIndex++;
                }

                if(Token_SemiColon != (Tokens[2] = GetToken(Tokenizer)).Type)
                {
                        *Tokenizer = Start;
                        return(false);
                }
                ParseTreeSet(&ParseTree->Children[ChildIndex++], "Symbol", Tokens[2]);

                Previous = *Tokenizer;
                if(!ParseExpression(Tokenizer, &ParseTree->Children[ChildIndex++]))
                {
                        --ChildIndex;
                        *Tokenizer = Previous;
                }

                if(Token_SemiColon != (Tokens[3] = GetToken(Tokenizer)).Type)
                {
                        *Tokenizer = Start;
                        return(false);
                }
                ParseTreeSet(&ParseTree->Children[ChildIndex++], "Symbol", Tokens[3]);

                Previous = *Tokenizer;
                if(!ParseExpression(Tokenizer, &ParseTree->Children[ChildIndex++]))
                {
                        --ChildIndex;
                        *Tokenizer = Previous;
                }

                if(Token_CloseParen == (Tokens[4] = GetToken(Tokenizer)).Type &&
                   ParseStatement(Tokenizer, &ParseTree->Children[ChildIndex + 1]))
                {
                        ParseTreeSet(&ParseTree->Children[ChildIndex], "Symbol", Tokens[4]);
                        return(true);
                }
        }

        *Tokenizer = Start;
        return(false);
}

/*
  selection-statement:
          if ( expression ) statement
          if ( expression ) statement else statement
          switch ( expression ) statement
*/
bool
ParseSelectionStatement(struct tokenizer *Tokenizer, parse_tree_node *ParseTree)
{
        struct tokenizer Start = *Tokenizer;
        struct token Tokens[3];
        Tokens[0] = GetToken(Tokenizer);
        struct tokenizer AtToken = *Tokenizer;

        PARSE_TREE_UPDATE("Selection Statement", 6);

        if(Token_Keyword == Tokens[0].Type &&
           GSStringIsEqual("if", Tokens[0].Text, Tokens[0].TextLength) &&
           Token_OpenParen == (Tokens[1] = GetToken(Tokenizer)).Type &&
           ParseExpression(Tokenizer, &ParseTree->Children[2]) &&
           Token_CloseParen == (Tokens[2] = GetToken(Tokenizer)).Type &&
           ParseStatement(Tokenizer, &ParseTree->Children[4]))
        {
                struct tokenizer AtElse = *Tokenizer;
                struct token Token = GetToken(Tokenizer);

                ParseTreeSet(&ParseTree->Children[0], "Keyword", Tokens[0]);
                ParseTreeSet(&ParseTree->Children[1], "Symbol", Tokens[1]);
                ParseTreeSet(&ParseTree->Children[3], "Symbol", Tokens[2]);

                if(Token_Keyword == Token.Type &&
                   GSStringIsEqual("else", Token.Text, Token.TextLength) &&
                   ParseStatement(Tokenizer, &ParseTree->Children[5]))
                {
                        ParseTreeSet(&ParseTree->Children[4], "Keyword", Token);
                        return(true);
                }

                *Tokenizer = AtElse;
                return(true);
        }

        PARSE_TREE_CLEAR_CHILDREN();

        *Tokenizer = AtToken;
        if(Token_Keyword == Tokens[0].Type &&
           GSStringIsEqual("switch", Tokens[0].Text, Tokens[0].TextLength) &&
           Token_OpenParen == (Tokens[1] = GetToken(Tokenizer)).Type &&
           ParseExpression(Tokenizer, &ParseTree->Children[2]) &&
           Token_CloseParen == (Tokens[2] = GetToken(Tokenizer)).Type &&
           ParseStatement(Tokenizer, &ParseTree->Children[4]))
        {
                ParseTreeSet(&ParseTree->Children[0], "Keyword", Tokens[0]);
                ParseTreeSet(&ParseTree->Children[1], "Symbol", Tokens[1]);
                ParseTreeSet(&ParseTree->Children[3], "Symbol", Tokens[2]);
                return(true);
        }

        *Tokenizer = Start;
        return(false);
}

bool
ParseStatementListI(struct tokenizer *Tokenizer, parse_tree_node *ParseTree)
{
        struct tokenizer Start = *Tokenizer;

        PARSE_TREE_UPDATE("Statement List'", 2);

        if(ParseStatement(Tokenizer, &ParseTree->Children[0]) &&
           ParseStatementListI(Tokenizer, &ParseTree->Children[1]))
        {
                return(true);
        }

        *Tokenizer = Start;
        return(true);
}

/*
  statement-list:
          statement
          statement-list statement
*/
bool
ParseStatementList(struct tokenizer *Tokenizer, parse_tree_node *ParseTree)
{
        struct tokenizer Start = *Tokenizer;

        PARSE_TREE_UPDATE("Statement List", 2);

        if(ParseStatement(Tokenizer, &ParseTree->Children[0]) &&
           ParseStatementListI(Tokenizer, &ParseTree->Children[1]))
        {
                return(true);
        }

        *Tokenizer = Start;
        return(false);
}

/*
  compound-statement:
          { declaration-list(opt) statement-list(opt) }
*/
bool
ParseCompoundStatement(struct tokenizer *Tokenizer, parse_tree_node *ParseTree)
{
        struct tokenizer Start = *Tokenizer;
        token Token;

        PARSE_TREE_UPDATE("Compound Statement", 3);

        if(Token_OpenBrace == (Token = GetToken(Tokenizer)).Type)
        {
                ParseTreeSet(&ParseTree->Children[0], "Symbol", Token);
                int ChildIndex = 0;

                struct tokenizer Previous = *Tokenizer;
                if(!ParseDeclarationList(Tokenizer, &ParseTree->Children[ChildIndex++]))
                {
                        --ChildIndex;
                        *Tokenizer = Previous;
                }

                Previous = *Tokenizer;
                if(!ParseStatementList(Tokenizer, &ParseTree->Children[ChildIndex++]))
                {
                        --ChildIndex;
                        *Tokenizer = Previous;
                }

                if(Token_CloseBrace == (Token = GetToken(Tokenizer)).Type)
                {
                        ParseTreeSet(&ParseTree->Children[ChildIndex], "Symbol", Token);
                        return(true);
                }
        }

        *Tokenizer = Start;
        return(false);
}

/*
  expression-statement:
          expression(opt) ;
*/
bool
ParseExpressionStatement(struct tokenizer *Tokenizer, parse_tree_node *ParseTree)
{
        struct tokenizer Start = *Tokenizer;
        token Token;

        PARSE_TREE_UPDATE("Expression Statement", 2);

        if(ParseExpression(Tokenizer, &ParseTree->Children[0]) &&
           Token_SemiColon == (Token = GetToken(Tokenizer)).Type)
        {
                ParseTreeSet(&ParseTree->Children[1], "Symbol", Token);
                return(true);
        }

        PARSE_TREE_CLEAR_CHILDREN();

        *Tokenizer = Start;
        if(Token_SemiColon == (Token = GetToken(Tokenizer)).Type)
        {
                ParseTreeSet(&ParseTree->Children[0], "Symbol", Token);
                return(true);
        }

        *Tokenizer = Start;
        return(false);
}

/*
  labeled-statement:
          identifier : statement
          case constant-expression : statement
          default : statement
*/
bool
ParseLabeledStatement(struct tokenizer *Tokenizer, parse_tree_node *ParseTree)
{
        struct tokenizer Start = *Tokenizer;
        token Tokens[2];

        PARSE_TREE_UPDATE("Labeled Statement", 4);

        if(ParseIdentifier(Tokenizer, &ParseTree->Children[0]) &&
           Token_Colon == (Tokens[0] = GetToken(Tokenizer)).Type &&
           ParseStatement(Tokenizer, &ParseTree->Children[2]))
        {
                ParseTreeSet(&ParseTree->Children[1], "Symbol", Tokens[0]);
                return(true);
        }

        *Tokenizer = Start;
        Tokens[0] = GetToken(Tokenizer);
        struct tokenizer AtToken = *Tokenizer;

        if(Token_Keyword == Tokens[0].Type &&
           GSStringIsEqual("case", Tokens[0].Text, Tokens[0].TextLength) &&
           ParseConstantExpression(Tokenizer, &ParseTree->Children[1]) &&
           Token_Colon == (Tokens[1] = GetToken(Tokenizer)).Type &&
           ParseStatement(Tokenizer, &ParseTree->Children[3]))
        {
                ParseTreeSet(&ParseTree->Children[0], "Keyword", Tokens[0]);
                ParseTreeSet(&ParseTree->Children[2], "Symbol", Tokens[1]);
                return(true);
        }

        PARSE_TREE_CLEAR_CHILDREN();

        *Tokenizer = AtToken;
        if(Token_Keyword == Tokens[0].Type &&
           GSStringIsEqual("default", Tokens[0].Text, Tokens[0].TextLength) &&
           Token_Colon == (Tokens[1] = GetToken(Tokenizer)).Type &&
           ParseStatement(Tokenizer, &ParseTree->Children[2]))
        {
                ParseTreeSet(&ParseTree->Children[0], "Keyword", Tokens[0]);
                ParseTreeSet(&ParseTree->Children[2], "Symbol", Tokens[1]);
                return(true);
        }

        *Tokenizer = Start;
        return(false);
}

/*
  statement:
          labeled-statement
          expression-statement
          compound-statement
          selection-statement
          iteration-statement
          jump-statement
*/
bool
ParseStatement(struct tokenizer *Tokenizer, parse_tree_node *ParseTree)
{
        struct tokenizer Start = *Tokenizer;

        PARSE_TREE_UPDATE("Statement", 1);
        parse_tree_node *Child = &ParseTree->Children[0];

        if(ParseLabeledStatement(Tokenizer, Child)) return(true);

        *Tokenizer = Start;
        if(ParseExpressionStatement(Tokenizer, Child)) return(true);

        *Tokenizer = Start;
        if(ParseCompoundStatement(Tokenizer, Child)) return(true);

        *Tokenizer = Start;
        if(ParseSelectionStatement(Tokenizer, Child)) return(true);

        *Tokenizer = Start;
        if(ParseIterationStatement(Tokenizer, Child)) return(true);

        *Tokenizer = Start;
        if(ParseJumpStatement(Tokenizer, Child)) return(true);

        *Tokenizer = Start;
        return(false);
}

/*
  typedef-name:
          identifier
*/
bool
ParseTypedefName(struct tokenizer *Tokenizer, parse_tree_node *ParseTree)
{
        struct tokenizer Start = *Tokenizer;
        struct token Token = GetToken(Tokenizer);
        *Tokenizer = Start;

        PARSE_TREE_UPDATE("Typedef Name", 1);

        if(ParseIdentifier(Tokenizer, &ParseTree->Children[0]) && TypedefIsName(Token))
        {
                ParseTreeSet(ParseTree, "Typedef Name", Token);
                return(true);
        }

        *Tokenizer = Start;
        return(false);
}

/*
  direct-abstract-declarator:
          ( abstract-declarator )
          direct-abstract-declarator(opt) [ constant-expression(opt) ]
          direct-abstract-declarator(opt) ( parameter-type-list(opt) )
*/
bool
ParseDirectAbstractDeclaratorI(struct tokenizer *Tokenizer, parse_tree_node *ParseTree)
{
        struct tokenizer Start = *Tokenizer;
        token Tokens[2];

        PARSE_TREE_UPDATE("Direct Abstract Declarator'", 4);

        if(Token_OpenBracket == (Tokens[0] = GetToken(Tokenizer)).Type &&
           ParseConstantExpression(Tokenizer, &ParseTree->Children[1]) &&
           Token_CloseBracket == (Tokens[1] = GetToken(Tokenizer)).Type &&
           ParseDirectAbstractDeclaratorI(Tokenizer, &ParseTree->Children[2]))
        {
                ParseTreeSet(&ParseTree->Children[0], "Symbol", Tokens[0]);
                ParseTreeSet(&ParseTree->Children[2], "Symbol", Tokens[1]);
                return(true);
        }

        PARSE_TREE_CLEAR_CHILDREN();

        *Tokenizer = Start;
        if(Token_OpenBracket == (Tokens[0] = GetToken(Tokenizer)).Type &&
           Token_CloseBracket == (Tokens[1] = GetToken(Tokenizer)).Type &&
           ParseDirectAbstractDeclaratorI(Tokenizer, &ParseTree->Children[2]))
        {
                ParseTreeSet(&ParseTree->Children[0], "Symbol", Tokens[0]);
                ParseTreeSet(&ParseTree->Children[1], "Symbol", Tokens[1]);
                return(true);
        }

        *Tokenizer = Start;
        if(Token_OpenParen == (Tokens[0] = GetToken(Tokenizer)).Type &&
           ParseParameterTypeList(Tokenizer, &ParseTree->Children[1]) &&
           Token_CloseParen == (Tokens[1] = GetToken(Tokenizer)).Type &&
           ParseDirectAbstractDeclaratorI(Tokenizer, &ParseTree->Children[3]))
        {
                ParseTreeSet(&ParseTree->Children[0], "Symbol", Tokens[0]);
                ParseTreeSet(&ParseTree->Children[2], "Symbol", Tokens[1]);
                return(true);
        }

        PARSE_TREE_CLEAR_CHILDREN();

        *Tokenizer = Start;
        if(Token_OpenParen == (Tokens[0] = GetToken(Tokenizer)).Type &&
           Token_CloseParen == (Tokens[1] = GetToken(Tokenizer)).Type &&
           ParseDirectAbstractDeclaratorI(Tokenizer, &ParseTree->Children[2]))
        {
                ParseTreeSet(&ParseTree->Children[0], "Symbol", Tokens[0]);
                ParseTreeSet(&ParseTree->Children[1], "Symbol", Tokens[1]);
                return(true);
        }

        *Tokenizer = Start;
        return(true);
}

/*
  direct-abstract-declarator:
          ( abstract-declarator )
          direct-abstract-declarator(opt) [ constant-expression(opt) ]
          direct-abstract-declarator(opt) ( parameter-type-list(opt) )
*/
bool
ParseDirectAbstractDeclarator(struct tokenizer *Tokenizer, parse_tree_node *ParseTree)
{
        struct tokenizer Start = *Tokenizer;
        token Tokens[2];

        PARSE_TREE_UPDATE("Direct Abstract Declarator", 4);

        if(Token_OpenParen == (Tokens[0] = GetToken(Tokenizer)).Type &&
           ParseAbstractDeclarator(Tokenizer, &ParseTree->Children[1]) &&
           Token_CloseParen == (Tokens[1] = GetToken(Tokenizer)).Type &&
           ParseDirectAbstractDeclaratorI(Tokenizer, &ParseTree->Children[3]))
        {
                ParseTreeSet(&ParseTree->Children[0], "Symbol", Tokens[0]);
                ParseTreeSet(&ParseTree->Children[2], "Symbol", Tokens[1]);
                return(true);
        }

        *Tokenizer = Start;
        if(Token_OpenBracket == (Tokens[0] = GetToken(Tokenizer)).Type &&
           ParseConstantExpression(Tokenizer, &ParseTree->Children[1]) &&
           Token_CloseBracket == (Tokens[1] = GetToken(Tokenizer)).Type &&
           ParseDirectAbstractDeclaratorI(Tokenizer, &ParseTree->Children[3]))
        {
                ParseTreeSet(&ParseTree->Children[0], "Symbol", Tokens[0]);
                ParseTreeSet(&ParseTree->Children[2], "Symbol", Tokens[1]);
                return(true);
        }

        PARSE_TREE_CLEAR_CHILDREN();

        *Tokenizer = Start;
        if(Token_OpenBracket == (Tokens[0] = GetToken(Tokenizer)).Type &&
           Token_CloseBracket == (Tokens[1] = GetToken(Tokenizer)).Type &&
           ParseDirectAbstractDeclaratorI(Tokenizer, &ParseTree->Children[2]))
        {
                ParseTreeSet(&ParseTree->Children[0], "Symbol", Tokens[0]);
                ParseTreeSet(&ParseTree->Children[1], "Symbol", Tokens[1]);
                return(true);
        }

        *Tokenizer = Start;
        if(Token_OpenParen == (Tokens[0] = GetToken(Tokenizer)).Type &&
           ParseParameterTypeList(Tokenizer, &ParseTree->Children[1]) &&
           Token_CloseParen == (Tokens[1] = GetToken(Tokenizer)).Type &&
           ParseDirectAbstractDeclaratorI(Tokenizer, &ParseTree->Children[2]))
        {
                ParseTreeSet(&ParseTree->Children[0], "Symbol", Tokens[0]);
                ParseTreeSet(&ParseTree->Children[2], "Symbol", Tokens[1]);
                return(true);
        }

        *Tokenizer = Start;
        if(Token_OpenParen == (Tokens[0] = GetToken(Tokenizer)).Type &&
           Token_CloseParen == (Tokens[1] = GetToken(Tokenizer)).Type &&
           ParseDirectAbstractDeclaratorI(Tokenizer, &ParseTree->Children[2]))
        {
                ParseTreeSet(&ParseTree->Children[0], "Symbol", Tokens[0]);
                ParseTreeSet(&ParseTree->Children[1], "Symbol", Tokens[1]);
                return(true);
        }

        *Tokenizer = Start;
        return(false);
}

/*
  abstract-declarator:
          pointer
          pointer(opt) direct-abstract-declarator
*/
bool
ParseAbstractDeclarator(struct tokenizer *Tokenizer, parse_tree_node *ParseTree)
{
        struct tokenizer Start = *Tokenizer;

        PARSE_TREE_UPDATE("Abstract Declarator", 2);

        if(ParsePointer(Tokenizer, &ParseTree->Children[0]) &&
           ParseDirectAbstractDeclarator(Tokenizer, &ParseTree->Children[1]))
        {
                return(true);
        }

        PARSE_TREE_CLEAR_CHILDREN();

        *Tokenizer = Start;
        if(ParseDirectAbstractDeclarator(Tokenizer, &ParseTree->Children[0]))
        {
                return(true);
        }

        *Tokenizer = Start;
        if(ParsePointer(Tokenizer, &ParseTree->Children[0]))
        {
                return(true);
        }

        *Tokenizer = Start;
        return(false);
}

/*
  type-name:
          specifier-qualifier-list abstract-declarator(opt)
*/
bool
ParseTypeName(struct tokenizer *Tokenizer, parse_tree_node *ParseTree)
{
        struct tokenizer Start = *Tokenizer;

        PARSE_TREE_UPDATE("Type Name", 2);

        if(ParseSpecifierQualifierList(Tokenizer, &ParseTree->Children[0]) &&
           ParseAbstractDeclarator(Tokenizer, &ParseTree->Children[1]))
        {
                return(true);
        }

        PARSE_TREE_CLEAR_CHILDREN();

        *Tokenizer = Start;
        if(ParseSpecifierQualifierList(Tokenizer, &ParseTree->Children[0]))
        {
                return(true);
        }

        *Tokenizer = Start;
        return(false);
}

bool
ParseInitializerListI(struct tokenizer *Tokenizer, parse_tree_node *ParseTree)
{
        struct tokenizer Start = *Tokenizer;
        token Token;

        PARSE_TREE_UPDATE("Initializer List'", 3);

        if(Token_Comma == (Token = GetToken(Tokenizer)).Type &&
           ParseInitializer(Tokenizer, &ParseTree->Children[1]) &&
           ParseInitializerListI(Tokenizer, &ParseTree->Children[2]))
        {
                ParseTreeSet(&ParseTree->Children[0], "Symbol", Token);
                return(true);
        }

        *Tokenizer = Start;
        return(true);
}

/*
  initializer-list:
          initializer
          initializer-list , initializer
*/
bool
ParseInitializerList(struct tokenizer *Tokenizer, parse_tree_node *ParseTree)
{
        struct tokenizer Start = *Tokenizer;

        PARSE_TREE_UPDATE("Initializer List", 2);

        if(ParseInitializer(Tokenizer, &ParseTree->Children[0]) &&
           ParseInitializerListI(Tokenizer, &ParseTree->Children[1]))
        {
                return(true);
        }

        *Tokenizer = Start;
        return(false);
}

/*
  initializer:
          assignment-expression
          { initializer-list }
          { initializer-list , }
*/
bool
ParseInitializer(struct tokenizer *Tokenizer, parse_tree_node *ParseTree)
{
        struct tokenizer Start = *Tokenizer;
        token Tokens[3];

        PARSE_TREE_UPDATE("Initializer", 4);

        if(ParseAssignmentExpression(Tokenizer, &ParseTree->Children[0]))
        {
                return(true);
        }

        *Tokenizer = Start;
        if(Token_OpenBrace == (Tokens[0] = GetToken(Tokenizer)).Type &&
           ParseInitializerList(Tokenizer, &ParseTree->Children[1]) &&
           Token_CloseBrace == (Tokens[1] = GetToken(Tokenizer)).Type)
        {
                ParseTreeSet(&ParseTree->Children[0], "Symbol", Tokens[0]);
                ParseTreeSet(&ParseTree->Children[2], "Symbol", Tokens[1]);
                return(true);
        }

        *Tokenizer = Start;
        if(Token_OpenBrace == (Tokens[0] = GetToken(Tokenizer)).Type &&
           ParseInitializerList(Tokenizer, &ParseTree->Children[1]) &&
           Token_Comma == (Tokens[1] = GetToken(Tokenizer)).Type &&
           Token_CloseBrace == (Tokens[2] = GetToken(Tokenizer)).Type)
        {
                ParseTreeSet(&ParseTree->Children[0], "Symbol", Tokens[0]);
                ParseTreeSet(&ParseTree->Children[2], "Symbol", Tokens[1]);
                ParseTreeSet(&ParseTree->Children[3], "Symbol", Tokens[2]);
                return(true);
        }

        *Tokenizer = Start;
        return(false);
}

bool
ParseIdentifierListI(struct tokenizer *Tokenizer, parse_tree_node *ParseTree)
{
        struct tokenizer Start = *Tokenizer;
        token Token;

        PARSE_TREE_UPDATE("Identifier List'", 3);

        if(Token_Comma == (Token = GetToken(Tokenizer)).Type &&
           ParseIdentifier(Tokenizer, &ParseTree->Children[1]) &&
           ParseIdentifierListI(Tokenizer, &ParseTree->Children[2]))
        {
                ParseTreeSet(&ParseTree->Children[0], "Symbol", Token);
                return(true);
        }

        *Tokenizer = Start;
        return(true);
}

/*
  identifier-list:
          identifier
          identifier-list , identifier
*/
bool
ParseIdentifierList(struct tokenizer *Tokenizer, parse_tree_node *ParseTree)
{
        struct tokenizer Start = *Tokenizer;

        PARSE_TREE_UPDATE("Identifier List", 2);

        if(ParseIdentifier(Tokenizer, &ParseTree->Children[0]) &&
           ParseIdentifierListI(Tokenizer, &ParseTree->Children[1]))
        {
                return(true);
        }

        *Tokenizer = Start;
        return(false);
}

/*
  parameter-declaration:
          declaration-specifiers declarator
          declaration-specifiers abstract-declarator(opt)
*/
bool
ParseParameterDeclaration(struct tokenizer *Tokenizer, parse_tree_node *ParseTree)
{
        struct tokenizer Start = *Tokenizer;

        PARSE_TREE_UPDATE("Parameter Declaration", 2);

        if(ParseDeclarationSpecifiers(Tokenizer, &ParseTree->Children[0]) &&
           ParseDeclarator(Tokenizer, &ParseTree->Children[1]))
        {
                return(true);
        }

        *Tokenizer = Start;
        if(ParseDeclarationSpecifiers(Tokenizer, &ParseTree->Children[0]) &&
           ParseAbstractDeclarator(Tokenizer, &ParseTree->Children[1]))
        {
                return(true);
        }

        PARSE_TREE_CLEAR_CHILDREN();

        *Tokenizer = Start;
        if(ParseDeclarationSpecifiers(Tokenizer, &ParseTree->Children[0]))
        {
                return(true);
        }

        *Tokenizer = Start;
        return(false);
}

bool
ParseParameterListI(struct tokenizer *Tokenizer, parse_tree_node *ParseTree)
{
        struct tokenizer Start = *Tokenizer;
        token Token;

        PARSE_TREE_UPDATE("Parameter List'", 3);

        if(Token_Comma == (Token = GetToken(Tokenizer)).Type &&
           ParseParameterDeclaration(Tokenizer, &ParseTree->Children[1]) &&
           ParseParameterListI(Tokenizer, &ParseTree->Children[2]))
        {
                ParseTreeSet(&ParseTree->Children[0], "Symbol", Token);
                return(true);
        }

        *Tokenizer = Start;
        return(true);
}

/*
  parameter-list:
          parameter-declaration
          parameter-list , parameter-declaration
*/
bool
ParseParameterList(struct tokenizer *Tokenizer, parse_tree_node *ParseTree)
{
        struct tokenizer Start = *Tokenizer;

        PARSE_TREE_UPDATE("Parameter List", 2);

        if(ParseParameterDeclaration(Tokenizer, &ParseTree->Children[0]) &&
           ParseParameterListI(Tokenizer, &ParseTree->Children[1]))
        {
                return(true);
        }

        *Tokenizer = Start;
        return(false);
}

/*
  parameter-type-list:
          parameter-list
          parameter-list , ...
*/
bool
ParseParameterTypeList(struct tokenizer *Tokenizer, parse_tree_node *ParseTree)
{
        struct tokenizer Start = *Tokenizer;
        token Tokens[2];

        PARSE_TREE_UPDATE("Parameter Type List", 3);

        if(ParseParameterList(Tokenizer, &ParseTree->Children[0]))
        {
                struct tokenizer Previous = *Tokenizer;
                if(Token_Comma == (Tokens[0] = GetToken(Tokenizer)).Type &&
                   Token_Ellipsis == (Tokens[1] = GetToken(Tokenizer)).Type)
                {
                        ParseTreeSet(&ParseTree->Children[1], "Symbol", Tokens[0]);
                        ParseTreeSet(&ParseTree->Children[2], "Symbol", Tokens[1]);
                        return(true);
                }

                *Tokenizer = Previous;
                return(true);
        }

        *Tokenizer = Start;
        return(false);
}

bool
ParseTypeQualifierListI(struct tokenizer *Tokenizer, parse_tree_node *ParseTree)
{
        struct tokenizer Start = *Tokenizer;

        PARSE_TREE_UPDATE("Type Qualifier List'", 2);

        if(ParseTypeQualifier(Tokenizer, &ParseTree->Children[0]) &&
           ParseTypeQualifierListI(Tokenizer, &ParseTree->Children[1]))
        {
                return(true);
        }

        *Tokenizer = Start;
        return(true);
}

/*
  type-qualifier-list:
          type-qualifier
          type-qualifier-list type-qualifier
*/
bool
ParseTypeQualifierList(struct tokenizer *Tokenizer, parse_tree_node *ParseTree)
{
        struct tokenizer Start = *Tokenizer;

        PARSE_TREE_UPDATE("Type Qualifier List", 2);

        if(ParseTypeQualifier(Tokenizer, &ParseTree->Children[0]) &&
           ParseTypeQualifierListI(Tokenizer, &ParseTree->Children[1]))
        {
                return(true);
        }

        *Tokenizer = Start;
        return(false);
}

/*
  pointer:
          * type-qualifier-list(opt)
          * type-qualifier-list(opt) pointer
  */
bool
ParsePointer(struct tokenizer *Tokenizer, parse_tree_node *ParseTree)
{
        struct tokenizer Start = *Tokenizer;
        struct token Token = GetToken(Tokenizer);
        struct tokenizer AtToken = *Tokenizer;

        PARSE_TREE_UPDATE("Pointer", 2);

        if(Token_Asterisk != Token.Type)
        {
                *Tokenizer = Start;
                return(false);
        }

        if(ParseTypeQualifierList(Tokenizer, &ParseTree->Children[0]) &&
           ParsePointer(Tokenizer, &ParseTree->Children[1]))
        {
                return(true);
        }

        PARSE_TREE_CLEAR_CHILDREN();

        *Tokenizer = AtToken;
        if(ParsePointer(Tokenizer, &ParseTree->Children[0]))
        {
                return(true);
        }

        *Tokenizer = AtToken;
        if(ParseTypeQualifierList(Tokenizer, &ParseTree->Children[0]))
        {
                return(true);
        }

        *Tokenizer = AtToken;
        return(true);
}

/*
  direct-declarator:
          identifier
          ( declarator )
          direct-declarator [ constant-expression(opt) ]
          direct-declarator ( parameter-type-list )
          direct-declarator ( identifier-list(opt) )
*/
bool
ParseDirectDeclaratorI(struct tokenizer *Tokenizer, parse_tree_node *ParseTree)
{
        struct tokenizer Start = *Tokenizer;
        token Tokens[2];

        PARSE_TREE_UPDATE("Direct Declarator'", 4);

        if(Token_OpenBracket == (Tokens[0] = GetToken(Tokenizer)).Type &&
           ParseConstantExpression(Tokenizer, &ParseTree->Children[1]) &&
           Token_CloseBracket == (Tokens[1] = GetToken(Tokenizer)).Type &&
           ParseDirectDeclaratorI(Tokenizer, &ParseTree->Children[3]))
        {
                ParseTreeSet(&ParseTree->Children[0], "Symbol", Tokens[0]);
                ParseTreeSet(&ParseTree->Children[2], "Symbol", Tokens[1]);
                return(true);
        }

        PARSE_TREE_CLEAR_CHILDREN();

        *Tokenizer = Start;
        if(Token_OpenBracket == (Tokens[0] = GetToken(Tokenizer)).Type &&
           Token_CloseBracket == (Tokens[1] = GetToken(Tokenizer)).Type &&
           ParseDirectDeclaratorI(Tokenizer, &ParseTree->Children[2]))
        {
                ParseTreeSet(&ParseTree->Children[0], "Symbol", Tokens[0]);
                ParseTreeSet(&ParseTree->Children[1], "Symbol", Tokens[1]);
                return(true);
        }

        *Tokenizer = Start;
        if(Token_OpenParen == (Tokens[0] = GetToken(Tokenizer)).Type &&
           ParseParameterTypeList(Tokenizer, &ParseTree->Children[1]) &&
           Token_CloseParen == (Tokens[1] = GetToken(Tokenizer)).Type &&
           ParseDirectDeclaratorI(Tokenizer, &ParseTree->Children[3]))
        {
                ParseTreeSet(&ParseTree->Children[0], "Symbol", Tokens[0]);
                ParseTreeSet(&ParseTree->Children[2], "Symbol", Tokens[1]);
                return(true);
        }

        *Tokenizer = Start;
        if(Token_OpenParen == (Tokens[0] = GetToken(Tokenizer)).Type &&
           ParseIdentifierList(Tokenizer, &ParseTree->Children[1]) &&
           Token_CloseParen == (Tokens[1] = GetToken(Tokenizer)).Type &&
           ParseDirectDeclaratorI(Tokenizer, &ParseTree->Children[3]))
        {
                ParseTreeSet(&ParseTree->Children[0], "Symbol", Tokens[0]);
                ParseTreeSet(&ParseTree->Children[2], "Symbol", Tokens[1]);
                return(true);
        }

        PARSE_TREE_CLEAR_CHILDREN();

        *Tokenizer = Start;
        if(Token_OpenParen == (Tokens[0] = GetToken(Tokenizer)).Type &&
           Token_CloseParen == (Tokens[1] = GetToken(Tokenizer)).Type &&
           ParseDirectDeclaratorI(Tokenizer, &ParseTree->Children[2]))
        {
                ParseTreeSet(&ParseTree->Children[0], "Symbol", Tokens[0]);
                ParseTreeSet(&ParseTree->Children[1], "Symbol", Tokens[1]);
                return(true);
        }

        *Tokenizer = Start;
        return(true);
}

/*
  direct-declarator:
          identifier
          ( declarator )
          direct-declarator [ constant-expression(opt) ]
          direct-declarator ( parameter-type-list )
          direct-declarator ( identifier-list(opt) )
*/
bool
ParseDirectDeclarator(struct tokenizer *Tokenizer, parse_tree_node *ParseTree)
{
        struct tokenizer Start = *Tokenizer;
        token Tokens[2];

        PARSE_TREE_UPDATE("Direct Declarator", 4);

        if(ParseIdentifier(Tokenizer, &ParseTree->Children[0]) &&
           ParseDirectDeclaratorI(Tokenizer, &ParseTree->Children[1]))
        {
                return(true);
        }

        *Tokenizer = Start;
        if(Token_OpenParen == (Tokens[0] = GetToken(Tokenizer)).Type &&
           ParseDeclarator(Tokenizer, &ParseTree->Children[1]) &&
           Token_CloseParen == (Tokens[1] = GetToken(Tokenizer)).Type &&
           ParseDirectDeclaratorI(Tokenizer, &ParseTree->Children[3]))
        {
                ParseTreeSet(&ParseTree->Children[0], "Symbol", Tokens[0]);
                ParseTreeSet(&ParseTree->Children[2], "Symbol", Tokens[1]);
                return(true);
        }

        *Tokenizer = Start;
        return(false);
}

/*
  declarator:
          pointer(opt) direct-declarator
*/
bool
ParseDeclarator(struct tokenizer *Tokenizer, parse_tree_node *ParseTree)
{
        struct tokenizer Start = *Tokenizer;

        PARSE_TREE_UPDATE("Declarator", 2);

        if(ParsePointer(Tokenizer, &ParseTree->Children[0]) &&
           ParseDirectDeclarator(Tokenizer, &ParseTree->Children[1]))
        {
                return(true);
        }

        PARSE_TREE_CLEAR_CHILDREN();

        *Tokenizer = Start;
        if(ParseDirectDeclarator(Tokenizer, &ParseTree->Children[0]))
        {
                return(true);
        }

        *Tokenizer = Start;
        return(false);
}

/*
  enumerator:
          identifier
          identifier = constant-expression
*/
bool
ParseEnumerator(struct tokenizer *Tokenizer, parse_tree_node *ParseTree)
{
        struct tokenizer Start = *Tokenizer;
        token Token;

        PARSE_TREE_UPDATE("Enumerator", 3);

        if(ParseIdentifier(Tokenizer, &ParseTree->Children[0]) &&
           Token_EqualSign == (Token = GetToken(Tokenizer)).Type &&
           ParseConstantExpression(Tokenizer, &ParseTree->Children[2]))
        {
                ParseTreeSet(&ParseTree->Children[1], "Symbol", Token);
                return(true);
        }

        PARSE_TREE_CLEAR_CHILDREN();

        *Tokenizer = Start;
        if(ParseIdentifier(Tokenizer, &ParseTree->Children[0]))
        {
                return(true);
        }

        *Tokenizer = Start;
        return(false);
}

bool
ParseEnumeratorListI(struct tokenizer *Tokenizer, parse_tree_node *ParseTree)
{
        struct tokenizer Start = *Tokenizer;
        token Token;

        PARSE_TREE_UPDATE("Enumerator List'", 3);

        if(Token_Comma == (Token = GetToken(Tokenizer)).Type &&
           ParseEnumerator(Tokenizer, &ParseTree->Children[1]) &&
           ParseEnumeratorListI(Tokenizer, &ParseTree->Children[2]))
        {
                ParseTreeSet(&ParseTree->Children[0], "Symbol", Token);
                return(true);
        }

        *Tokenizer = Start;
        return(true);
}

/*
  enumerator-list:
          enumerator
          enumerator-list , enumerator
*/
bool
ParseEnumeratorList(struct tokenizer *Tokenizer, parse_tree_node *ParseTree)
{
        struct tokenizer Start = *Tokenizer;

        PARSE_TREE_UPDATE("Enumerator List", 2);

        if(ParseEnumerator(Tokenizer, &ParseTree->Children[0]) &&
           ParseEnumeratorListI(Tokenizer, &ParseTree->Children[1]))
        {
                return(true);
        }

        *Tokenizer = Start;
        return(false);
}

/*
  enum-specifier:
          enum identifier(opt) { enumerator-list }
          enum identifier
*/
bool
ParseEnumSpecifier(struct tokenizer *Tokenizer, parse_tree_node *ParseTree)
{
        struct tokenizer Start = *Tokenizer;
        struct token Token = GetToken(Tokenizer);
        struct tokenizer AtToken = *Tokenizer;
        struct token Tokens[2];

        PARSE_TREE_UPDATE("Enum Specifier", 5);

        if(!(Token_Keyword == Token.Type &&
             GSStringIsEqual("enum", Token.Text, Token.TextLength)))
        {
                *Tokenizer = Start;
                return(false);
        }

        ParseTreeSet(&ParseTree->Children[0], "Keyword", Token);

        if(ParseIdentifier(Tokenizer, &ParseTree->Children[1]) &&
           Token_OpenBrace == (Tokens[0] = GetToken(Tokenizer)).Type &&
           ParseEnumeratorList(Tokenizer, &ParseTree->Children[3]) &&
           Token_CloseBrace == (Tokens[1] = GetToken(Tokenizer)).Type)
        {
                ParseTreeSet(&ParseTree->Children[2], "Symbol", Tokens[0]);
                ParseTreeSet(&ParseTree->Children[4], "Symbol", Tokens[1]);
                return(true);
        }


        PARSE_TREE_CLEAR_CHILDREN();

        *Tokenizer = AtToken;
        if(Token_OpenBrace == (Tokens[0] = GetToken(Tokenizer)).Type &&
           ParseEnumeratorList(Tokenizer, &ParseTree->Children[2]) &&
           Token_CloseBrace == (Tokens[1] = GetToken(Tokenizer)).Type)
        {
                ParseTreeSet(&ParseTree->Children[1], "Symbol", Tokens[0]);
                ParseTreeSet(&ParseTree->Children[3], "Symbol", Tokens[1]);
                return(true);
        }

        PARSE_TREE_CLEAR_CHILDREN();

        *Tokenizer = AtToken;
        if(ParseIdentifier(Tokenizer, &ParseTree->Children[0]))
        {
                return(true);
        }

        *Tokenizer = Start;
        return(false);
}

/*
  struct-declarator:
          declarator
          declarator(opt) : constant-expression
*/
bool
ParseStructDeclarator(struct tokenizer *Tokenizer, parse_tree_node *ParseTree)
{
        struct tokenizer Start = *Tokenizer;
        token Token;

        PARSE_TREE_UPDATE("Struct Declarator", 3);

        if(ParseDeclarator(Tokenizer, &ParseTree->Children[0]) &&
           Token_Colon == (Token = GetToken(Tokenizer)).Type &&
           ParseConstantExpression(Tokenizer, &ParseTree->Children[2]))
        {
                ParseTreeSet(&ParseTree->Children[1], "Symbol", Token);
                return(true);
        }

        PARSE_TREE_CLEAR_CHILDREN();

        *Tokenizer = Start;
        if(Token_Colon == (Token = GetToken(Tokenizer)).Type &&
           ParseConstantExpression(Tokenizer, &ParseTree->Children[1]))
        {
                ParseTreeSet(&ParseTree->Children[0], "Symbol", Token);
                return(true);
        }

        PARSE_TREE_CLEAR_CHILDREN();

        *Tokenizer = Start;
        if(ParseDeclarator(Tokenizer, &ParseTree->Children[0]))
        {
                return(true);
        }

        *Tokenizer = Start;
        return(false);
}

bool
ParseStructDeclaratorListI(struct tokenizer *Tokenizer, parse_tree_node *ParseTree)
{
        struct tokenizer Start = *Tokenizer;
        token Token;

        PARSE_TREE_UPDATE("Struct Declarator List'", 3);

        if(Token_Comma == (Token = GetToken(Tokenizer)).Type &&
           ParseStructDeclarator(Tokenizer, &ParseTree->Children[1]) &&
           ParseStructDeclaratorListI(Tokenizer, &ParseTree->Children[2]))
        {
                ParseTreeSet(&ParseTree->Children[0], "Symbol", Token);
                return(true);
        }

        *Tokenizer = Start;
        return(true);
}

/*
  struct-declarator-list:
          struct-declarator
          struct-declarator-list , struct-declarator
*/
bool
ParseStructDeclaratorList(struct tokenizer *Tokenizer, parse_tree_node *ParseTree)
{
        struct tokenizer Start = *Tokenizer;

        PARSE_TREE_UPDATE("Struct Declarator List", 2);

        if(ParseStructDeclarator(Tokenizer, &ParseTree->Children[0]) &&
           ParseStructDeclaratorListI(Tokenizer, &ParseTree->Children[1]))
        {
                return(true);
        }

        *Tokenizer = Start;
        return(false);
}

/*
  specifier-qualifier-list:
          type-specifier specifier-qualifier-list(opt)
          type-qualifier specifier-qualifier-list(opt)
*/
bool
ParseSpecifierQualifierList(struct tokenizer *Tokenizer, parse_tree_node *ParseTree)
{
        struct tokenizer Start = *Tokenizer;

        PARSE_TREE_UPDATE("Specifier Qualifier List", 2);

        if(ParseTypeSpecifier(Tokenizer, &ParseTree->Children[0]) &&
           ParseSpecifierQualifierList(Tokenizer, &ParseTree->Children[1]))
        {
                return(true);
        }

        PARSE_TREE_CLEAR_CHILDREN();

        *Tokenizer = Start;
        if(ParseTypeSpecifier(Tokenizer, &ParseTree->Children[0]))
        {
                return(true);
        }

        *Tokenizer = Start;
        if(ParseTypeQualifier(Tokenizer, &ParseTree->Children[0]) &&
           ParseSpecifierQualifierList(Tokenizer, &ParseTree->Children[1]))
        {
                return(true);
        }

        PARSE_TREE_CLEAR_CHILDREN();

        *Tokenizer = Start;
        if(ParseTypeQualifier(Tokenizer, &ParseTree->Children[0]))
        {
                return(true);
        }

        *Tokenizer = Start;
        return(false);
}

/*
  struct-declaration:
          specifier-qualifier-list struct-declarator-list ;
*/
bool
ParseStructDeclaration(struct tokenizer *Tokenizer, parse_tree_node *ParseTree)
{
        struct tokenizer Start = *Tokenizer;
        token Token;

        PARSE_TREE_UPDATE("Struct Declaration", 3);

        if(ParseSpecifierQualifierList(Tokenizer, &ParseTree->Children[0]) &&
           ParseStructDeclaratorList(Tokenizer, &ParseTree->Children[1]) &&
           Token_SemiColon == (Token = GetToken(Tokenizer)).Type)
        {
                ParseTreeSet(&ParseTree->Children[2], "Struct Declaration", Token);
                return(true);
        }

        *Tokenizer = Start;
        return(false);
}

/*
  init-declarator:
          declarator
          declarator = initializer
*/
bool
ParseInitDeclarator(struct tokenizer *Tokenizer, parse_tree_node *ParseTree)
{
        struct tokenizer Start = *Tokenizer;
        token Token;

        PARSE_TREE_UPDATE("Init Declarator", 3);

        if(ParseDeclarator(Tokenizer, &ParseTree->Children[0]) &&
           Token_EqualSign == (Token = GetToken(Tokenizer)).Type &&
           ParseInitializer(Tokenizer, &ParseTree->Children[2]))
        {
                ParseTreeSet(&ParseTree->Children[1], "Symbol", Token);
                return(true);
        }

        PARSE_TREE_CLEAR_CHILDREN();

        *Tokenizer = Start;
        if(ParseDeclarator(Tokenizer, &ParseTree->Children[0]))
        {
                return(true);
        }

        *Tokenizer = Start;
        return(false);
}

bool
ParseInitDeclaratorListI(struct tokenizer *Tokenizer, parse_tree_node *ParseTree)
{
        struct tokenizer Start = *Tokenizer;
        token Token;

        PARSE_TREE_UPDATE("Init Declarator List'", 2);

        if(Token_Comma == (Token = GetToken(Tokenizer)).Type &&
           ParseInitDeclarator(Tokenizer, &ParseTree->Children[1]))
        {
                ParseTreeSet(&ParseTree->Children[0], "Symbol", Token);
                return(true);
        }

        *Tokenizer = Start;
        return(true);
}

/*
  init-declarator-list:
          init-declarator
          init-declarator-list , init-declarator
*/
bool
ParseInitDeclaratorList(struct tokenizer *Tokenizer, parse_tree_node *ParseTree)
{
        struct tokenizer Start = *Tokenizer;

        PARSE_TREE_UPDATE("Init Declaration List", 2);

        if(ParseInitDeclarator(Tokenizer, &ParseTree->Children[0]) &&
           ParseInitDeclaratorListI(Tokenizer, &ParseTree->Children[1]))
        {
                return(true);
        }

        *Tokenizer = Start;
        return(false);
}

bool
ParseStructDeclarationListI(struct tokenizer *Tokenizer, parse_tree_node *ParseTree)
{
        struct tokenizer Start = *Tokenizer;

        PARSE_TREE_UPDATE("Struct Declaration List'", 2);

        if(ParseStructDeclaration(Tokenizer, &ParseTree->Children[0]) &&
           ParseStructDeclarationListI(Tokenizer, &ParseTree->Children[1]))
        {
                return(true);
        }

        *Tokenizer = Start;
        return(true);
}

/*
  struct-declaration-list:
          struct-declaration
          struct-declaration-list struct-declaration
*/
bool
ParseStructDeclarationList(struct tokenizer *Tokenizer, parse_tree_node *ParseTree)
{
        struct tokenizer Start = *Tokenizer;

        PARSE_TREE_UPDATE("Struct Declaration List", 2);

        if(ParseStructDeclaration(Tokenizer, &ParseTree->Children[0]) &&
           ParseStructDeclarationListI(Tokenizer, &ParseTree->Children[1]))
        {
                return(true);
        }

        *Tokenizer = Start;
        return(false);
}

/*
  struct-or-union:
  One of: struct union
*/
bool
ParseStructOrUnion(struct tokenizer *Tokenizer, parse_tree_node *ParseTree)
{
        struct tokenizer Start = *Tokenizer;
        struct token Token = GetToken(Tokenizer);

        if(Token_Keyword == Token.Type)
        {
                if(GSStringIsEqual(Token.Text, "struct", Token.TextLength) ||
                   GSStringIsEqual(Token.Text, "union", Token.TextLength))
                {
                        ParseTreeSet(ParseTree, "Struct or Union", Token);
                        return(true);
                }
        }

        *Tokenizer = Start;
        return(false);
}

/*
  struct-or-union-specifier:
          struct-or-union identifier(opt) { struct-declaration-list }
          struct-or-union identifier
*/
bool
ParseStructOrUnionSpecifier(struct tokenizer *Tokenizer, parse_tree_node *ParseTree)
{
        struct tokenizer Start = *Tokenizer;

        PARSE_TREE_UPDATE("Struct or Union Specifier", 5);

        token Tokens[2];

        if(ParseStructOrUnion(Tokenizer, &ParseTree->Children[0]) &&
           ParseIdentifier(Tokenizer, &ParseTree->Children[1]) &&
           Token_OpenBrace == (Tokens[0] = GetToken(Tokenizer)).Type &&
           ParseStructDeclarationList(Tokenizer, &ParseTree->Children[3]) &&
           Token_CloseBrace == (Tokens[1] = GetToken(Tokenizer)).Type)
        {
                ParseTreeSet(&ParseTree->Children[2], "Symbol", Tokens[0]);
                ParseTreeSet(&ParseTree->Children[4], "Symbol", Tokens[1]);
                return(true);
        }

        PARSE_TREE_CLEAR_CHILDREN();

        *Tokenizer = Start;
        if(ParseStructOrUnion(Tokenizer, &ParseTree->Children[0]) &&
           Token_OpenBrace == (Tokens[0] = GetToken(Tokenizer)).Type &&
           ParseStructDeclarationList(Tokenizer, &ParseTree->Children[2]) &&
           Token_CloseBrace == (Tokens[1] = GetToken(Tokenizer)).Type)
        {
                ParseTreeSet(&ParseTree->Children[1], "Symbol", Tokens[0]);
                ParseTreeSet(&ParseTree->Children[3], "Symbol", Tokens[1]);
                return(true);
        }

        PARSE_TREE_CLEAR_CHILDREN();

        *Tokenizer = Start;
        if(ParseStructOrUnion(Tokenizer, &ParseTree->Children[0]) &&
           ParseIdentifier(Tokenizer, &ParseTree->Children[1]))
        {
                return(true);
        }

        *Tokenizer = Start;
        return(false);
}

/*
  type-qualifier:
  One of: const volatile
*/
bool
ParseTypeQualifier(struct tokenizer *Tokenizer, parse_tree_node *ParseTree)
{
        struct tokenizer Start = *Tokenizer;
        struct token Token = GetToken(Tokenizer);

        if(Token.Type == Token_Keyword)
        {
                if(GSStringIsEqual(Token.Text, "const", Token.TextLength) ||
                   GSStringIsEqual(Token.Text, "volatile", Token.TextLength))
                {
                        ParseTreeSet(ParseTree, "Type Qualifier", Token);
                        return(true);
                }
        }

        *Tokenizer = Start;
        return(false);
}

/*
  type-specifier:
  One of: void char short int long float double signed unsigned
  struct-or-union-specifier enum-specifier typedef-name
*/
bool
ParseTypeSpecifier(struct tokenizer *Tokenizer, parse_tree_node *ParseTree)
{
        struct tokenizer Start = *Tokenizer;
        char *Keywords[] = { "void", "char", "short", "int", "long", "float",
                             "double", "signed", "unsigned" };

        struct token Token = GetToken(Tokenizer);
        if(Token.Type == Token_Keyword)
        {
                for(int Index = 0; Index < ArraySize(Keywords); Index++)
                {
                        if(GSStringIsEqual(Token.Text, Keywords[Index], Token.TextLength))
                        {
                                ParseTreeSet(ParseTree, "Type Specifier", Token);
                                return(true);
                        }
                }
        }

        PARSE_TREE_UPDATE("Type Specifier", 1);

        *Tokenizer = Start;
        if(ParseStructOrUnionSpecifier(Tokenizer, &ParseTree->Children[0]))
        {
                return(true);
        }

        *Tokenizer = Start;
        if(ParseEnumSpecifier(Tokenizer, &ParseTree->Children[0]))
        {
                return(true);
        }

        *Tokenizer = Start;
        if(ParseTypedefName(Tokenizer, &ParseTree->Children[0]))
        {
                return(true);
        }

        *Tokenizer = Start;
        return(false);
}

/*
  storage-class-specifier:
  One of: auto register static extern typedef
*/
bool
ParseStorageClassSpecifier(struct tokenizer *Tokenizer, parse_tree_node *ParseTree)
{
        struct tokenizer Start = *Tokenizer;
        char *Keywords[] = { "auto", "register", "static", "extern", "typedef" };

        struct token Token = GetToken(Tokenizer);
        if(Token.Type == Token_Keyword)
        {
                for(int Index = 0; Index < ArraySize(Keywords); Index++)
                {
                        if(GSStringIsEqual(Token.Text, Keywords[Index], Token.TextLength))
                        {
                                ParseTreeSet(ParseTree, "Storage Class Specifier", Token);
                                return(true);
                        }
                }
        }

        *Tokenizer = Start;
        return(false);
}


/*
  declaration-specifiers:
          storage-class-specifier declaration-specifiers(opt)
          type-specifier declaration-specifiers(opt)
          type-qualifier declaration-specifiers(opt)
*/
bool
ParseDeclarationSpecifiers(struct tokenizer *Tokenizer, parse_tree_node *ParseTree)
{
        struct tokenizer Start = *Tokenizer;

        PARSE_TREE_UPDATE("Declaration Specifiers", 2);

        if(ParseStorageClassSpecifier(Tokenizer, &ParseTree->Children[0]) &&
           ParseDeclarationSpecifiers(Tokenizer, &ParseTree->Children[1]))
        {
                return(true);
        }

        *Tokenizer = Start;
        if(ParseTypeSpecifier(Tokenizer, &ParseTree->Children[0]) &&
           ParseDeclarationSpecifiers(Tokenizer, &ParseTree->Children[1]))
        {
                return(true);
        }

        *Tokenizer = Start;
        if(ParseTypeQualifier(Tokenizer, &ParseTree->Children[0]) &&
           ParseDeclarationSpecifiers(Tokenizer, &ParseTree->Children[1]))
        {
                return(true);
        }

        *Tokenizer = Start;
        if(ParseStorageClassSpecifier(Tokenizer, &ParseTree->Children[0]))
        {
                return(true);
        }

        PARSE_TREE_CLEAR_CHILDREN();

        *Tokenizer = Start;
        if(ParseTypeSpecifier(Tokenizer, &ParseTree->Children[0]))
        {
                return(true);
        }

        *Tokenizer = Start;
        if(ParseTypeQualifier(Tokenizer, &ParseTree->Children[0]))
        {
                return(true);
        }

        *Tokenizer = Start;
        return(false);
}

bool
ParseDeclarationListI(struct tokenizer *Tokenizer, parse_tree_node *ParseTree)
{
        struct tokenizer Start = *Tokenizer;

        PARSE_TREE_UPDATE("Declaration List'", 2);

        if(ParseDeclaration(Tokenizer, &ParseTree->Children[0]) &&
           ParseDeclarationListI(Tokenizer, &ParseTree->Children[1]))
        {
                return(true);
        }

        *Tokenizer = Start;
        return(true);
}

/*
  declaration-list:
          declaration
          declaration-list declaration
*/
bool
ParseDeclarationList(struct tokenizer *Tokenizer, parse_tree_node *ParseTree)
{
        struct tokenizer Start = *Tokenizer;

        PARSE_TREE_UPDATE("Declaration List", 2);

        if(ParseDeclaration(Tokenizer, &ParseTree->Children[0]) &&
           ParseDeclarationListI(Tokenizer, &ParseTree->Children[1]))
        {
                return(true);
        }

        *Tokenizer = Start;
        return(false);
}

/*
  declaration:
          declaration-specifiers init-declarator-list(opt) ;
*/
bool
ParseDeclaration(struct tokenizer *Tokenizer, parse_tree_node *ParseTree)
{
        struct tokenizer Start = *Tokenizer;
        struct token Token;

        PARSE_TREE_UPDATE("Declaration", 3);

        if(ParseDeclarationSpecifiers(Tokenizer, &ParseTree->Children[0]) &&
           ParseInitDeclaratorList(Tokenizer, &ParseTree->Children[1]) &&
           Token_SemiColon == (Token = GetToken(Tokenizer)).Type)
        {
                ParseTreeSet(&ParseTree->Children[2], "Symbol", Token);
                return(true);
        }

        *Tokenizer = Start;
        if(ParseDeclarationSpecifiers(Tokenizer, &ParseTree->Children[0]) &&
           Token_SemiColon == (Token = GetToken(Tokenizer)).Type)
        {
                ParseTreeSet(&ParseTree->Children[1], "Symbol", Token);
                return(true);
        }

        *Tokenizer = Start;
        return(false);
}

/*
  function-definition:
          declaration-specifiers(opt) declarator declaration-list(opt) compound-statement
*/
bool
ParseFunctionDefinition(struct tokenizer *Tokenizer, parse_tree_node *ParseTree)
{
        struct tokenizer Start = *Tokenizer;

        PARSE_TREE_UPDATE("Function Definition", 4);

        if(ParseDeclarationSpecifiers(Tokenizer, &ParseTree->Children[0]) &&
           ParseDeclarator(Tokenizer, &ParseTree->Children[1]) &&
           ParseDeclarationList(Tokenizer, &ParseTree->Children[2]) &&
           ParseCompoundStatement(Tokenizer, &ParseTree->Children[3]))
        {
                return(true);
        }

        PARSE_TREE_CLEAR_CHILDREN();

        *Tokenizer = Start;
        if(ParseDeclarationSpecifiers(Tokenizer, &ParseTree->Children[0]) &&
           ParseDeclarator(Tokenizer, &ParseTree->Children[1]) &&
           ParseCompoundStatement(Tokenizer, &ParseTree->Children[2]))
        {
                return(true);
        }

        *Tokenizer = Start;
        if(ParseDeclarator(Tokenizer, &ParseTree->Children[0]) &&
           ParseDeclarationList(Tokenizer, &ParseTree->Children[1]) &&
           ParseCompoundStatement(Tokenizer, &ParseTree->Children[2]))
        {
                return(true);
        }

        PARSE_TREE_CLEAR_CHILDREN();

        *Tokenizer = Start;
        if(ParseDeclarator(Tokenizer, &ParseTree->Children[0]) &&
           ParseCompoundStatement(Tokenizer, &ParseTree->Children[1]))
        {
                return(true);
        }

        *Tokenizer = Start;
        return(false);
}

/*
  external-declaration:
          function-definition
          declaration
*/
bool
ParseExternalDeclaration(struct tokenizer *Tokenizer, parse_tree_node *ParseTree)
{
        struct tokenizer Start = *Tokenizer;

        PARSE_TREE_UPDATE("External Declaration", 1);

        if(ParseFunctionDefinition(Tokenizer, &ParseTree->Children[0])) return(true);

        *Tokenizer = Start;
        if(ParseDeclaration(Tokenizer, &ParseTree->Children[0])) return(true);

        *Tokenizer = Start;
        return(false);
}

bool
ParseTranslationUnitI(struct tokenizer *Tokenizer, parse_tree_node *ParseTree)
{
        struct tokenizer Start = *Tokenizer;

        PARSE_TREE_UPDATE("Translation Unit'", 2);

        if(ParseExternalDeclaration(Tokenizer, &ParseTree->Children[0]) &&
           ParseTranslationUnitI(Tokenizer, &ParseTree->Children[1]))
        {
                return(true);
        }

        *Tokenizer = Start;
        return(true);
}

/*
  translation-unit:
          external-declaration
          translation-unit external-declaration
*/
bool
ParseTranslationUnit(struct tokenizer *Tokenizer, parse_tree_node *ParseTree)
{
        struct tokenizer Start = *Tokenizer;

        PARSE_TREE_UPDATE("Translation Unit", 2);

        if(ParseExternalDeclaration(Tokenizer, &ParseTree->Children[0]) &&
           ParseTranslationUnitI(Tokenizer, &ParseTree->Children[1]))
        {
                return(true);
        }

        *Tokenizer = Start;
        return(false);
}

void
Parse(gs_buffer *FileContents)
{
        parse_tree_node *ParseTree = ParseTreeNew();

        struct tokenizer Tokenizer;
        Tokenizer.Beginning = Tokenizer.At = FileContents->Start;
        Tokenizer.Line = Tokenizer.Column = 1;

        TypedefInit(TypedefNames);

        bool Parsing = true;
        while(Parsing)
        {
                struct tokenizer Start = Tokenizer;
                struct token Token = GetToken(&Tokenizer);
                struct tokenizer AfterToken = Tokenizer;
                Tokenizer = Start;

                switch(Token.Type)
                {
                        case Token_EndOfStream:
                        {
                                Parsing = false;
                        } break;

                        case Token_PreprocessorCommand:
                        case Token_Comment:
                        case Token_Unknown:
                        {
                                Tokenizer = AfterToken;
                        } break;

                        default:
                        {
                                bool Result = ParseTranslationUnit(&Tokenizer, ParseTree);

                                if(Result && Tokenizer.At == FileContents->Start + FileContents->Length - 1)
                                        puts("Successfully parsed input");
                                else
                                {
                                        puts("Input did not parse");
                                        if(!Result)
                                                puts("Parsing failed");
                                        else if(Tokenizer.At != FileContents->Start + FileContents->Length - 1)
                                        {
                                                puts("Parsing terminated early");
                                                printf("Tokenizer->At(%p), File End(%p)\n", Tokenizer.At, FileContents->Start + FileContents->Length - 1);
                                        }
                                }

                                puts("--------------------------------------------------------------------------------");
                                ParseTreePrint(ParseTree, 0, 2);
                                Parsing = false;
                        } break;
                }
        }
}

#endif /* _PARSER_C */
