#ifndef _PARSER_C
#define _PARSER_C

#include <stdio.h>
#include "bool.c"
#include "lexer.c"

/* NOTE(AARON): Debug include - tree.c */
#include "tree.c"

#define PARSE_TREE_UPDATE(Name, NumChildren) \
        char *NodeName = (Name); \
        ParseTreeSet(ParseTree, (Name)); \
        if(NumChildren > 0) \
        { \
                ParseTreeNewChildren(ParseTree, (NumChildren)); \
        }

#define PARSE_TREE_CLEAR_CHILDREN() \
        for(int i=0; i<ParseTree->NumChildren; i++) \
        { \
                ParseTreeSet(ParseTree->Children[i], "Empty");  \
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

        if(Token_Comma == GetToken(Tokenizer).Type &&
           ParseAssignmentExpression(Tokenizer) &&
           ParseArgumentExpressionListI(Tokenizer))
        {
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
        if(ParseAssignmentExpression(Tokenizer) &&
           ParseArgumentExpressionListI(Tokenizer))
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

        if(Token_Identifier == GetToken(Tokenizer).Type) return(true);

        *Tokenizer = Start;
        if(ParseConstant(Tokenizer)) return(true);

        *Tokenizer = Start;
        if(Token_String == GetToken(Tokenizer).Type) return(true);

        *Tokenizer = Start;
        if(Token_OpenParen == GetToken(Tokenizer).Type &&
           ParseExpression(Tokenizer) &&
           Token_CloseParen == GetToken(Tokenizer).Type)
        {
                return true;
        }

        *Tokenizer = Start;
        return(false);
}

bool
ParsePostfixExpressionI(struct tokenizer *Tokenizer, parse_tree_node *ParseTree)
{
        struct tokenizer Start = *Tokenizer;

        if(Token_OpenBracket == GetToken(Tokenizer).Type &&
           ParseExpression(Tokenizer) &&
           Token_CloseBracket == GetToken(Tokenizer).Type &&
           ParsePostfixExpressionI(Tokenizer))
        {
                return(true);
        }

        *Tokenizer = Start;
        if(Token_OpenParen == GetToken(Tokenizer).Type &&
           ParseArgumentExpressionList(Tokenizer) &&
           Token_CloseParen == GetToken(Tokenizer).Type &&
           ParsePostfixExpressionI(Tokenizer))
        {
                return(true);
        }

        *Tokenizer = Start;
        if(Token_OpenParen == GetToken(Tokenizer).Type &&
           Token_CloseParen == GetToken(Tokenizer).Type &&
           ParsePostfixExpressionI(Tokenizer))
        {
                return(true);
        }

        *Tokenizer = Start;
        if(Token_Dot == GetToken(Tokenizer).Type &&
           Token_Identifier == GetToken(Tokenizer).Type &&
           ParsePostfixExpressionI(Tokenizer))
        {
                return(true);
        }

        *Tokenizer = Start;
        if(Token_Arrow == GetToken(Tokenizer).Type &&
           Token_Identifier == GetToken(Tokenizer).Type &&
           ParsePostfixExpressionI(Tokenizer))
        {
                return(true);
        }

        *Tokenizer = Start;
        if(Token_PlusPlus == GetToken(Tokenizer).Type &&
           ParsePostfixExpressionI(Tokenizer))
        {
                return(true);
        }

        *Tokenizer = Start;
        if(Token_MinusMinus == GetToken(Tokenizer).Type &&
           ParsePostfixExpressionI(Tokenizer))
        {
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

        if(ParsePrimaryExpression(Tokenizer) &&
           ParsePostfixExpressionI(Tokenizer))
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
        struct token Token;

        if(ParsePostfixExpression(Tokenizer))
        {
                return(true);
        }

        *Tokenizer = Start;
        if(Token_PlusPlus == GetToken(Tokenizer).Type &&
           ParseUnaryExpression(Tokenizer))
        {
                return(true);
        }

        *Tokenizer = Start;
        if(Token_MinusMinus == GetToken(Tokenizer).Type &&
           ParseUnaryExpression(Tokenizer))
        {
                return(true);
        }

        *Tokenizer = Start;
        if(ParseUnaryOperator(Tokenizer) &&
           ParseCastExpression(Tokenizer))
        {
                return(true);
        }

        *Tokenizer = Start;
        Token = GetToken(Tokenizer);
        if(Token_Keyword == Token.Type &&
           GSStringIsEqual("sizeof", Token.Text, GSStringLength("sizeof")))
        {
                struct tokenizer Previous = *Tokenizer;
                if(ParseUnaryExpression(Tokenizer))
                {
                        return(true);
                }

                *Tokenizer = Previous;
                if(Token_OpenParen == GetToken(Tokenizer).Type &&
                   ParseTypeName(Tokenizer) &&
                   Token_CloseParen == GetToken(Tokenizer).Type)
                {
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

        if(ParseUnaryExpression(Tokenizer))
        {
                return(true);
        }

        *Tokenizer = Start;
        if(Token_OpenParen == GetToken(Tokenizer).Type &&
           ParseTypeName(Tokenizer) &&
           Token_CloseParen == GetToken(Tokenizer).Type &&
           ParseCastExpression(Tokenizer))
        {
                return(true);
        }

        *Tokenizer = Start;
        return(false);
}

bool
ParseMultiplicativeExpressionI(struct tokenizer *Tokenizer, parse_tree_node *ParseTree)
{
        struct tokenizer Start = *Tokenizer;

        if(Token_Asterisk == GetToken(Tokenizer).Type &&
           ParseCastExpression(Tokenizer) &&
           ParseMultiplicativeExpressionI(Tokenizer))
        {
                return(true);
        }

        *Tokenizer = Start;
        if(Token_Slash == GetToken(Tokenizer).Type &&
           ParseCastExpression(Tokenizer) &&
           ParseMultiplicativeExpressionI(Tokenizer))
        {
                return(true);
        }

        *Tokenizer = Start;
        if(Token_PercentSign == GetToken(Tokenizer).Type &&
           ParseCastExpression(Tokenizer) &&
           ParseMultiplicativeExpressionI(Tokenizer))
        {
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

        if(ParseCastExpression(Tokenizer) &&
           ParseMultiplicativeExpressionI(Tokenizer))
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

        if(Token_Cross == GetToken(Tokenizer).Type &&
           ParseMultiplicativeExpression(Tokenizer) &&
           ParseAdditiveExpressionI(Tokenizer))
        {
                return(true);
        }

        *Tokenizer = Start;
        if(Token_Dash == GetToken(Tokenizer).Type &&
           ParseMultiplicativeExpression(Tokenizer) &&
           ParseAdditiveExpressionI(Tokenizer))
        {
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

        if(ParseMultiplicativeExpression(Tokenizer) &&
           ParseAdditiveExpressionI(Tokenizer))
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

        if(Token_BitShiftLeft == GetToken(Tokenizer).Type &&
           ParseAdditiveExpression(Tokenizer) &&
           ParseShiftExpressionI(Tokenizer))
        {
                return(true);
        }

        *Tokenizer = Start;
        if(Token_BitShiftRight == GetToken(Tokenizer).Type &&
           ParseAdditiveExpression(Tokenizer) &&
           ParseShiftExpressionI(Tokenizer))
        {
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

        if(ParseAdditiveExpression(Tokenizer) &&
           ParseShiftExpressionI(Tokenizer))
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

        if(Token_LessThan == GetToken(Tokenizer).Type &&
           ParseShiftExpression(Tokenizer) &&
           ParseRelationalExpressionI(Tokenizer))
        {
                return(true);
        }

        *Tokenizer = Start;
        if(Token_GreaterThan == GetToken(Tokenizer).Type &&
           ParseShiftExpression(Tokenizer) &&
           ParseRelationalExpressionI(Tokenizer))
        {
                return(true);
        }

        *Tokenizer = Start;
        if(Token_LessThanEqual == GetToken(Tokenizer).Type &&
           ParseShiftExpression(Tokenizer) &&
           ParseRelationalExpressionI(Tokenizer))
        {
                return(true);
        }

        *Tokenizer = Start;
        if(Token_GreaterThanEqual == GetToken(Tokenizer).Type &&
           ParseShiftExpression(Tokenizer) &&
           ParseRelationalExpressionI(Tokenizer))
        {
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

        if(ParseShiftExpression(Tokenizer) &&
           ParseRelationalExpressionI(Tokenizer))
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

        if(Token_LogicalEqual == GetToken(Tokenizer).Type &&
           ParseRelationalExpression(Tokenizer) &&
           ParseEqualityExpressionI(Tokenizer))
        {
                return(true);
        }

        *Tokenizer = Start;
        if(Token_NotEqual == GetToken(Tokenizer).Type &&
           ParseRelationalExpression(Tokenizer) &&
           ParseEqualityExpressionI(Tokenizer))
        {
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

        if(ParseRelationalExpression(Tokenizer) &&
           ParseEqualityExpressionI(Tokenizer))
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

        if(Token_Ampersand == GetToken(Tokenizer).Type &&
           ParseEqualityExpression(Tokenizer) &&
           ParseAndExpressionI(Tokenizer))
        {
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

        if(ParseEqualityExpression(Tokenizer) &&
           ParseAndExpressionI(Tokenizer))
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

        if(Token_Carat == GetToken(Tokenizer).Type &&
           ParseAndExpression(Tokenizer) &&
           ParseExclusiveOrExpressionI(Tokenizer))
        {
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

        if(ParseAndExpression(Tokenizer) &&
           ParseExclusiveOrExpressionI(Tokenizer))
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

        if(Token_Pipe == GetToken(Tokenizer).Type &&
           ParseExclusiveOrExpression(Tokenizer) &&
           ParseInclusiveOrExpressionI(Tokenizer))
        {
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

        if(ParseExclusiveOrExpression(Tokenizer) &&
           ParseInclusiveOrExpressionI(Tokenizer))
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

        if(Token_LogicalAnd == GetToken(Tokenizer).Type &&
           ParseInclusiveOrExpression(Tokenizer) &&
           ParseLogicalAndExpressionI(Tokenizer))
        {
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

        if(ParseInclusiveOrExpression(Tokenizer) &&
           ParseLogicalAndExpressionI(Tokenizer))
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

        if(Token_LogicalOr == GetToken(Tokenizer).Type &&
           ParseLogicalAndExpression(Tokenizer) &&
           ParseLogicalOrExpressionI(Tokenizer))
        {
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

        if(ParseLogicalAndExpression(Tokenizer) &&
           ParseLogicalOrExpressionI(Tokenizer))
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
        if(ParseConditionalExpression(Tokenizer))
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

        if(ParseLogicalOrExpression(Tokenizer) &&
           Token_QuestionMark == GetToken(Tokenizer).Type &&
           ParseExpression(Tokenizer) &&
           Token_Colon == GetToken(Tokenizer).Type &&
           ParseConditionalExpression(Tokenizer))
        {
                return(true);
        }

        *Tokenizer = Start;
        if(ParseLogicalOrExpression(Tokenizer))
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

        if(ParseUnaryExpression(Tokenizer) &&
           ParseAssignmentOperator(Tokenizer) &&
           ParseAssignmentExpression(Tokenizer))
        {
                return(true);
        }

        *Tokenizer = Start;
        if(ParseConditionalExpression(Tokenizer))
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

        if(Token_Comma == GetToken(Tokenizer).Type &&
           ParseAssignmentExpression(Tokenizer) &&
           ParseExpressionI(Tokenizer))
        {
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

        if(ParseAssignmentExpression(Tokenizer) &&
           ParseExpressionI(Tokenizer))
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

        if(Token_Identifier == GetToken(Tokenizer).Type)
        {
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
        struct token Token = GetToken(Tokenizer);
        struct tokenizer AtToken = *Tokenizer;

        if(Token_Keyword == Token.Type &&
           GSStringIsEqual("goto", Token.Text, Token.TextLength) &&
           ParseIdentifier(Tokenizer) &&
           Token_SemiColon == GetToken(Tokenizer).Type)
        {
                return(true);
        }

        *Tokenizer = AtToken;
        if(Token_Keyword == Token.Type &&
           GSStringIsEqual("continue", Token.Text, Token.TextLength) &&
           Token_SemiColon == GetToken(Tokenizer).Type)
        {
                return(true);
        }

        *Tokenizer = AtToken;
        if(Token_Keyword == Token.Type &&
           GSStringIsEqual("break", Token.Text, Token.TextLength) &&
           Token_SemiColon == GetToken(Tokenizer).Type)
        {
                return(true);
        }

        *Tokenizer = AtToken;
        if(Token_Keyword == Token.Type &&
           GSStringIsEqual("return", Token.Text, Token.TextLength))
        {
                struct tokenizer Previous = *Tokenizer;
                if(!ParseExpression(Tokenizer))
                        *Tokenizer = Previous;

                if(Token_SemiColon == GetToken(Tokenizer).Type)
                {
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
        struct token Token = GetToken(Tokenizer);
        struct tokenizer AtToken = *Tokenizer;

        PARSE_TREE_UPDATE("Iteration Statement", FOO);

        if(Token_Keyword == Token.Type &&
           GSStringIsEqual("while", Token.Text, Token.TextLength) &&
           Token_OpenParen == GetToken(Tokenizer).Type &&
           ParseExpression(Tokenizer) &&
           Token_CloseParen == GetToken(Tokenizer).Type &&
           ParseStatement(Tokenizer))
        {
                return(true);
        }

        *Tokenizer = AtToken;
        if(Token_Keyword == Token.Type &&
           GSStringIsEqual("do", Token.Text, Token.TextLength) &&
           ParseStatement(Tokenizer))
        {
                struct token Token = GetToken(Tokenizer);
                if(Token_Keyword == Token.Type &&
                   GSStringIsEqual("while", Token.Text, Token.TextLength) &&
                   Token_OpenParen == GetToken(Tokenizer).Type &&
                   ParseExpression(Tokenizer) &&
                   Token_CloseParen == GetToken(Tokenizer).Type &&
                   Token_SemiColon == GetToken(Tokenizer).Type)
                {
                        return(true);
                }
        }

        *Tokenizer = AtToken;
        if(Token_Keyword == Token.Type &&
           GSStringIsEqual("for", Token.Text, Token.TextLength) &&
           Token_OpenParen == GetToken(Tokenizer).Type)
        {
                struct tokenizer Previous = *Tokenizer;
                if(!ParseExpression(Tokenizer))
                        *Tokenizer = Previous;

                if(Token_SemiColon != GetToken(Tokenizer).Type)
                {
                        *Tokenizer = Start;
                        return(false);
                }

                Previous = *Tokenizer;
                if(!ParseExpression(Tokenizer))
                        *Tokenizer = Previous;

                if(Token_SemiColon != GetToken(Tokenizer).Type)
                {
                        *Tokenizer = Start;
                        return(false);
                }

                Previous = *Tokenizer;
                if(!ParseExpression(Tokenizer))
                        *Tokenizer = Previous;

                if(Token_CloseParen == GetToken(Tokenizer).Type &&
                   ParseStatement(Tokenizer))
                {
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
        struct token Token = GetToken(Tokenizer);
        struct tokenizer AtToken = *Tokenizer;

        PARSE_TREE_UPDATE("Selection Statement", 6);

        if(Token_Keyword == Token.Type &&
           GSStringIsEqual("if", Token.Text, Token.TextLength) &&
           Token_OpenParen == GetToken(Tokenizer).Type &&
           ParseExpression(Tokenizer, ParseTree->Children[2]) &&
           Token_CloseParen == GetToken(Tokenizer).Type &&
           ParseStatement(Tokenizer, ParseTree->Children[4]))
        {
                struct tokenizer AtElse = *Tokenizer;
                struct token Token = GetToken(Tokenizer);

                ParseTreeSet(ParseTree->Children[0], "keyword: if");
                ParseTreeSet(ParseTree->Children[1], "symbol: (");
                ParseTreeSet(ParseTree->Children[3], "symbol: )");

                if(Token_Keyword == Token.Type &&
                   GSStringIsEqual("else", Token.Text, Token.TextLength) &&
                   ParseStatement(Tokenizer, ParseTree->Children[5]))
                {
                        ParseTreeSet(ParseTree->Children[4], "keyword: else");
                        return(true);
                }

                *Tokenizer = AtElse;
                return(true);
        }

        PARSE_TREE_CLEAR_CHILDREN();

        *Tokenizer = AtToken;
        if(Token_Keyword == Token.Type &&
           GSStringIsEqual("switch", Token.Text, Token.TextLength) &&
           Token_OpenParen == GetToken(Tokenizer).Type &&
           ParseExpression(Tokenizer, ParseTree->Children[2]) &&
           Token_CloseParen == GetToken(Tokenizer).Type &&
           ParseStatement(Tokenizer, ParseTree->Children[4]))
        {
                ParseTreeSet(ParseTree->Children[0], "keyword: switch");
                ParseTreeSet(ParseTree->Children[1], "symbol: (");
                ParseTreeSet(ParseTree->Children[3], "symbol: )");
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

        if(ParseStatement(Tokenizer, ParseTree->Children[0]) &&
           ParseStatementListI(Tokenizer, ParseTree->Children[1]))
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

        if(ParseStatement(Tokenizer, ParseTree->Children[0]) &&
           ParseStatementListI(Tokenizer, ParseTree->Children[1]))
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

        PARSE_TREE_UPDATE("Compound Statement", 3);

        if(Token_OpenBrace == GetToken(Tokenizer).Type)
        {
                ParseTreeSet(ParseTree->Children[0], "symbol: {");
                int ChildIndex = 0;

                struct tokenizer Previous = *Tokenizer;
                if(!ParseDeclarationList(Tokenizer, ParseTree->Children[ChildIndex++]))
                {
                        --ChildIndex;
                        *Tokenizer = Previous;
                }

                Previous = *Tokenizer;
                if(!ParseStatementList(Tokenizer, ParseTree->Children[ChildIndex++]))
                {
                        --ChildIndex;
                        *Tokenizer = Previous;
                }

                if(Token_CloseBrace == GetToken(Tokenizer).Type)
                {
                        ParseTreeSet(ParseTree->Children[ChildIndex], "symbol: }");
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

        PARSE_TREE_UPDATE("Expression Statement", 2);

        if(ParseExpression(Tokenizer, ParseTree->Children[0]) &&
           Token_SemiColon == GetToken(Tokenizer).Type)
        {
                ParseTreeSet(ParseTree->Children[1], "symbol: ;");
                return(true);
        }

        PARSE_TREE_CLEAR_CHILDREN();

        *Tokenizer = Start;
        if(Token_SemiColon == GetToken(Tokenizer).Type)
        {
                ParseTreeSet(ParseTree->Children[0], "symbol: ;");
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
        PARSE_TREE_UPDATE("Labeled Statement", 4);

        if(ParseIdentifier(Tokenizer, ParseTree->Children[0]) &&
           Token_Colon == GetToken(Tokenizer).Type &&
           ParseStatement(Tokenizer, ParseTree->Children[2]))
        {
                ParseTreeSet(ParseTree->Children[1], "symbol: :");
                return(true);
        }

        *Tokenizer = Start;
        struct token Token = GetToken(Tokenizer);
        struct tokenizer AtToken = *Tokenizer;

        if(Token_Keyword == Token.Type &&
           GSStringIsEqual("case", Token.Text, Token.TextLength) &&
           ParseConstantExpression(Tokenizer, ParseTree->Children[1]) &&
           Token_Colon == GetToken(Tokenizer).Type &&
           ParseStatement(Tokenizer, ParseTree->Children[3]))
        {
                ParseTreeSet(ParseTree->Children[0], "keyword: case");
                ParseTreeSet(ParseTree->Children[2], "symbol: :");
                return(true);
        }

        PARSE_TREE_CLEAR_CHILDREN();

        *Tokenizer = AtToken;
        if(Token_Keyword == Token.Type &&
           GSStringIsEqual("default", Token.Text, Token.TextLength) &&
           Token_Colon == GetToken(Tokenizer).Type &&
           ParseStatement(Tokenizer, ParseTree->Children[2]))
        {
                ParseTreeSet(ParseTree->Children[0], "keyword: default");
                ParseTreeSet(ParseTree->Children[2], "symbol: :");
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
        parse_tree_node *Child = ParseTree->Children[0];

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

        if(ParseIdentifier(Tokenizer, ParseTree->Children[0]) && TypedefIsName(Token))
        {
                ParseTreeSet(ParseTree, "Typedef Name");
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

        PARSE_TREE_UPDATE("Direct Abstract Declarator'", 4);

        if(Token_OpenBracket == GetToken(Tokenizer).Type &&
           ParseConstantExpression(Tokenizer, ParseTree->Children[1]) &&
           Token_CloseBracket == GetToken(Tokenizer).Type &&
           ParseDirectAbstractDeclaratorI(Tokenizer, ParseTree->Children[2]))
        {
                ParseTreeSet(ParseTree->Children[0], "symbol: [");
                ParseTreeSet(ParseTree->Children[2], "symbol: ]");
                return(true);
        }

        PARSE_TREE_CLEAR_CHILDREN();

        *Tokenizer = Start;
        if(Token_OpenBracket == GetToken(Tokenizer).Type &&
           Token_CloseBracket == GetToken(Tokenizer).Type &&
           ParseDirectAbstractDeclaratorI(Tokenizer, ParseTree->Children[2]))
        {
                ParseTreeSet(ParseTree->Children[0], "symbol: [");
                ParseTreeSet(ParseTree->Children[1], "symbol: ]");
                return(true);
        }

        *Tokenizer = Start;
        if(Token_OpenParen == GetToken(Tokenizer).Type &&
           ParseParameterTypeList(Tokenizer, ParseTree->Children[1]) &&
           Token_CloseParen == GetToken(Tokenizer).Type &&
           ParseDirectAbstractDeclaratorI(Tokenizer, ParseTree->Children[3]))
        {
                ParseTreeSet(ParseTree->Children[0], "symbol: (");
                ParseTreeSet(ParseTree->Children[2], "symbol: )");
                return(true);
        }

        PARSE_TREE_CLEAR_CHILDREN();

        *Tokenizer = Start;
        if(Token_OpenParen == GetToken(Tokenizer).Type &&
           Token_CloseParen == GetToken(Tokenizer).Type &&
           ParseDirectAbstractDeclaratorI(Tokenizer, ParseTree->Children[2]))
        {
                ParseTreeSet(ParseTree->Children[0], "symbol: (");
                ParseTreeSet(ParseTree->Children[1], "symbol: )");
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

        PARSE_TREE_UPDATE("Direct Abstract Declarator", 4);

        if(Token_OpenParen == GetToken(Tokenizer).Type &&
           ParseAbstractDeclarator(Tokenizer, ParseTree->Children[1]) &&
           Token_CloseParen == GetToken(Tokenizer).Type &&
           ParseDirectAbstractDeclaratorI(Tokenizer, ParseTree->Children[3]))
        {
                ParseTreeSet(ParseTree->Children[0], "symbol: (");
                ParseTreeSet(ParseTree->Children[2], "symbol: )");
                return(true);
        }

        *Tokenizer = Start;
        if(Token_OpenBracket == GetToken(Tokenizer).Type &&
           ParseConstantExpression(Tokenizer, ParseTree->Children[1]) &&
           Token_CloseBracket == GetToken(Tokenizer).Type &&
           ParseDirectAbstractDeclaratorI(Tokenizer, ParseTree->Children[3]))
        {
                ParseTreeSet(ParseTree->Children[0], "symbol: [");
                ParseTreeSet(ParseTree->Children[2], "symbol: ]");
                return(true);
        }

        PARSE_TREE_CLEAR_CHILDREN();

        *Tokenizer = Start;
        if(Token_OpenBracket == GetToken(Tokenizer).Type &&
           Token_CloseBracket == GetToken(Tokenizer).Type &&
           ParseDirectAbstractDeclaratorI(Tokenizer, ParseTree->Children[2]))
        {
                ParseTreeSet(ParseTree->Children[0], "symbol: [");
                ParseTreeSet(ParseTree->Children[1], "symbol: ]");
                return(true);
        }

        *Tokenizer = Start;
        if(Token_OpenParen == GetToken(Tokenizer).Type &&
           ParseParameterTypeList(Tokenizer, ParseTree->Children[1]) &&
           Token_CloseParen == GetToken(Tokenizer).Type &&
           ParseDirectAbstractDeclaratorI(Tokenizer, ParseTree->Children[2]))
        {
                ParseTreeSet(ParseTree->Children[0], "symbol: (");
                ParseTreeSet(ParseTree->Children[2], "symbol: )");
                return(true);
        }

        *Tokenizer = Start;
        if(Token_OpenParen == GetToken(Tokenizer).Type &&
           Token_CloseParen == GetToken(Tokenizer).Type &&
           ParseDirectAbstractDeclaratorI(Tokenizer, ParseTree->Children[2]))
        {
                ParseTreeSet(ParseTree->Children[0], "symbol: (");
                ParseTreeSet(ParseTree->Children[1], "symbol: )");
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

        if(ParsePointer(Tokenizer, ParseTree->Children[0]) &&
           ParseDirectAbstractDeclarator(Tokenizer, ParseTree->Children[1]))
        {
                return(true);
        }

        PARSE_TREE_CLEAR_CHILDREN();

        *Tokenizer = Start;
        if(ParseDirectAbstractDeclarator(Tokenizer, ParseTree->Children[0]))
        {
                return(true);
        }

        *Tokenizer = Start;
        if(ParsePointer(Tokenizer, ParseTree->Children[0]))
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

        if(ParseSpecifierQualifierList(Tokenizer, ParseTree->Children[0]) &&
           ParseAbstractDeclarator(Tokenizer, ParseTree->Children[1]))
        {
                return(true);
        }

        PARSE_TREE_CLEAR_CHILDREN();

        *Tokenizer = Start;
        if(ParseSpecifierQualifierList(Tokenizer, ParseTree->Children[0]))
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

        PARSE_TREE_UPDATE("Initializer List'", 3);

        if(Token_Comma == GetToken(Tokenizer).Type &&
           ParseInitializer(Tokenizer, ParseTree->Children[1]) &&
           ParseInitializerListI(Tokenizer, ParseTree->Children[2]))
        {
                ParseTreeSet(ParseTree->Children[0], "symbol: ,");
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

        if(ParseInitializer(Tokenizer, ParseTree->Children[0]) &&
           ParseInitializerListI(Tokenizer, ParseTree->Children[1]))
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

        PARSE_TREE_UPDATE("Initializer", 4);

        if(ParseAssignmentExpression(Tokenizer, ParseTree->Children[0]))
        {
                return(true);
        }

        *Tokenizer = Start;
        if(Token_OpenBrace == GetToken(Tokenizer).Type &&
           ParseInitializerList(Tokenizer, ParseTree->Children[1]) &&
           Token_CloseBrace == GetToken(Tokenizer).Type)
        {
                ParseTreeSet(ParseTree->Children[0], "symbol: {");
                ParseTreeSet(ParseTree->Children[2], "symbol: }");
                return(true);
        }

        *Tokenizer = Start;
        if(Token_OpenBrace == GetToken(Tokenizer).Type &&
           ParseInitializerList(Tokenizer, ParseTree->Children[1]) &&
           Token_Comma == GetToken(Tokenizer).Type &&
           Token_CloseBrace == GetToken(Tokenizer).Type)
        {
                ParseTreeSet(ParseTree->Children[0], "symbol: {");
                ParseTreeSet(ParseTree->Children[2], "symbol: ,");
                ParseTreeSet(ParseTree->Children[3], "symbol: }");
                return(true);
        }

        *Tokenizer = Start;
        return(false);
}

bool
ParseIdentifierListI(struct tokenizer *Tokenizer, parse_tree_node *ParseTree)
{
        struct tokenizer Start = *Tokenizer;

        PARSE_TREE_UPDATE("Identifier List'", 3);

        if(Token_Comma == GetToken(Tokenizer).Type &&
           ParseIdentifier(Tokenizer, ParseTree->Children[1]) &&
           ParseIdentifierListI(Tokenizer, ParseTree->Children[2]))
        {
                ParseTreeSet(ParseTree->Children[0], "symbol: ,");
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

        if(ParseIdentifier(Tokenizer, ParseTree->Children[0]) &&
           ParseIdentifierListI(Tokenizer, ParseTree->Children[1]))
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

        if(ParseDeclarationSpecifiers(Tokenizer, ParseTree->Children[0]) &&
           ParseDeclarator(Tokenizer, ParseTree->Children[1]))
        {
                return(true);
        }

        *Tokenizer = Start;
        if(ParseDeclarationSpecifiers(Tokenizer, ParseTree->Children[0]) &&
           ParseAbstractDeclarator(Tokenizer, ParseTree->Children[1]))
        {
                return(true);
        }

        PARSE_TREE_CLEAR_CHILDREN();

        *Tokenizer = Start;
        if(ParseDeclarationSpecifiers(Tokenizer, ParseTree->Children[0]))
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

        PARSE_TREE_UPDATE("Parameter List'", 3);

        if(Token_Comma == GetToken(Tokenizer).Type &&
           ParseParameterDeclaration(Tokenizer, ParseTree->Children[1]) &&
           ParseParameterListI(Tokenizer, ParseTree->Children[2]))
        {
                ParseTreeSet(ParseTree->Children[0], "symbol: ,");
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

        if(ParseParameterDeclaration(Tokenizer, ParseTree->Children[0]) &&
           ParseParameterListI(Tokenizer, ParseTree->Children[1]))
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

        PARSE_TREE_UPDATE("Parameter Type List", 3);

        if(ParseParameterList(Tokenizer, ParseTree->Children[0]))
        {
                struct tokenizer Previous = *Tokenizer;
                if(Token_Comma == GetToken(Tokenizer).Type &&
                   Token_Ellipsis == GetToken(Tokenizer).Type)
                {
                        ParseTreeSet(ParseTree->Children[1], "symbol: ,");
                        ParseTreeSet(ParseTree->Children[2], "symbol: ...");
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

        if(ParseTypeQualifier(Tokenizer, ParseTree->Children[0]) &&
           ParseTypeQualifierListI(Tokenizer, ParseTree->Children[1]))
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

        if(ParseTypeQualifier(Tokenizer, ParseTree->Children[0]) &&
           ParseTypeQualifierListI(Tokenizer, ParseTree->Children[1]))
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

        if(ParseTypeQualifierList(Tokenizer, ParseTree->Children[0]) &&
           ParsePointer(Tokenizer, ParseTree->Children[1]))
        {
                return(true);
        }

        PARSE_TREE_CLEAR_CHILDREN();

        *Tokenizer = AtToken;
        if(ParsePointer(Tokenizer, ParseTree->Children[0]))
        {
                return(true);
        }

        *Tokenizer = AtToken;
        if(ParseTypeQualifierList(Tokenizer, ParseTree->Children[0]))
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

        PARSE_TREE_UPDATE("Direct Declarator'", 4);

        if(Token_OpenBracket == GetToken(Tokenizer).Type &&
           ParseConstantExpression(Tokenizer, ParseTree->Children[1]) &&
           Token_CloseBracket == GetToken(Tokenizer).Type &&
           ParseDirectDeclaratorI(Tokenizer, ParseTree->Children[3]))
        {
                ParseTreeSet(ParseTree->Children[0], "symbol: [");
                ParseTreeSet(ParseTree->Children[2], "symbol: ]");
                return(true);
        }

        PARSE_TREE_CLEAR_CHILDREN();

        *Tokenizer = Start;
        if(Token_OpenBracket == GetToken(Tokenizer).Type &&
           Token_CloseBracket == GetToken(Tokenizer).Type &&
           ParseDirectDeclaratorI(Tokenizer, ParseTree->Children[2]))
        {
                ParseTreeSet(ParseTree->Children[0], "symbol: [");
                ParseTreeSet(ParseTree->Children[1], "symbol: ]");
                return(true);
        }

        *Tokenizer = Start;
        if(Token_OpenParen == GetToken(Tokenizer).Type &&
           ParseParameterTypeList(Tokenizer, ParseTree->Children[1]) &&
           Token_CloseParen == GetToken(Tokenizer).Type &&
           ParseDirectDeclaratorI(Tokenizer, ParseTree->Children[3]))
        {
                ParseTreeSet(ParseTree->Children[0], "symbol: (");
                ParseTreeSet(ParseTree->Children[2], "symbol: )");
                return(true);
        }

        *Tokenizer = Start;
        if(Token_OpenParen == GetToken(Tokenizer).Type &&
           ParseIdentifierList(Tokenizer, ParseTree->Children[1]) &&
           Token_CloseParen == GetToken(Tokenizer).Type &&
           ParseDirectDeclaratorI(Tokenizer, ParseTree->Children[3]))
        {
                ParseTreeSet(ParseTree->Children[0], "symbol: (");
                ParseTreeSet(ParseTree->Children[2], "symbol: )");
                return(true);
        }

        PARSE_TREE_CLEAR_CHILDREN();

        *Tokenizer = Start;
        if(Token_OpenParen == GetToken(Tokenizer).Type &&
           Token_CloseParen == GetToken(Tokenizer).Type &&
           ParseDirectDeclaratorI(Tokenizer, ParseTree->Children[2]))
        {
                ParseTreeSet(ParseTree->Children[0], "symbol: (");
                ParseTreeSet(ParseTree->Children[1], "symbol: )");
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

        PARSE_TREE_UPDATE("Direct Declarator", 4);

        if(ParseIdentifier(Tokenizer, ParseTree->Children[0]) &&
           ParseDirectDeclaratorI(Tokenizer, ParseTree->Children[1]))
        {
                return(true);
        }

        *Tokenizer = Start;
        if(Token_OpenParen == GetToken(Tokenizer).Type &&
           ParseDeclarator(Tokenizer, ParseTree->Children[1]) &&
           Token_CloseParen == GetToken(Tokenizer).Type &&
           ParseDirectDeclaratorI(Tokenizer, ParseTree->Children[3]))
        {
                ParseTreeSet(ParseTree->Children[0], "symbol: (");
                ParseTreeSet(ParseTree->Children[2], "symbol: )");
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

        if(ParsePointer(Tokenizer, ParseTree->Children[0]) &&
           ParseDirectDeclarator(Tokenizer, ParseTree->Children[1]))
        {
                return(true);
        }

        PARSE_TREE_CLEAR_CHILDREN();

        *Tokenizer = Start;
        if(ParseDirectDeclarator(Tokenizer, ParseTree->Children[0]))
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

        PARSE_TREE_UPDATE("Enumerator", 3);

        if(ParseIdentifier(Tokenizer, ParseTree->Children[0]) &&
           Token_EqualSign == GetToken(Tokenizer).Type &&
           ParseConstantExpression(Tokenizer, ParseTree->Children[2]))
        {
                ParseTreeSet(ParseTree->Children[1], "symbol: =");
                return(true);
        }

        PARSE_TREE_CLEAR_CHILDREN();

        *Tokenizer = Start;
        if(ParseIdentifier(Tokenizer, ParseTree->Children[0]))
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

        PARSE_TREE_UPDATE("Enumerator List'", 3);

        if(Token_Comma == GetToken(Tokenizer).Type &&
           ParseEnumerator(Tokenizer, ParseTree->Children[1]) &&
           ParseEnumeratorListI(Tokenizer, ParseTree->Children[2]))
        {
                ParseTreeSet(ParseTree->Children[0], "symbol: ,");
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

        if(ParseEnumerator(Tokenizer, ParseTree->Children[0]) &&
           ParseEnumeratorListI(Tokenizer, ParseTree->Children[1]))
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

        PARSE_TREE_UPDATE("Enum Specifier", 5);

        if(!(Token_Keyword == Token.Type &&
             GSStringIsEqual("enum", Token.Text, Token.TextLength)))
        {
                *Tokenizer = Start;
                return(false);
        }

        ParseTreeSet(ParseTree->Children[0], "keyword: enum");

        if(ParseIdentifier(Tokenizer, ParseTree->Children[1]) &&
           Token_OpenBrace == GetToken(Tokenizer).Type &&
           ParseEnumeratorList(Tokenizer, ParseTree->Children[3]) &&
           Token_CloseBrace == GetToken(Tokenizer).Type)
        {
                ParseTreeSet(ParseTree->Children[2], "symbol: {");
                ParseTreeSet(ParseTree->Children[4], "symbol: }");
                return(true);
        }


        PARSE_TREE_CLEAR_CHILDREN();

        *Tokenizer = AtToken;
        if(Token_OpenBrace == GetToken(Tokenizer).Type &&
           ParseEnumeratorList(Tokenizer, ParseTree->Children[2]) &&
           Token_CloseBrace == GetToken(Tokenizer).Type)
        {
                ParseTreeSet(ParseTree->Children[1], "symbol: {");
                ParseTreeSet(ParseTree->Children[3], "symbol: }");
                return(true);
        }

        PARSE_TREE_CLEAR_CHILDREN();

        *Tokenizer = AtToken;
        if(ParseIdentifier(Tokenizer, ParseTree->Children[0]))
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

        PARSE_TREE_UPDATE("Struct Declarator", 3);

        if(ParseDeclarator(Tokenizer, ParseTree->Children[0]) &&
           Token_Colon == GetToken(Tokenizer).Type &&
           ParseConstantExpression(Tokenizer, ParseTree->Children[2]))
        {
                ParseTreeSet(ParseTree->Children[1], "symbol: :");
                return(true);
        }

        PARSE_TREE_CLEAR_CHILDREN();

        *Tokenizer = Start;
        if(Token_Colon == GetToken(Tokenizer).Type &&
           ParseConstantExpression(Tokenizer, ParseTree->Children[1]))
        {
                ParseTreeSet(ParseTree->Children[0], "symbol: :");
                return(true);
        }

        PARSE_TREE_CLEAR_CHILDREN();

        *Tokenizer = Start;
        if(ParseDeclarator(Tokenizer, ParseTree->Children[0]))
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

        PARSE_TREE_UPDATE("Struct Declarator List'", 3);

        if(Token_Comma == GetToken(Tokenizer).Type &&
           ParseStructDeclarator(Tokenizer, ParseTree->Children[1]) &&
           ParseStructDeclaratorListI(Tokenizer, ParseTree->Children[2]))
        {
                ParseTreeSet(ParseTree->Children[0], "symbol: ,");
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

        if(ParseStructDeclarator(Tokenizer, ParseTree->Children[0]) &&
           ParseStructDeclaratorListI(Tokenizer, ParseTree->Children[1]))
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

        if(ParseTypeSpecifier(Tokenizer, ParseTree->Children[0]) &&
           ParseSpecifierQualifierList(Tokenizer, ParseTree->Children[1]))
        {
                return(true);
        }

        PARSE_TREE_CLEAR_CHILDREN();

        *Tokenizer = Start;
        if(ParseTypeSpecifier(Tokenizer, ParseTree->Children[0]))
        {
                return(true);
        }

        *Tokenizer = Start;
        if(ParseTypeQualifier(Tokenizer, ParseTree->Children[0]) &&
           ParseSpecifierQualifierList(Tokenizer, ParseTree->Children[1]))
        {
                return(true);
        }

        PARSE_TREE_CLEAR_CHILDREN();

        *Tokenizer = Start;
        if(ParseTypeQualifier(Tokenizer, ParseTree->Children[0]))
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

        PARSE_TREE_UPDATE("Struct Declaration", 3);

        if(ParseSpecifierQualifierList(Tokenizer, ParseTree->Children[0]) &&
           ParseStructDeclaratorList(Tokenizer, ParseTree->Children[1]) &&
           Token_SemiColon == GetToken(Tokenizer).Type)
        {
                ParseTreeSet(ParseTree->Children[2], "Struct Declaration");
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

        PARSE_TREE_UPDATE("Init Declarator", 3);

        if(ParseDeclarator(Tokenizer, ParseTree->Children[0]) &&
           Token_EqualSign == GetToken(Tokenizer).Type &&
           ParseInitializer(Tokenizer, ParseTree->Children[2]))
        {
                ParseTreeSet(ParseTree, "symbol: =");
                return(true);
        }

        PARSE_TREE_CLEAR_CHILDREN();

        *Tokenizer = Start;
        if(ParseDeclarator(Tokenizer, ParseTree->Children[0]))
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

        PARSE_TREE_UPDATE("Init Declarator List'", 2);

        if(Token_Comma == GetToken(Tokenizer).Type &&
           ParseInitDeclarator(Tokenizer, ParseTree->Children[1]))
        {
                ParseTreeSet(ParseTree->Children[0], "symbol: ,");
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

        if(ParseInitDeclarator(Tokenizer, ParseTree->Children[0]) &&
           ParseInitDeclaratorListI(Tokenizer, ParseTree->Children[1]))
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

        if(ParseStructDeclaration(Tokenizer, ParseTree->Children[0]) &&
           ParseStructDeclarationListI(Tokenizer, ParseTree->Children[1]))
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

        if(ParseStructDeclaration(Tokenizer, ParseTree->Children[0]) &&
           ParseStructDeclarationListI(Tokenizer, ParseTree->Children[1]))
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
                        ParseTreeSet(ParseTree, "Struct or Union");
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

        if(ParseStructOrUnion(Tokenizer, ParseTree->Children[0]) &&
           ParseIdentifier(Tokenizer, ParseTree->Children[1]) &&
           Token_OpenBrace == GetToken(Tokenizer).Type &&
           ParseStructDeclarationList(Tokenizer, ParseTree->Children[3]) &&
           Token_CloseBrace == GetToken(Tokenizer).Type)
        {
                ParseTreeSet(ParseTree->Children[2], "symbol: (");
                ParseTreeSet(ParseTree->Children[4], "symbol: )");
                return(true);
        }

        PARSE_TREE_CLEAR_CHILDREN();

        *Tokenizer = Start;
        if(ParseStructOrUnion(Tokenizer, ParseTree->Children[0]) &&
           Token_OpenBrace == GetToken(Tokenizer).Type &&
           ParseStructDeclarationList(Tokenizer, ParseTree->Children[2]) &&
           Token_CloseBrace == GetToken(Tokenizer).Type)
        {
                ParseTreeSet(ParseTree->Children[1], "symbol: (");
                ParseTreeSet(ParseTree->Children[3], "symbol: )");
                return(true);
        }

        PARSE_TREE_CLEAR_CHILDREN();

        *Tokenizer = Start;
        if(ParseStructOrUnion(Tokenizer, ParseTree->Children[0]) &&
           ParseIdentifier(Tokenizer, ParseTree->Children[1]))
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
                        ParseTreeSet(ParseTree, "Type Qualifier");
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
                                ParseTreeSet(ParseTree, "Type Specifier");
                                return(true);
                        }
                }
        }

        PARSE_TREE_UPDATE("Type Specifier", 1);

        *Tokenizer = Start;
        if(ParseStructOrUnionSpecifier(Tokenizer, ParseTree->Children[0]))
        {
                return(true);
        }

        *Tokenizer = Start;
        if(ParseEnumSpecifier(Tokenizer, ParseTree->Children[0]))
        {
                return(true);
        }

        *Tokenizer = Start;
        if(ParseTypedefName(Tokenizer, ParseTree->Children[0]))
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
                                ParseTreeSet(ParseTree, "Storage Class Specifier");
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

        if(ParseStorageClassSpecifier(Tokenizer, ParseTree->Children[0]) &&
           ParseDeclarationSpecifiers(Tokenizer, ParseTree->Children[1]))
        {
                return(true);
        }

        *Tokenizer = Start;
        if(ParseTypeSpecifier(Tokenizer, ParseTree->Children[0]) &&
           ParseDeclarationSpecifiers(Tokenizer, ParseTree->Children[1]))
        {
                return(true);
        }

        *Tokenizer = Start;
        if(ParseTypeQualifier(Tokenizer, ParseTree->Children[0]) &&
           ParseDeclarationSpecifiers(Tokenizer, ParseTree->Children[1]))
        {
                return(true);
        }

        *Tokenizer = Start;
        if(ParseStorageClassSpecifier(Tokenizer, ParseTree->Children[0]))
        {
                return(true);
        }

        PARSE_TREE_CLEAR_CHILDREN();

        *Tokenizer = Start;
        if(ParseTypeSpecifier(Tokenizer, ParseTree->Children[0]))
        {
                return(true);
        }

        *Tokenizer = Start;
        if(ParseTypeQualifier(Tokenizer, ParseTree->Children[0]))
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

        if(ParseDeclaration(Tokenizer, ParseTree->Children[0]) &&
           ParseDeclarationListI(Tokenizer, ParseTree->Children[1]))
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

        if(ParseDeclaration(Tokenizer, ParseTree->Children[0]) &&
           ParseDeclarationListI(Tokenizer, ParseTree->Children[1]))
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

        PARSE_TREE_UPDATE("Declaration", 3);

        if(ParseDeclarationSpecifiers(Tokenizer, ParseTree->Children[0]) &&
           ParseInitDeclaratorList(Tokenizer, ParseTree->Children[1]) &&
           Token_SemiColon == GetToken(Tokenizer).Type)
        {
                ParseTreeSet(ParseTree->Children[2], "symbol: ;");
                return(true);
        }

        *Tokenizer = Start;
        if(ParseDeclarationSpecifiers(Tokenizer, ParseTree->Children[0]) &&
           Token_SemiColon == GetToken(Tokenizer).Type)
        {
                ParseTreeSet(ParseTree->Children[1], "symbol: ;");
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

        if(ParseDeclarationSpecifiers(Tokenizer, ParseTree->Children[0]) &&
           ParseDeclarator(Tokenizer, ParseTree->Children[1]) &&
           ParseDeclarationList(Tokenizer, ParseTree->Children[2]) &&
           ParseCompoundStatement(Tokenizer, ParseTree->Children[3]))
        {
                return(true);
        }

        PARSE_TREE_CLEAR_CHILDREN();

        *Tokenizer = Start;
        if(ParseDeclarationSpecifiers(Tokenizer, ParseTree->Children[0]) &&
           ParseDeclarator(Tokenizer, ParseTree->Children[1]) &&
           ParseCompoundStatement(Tokenizer, ParseTree->Children[2]))
        {
                return(true);
        }

        *Tokenizer = Start;
        if(ParseDeclarator(Tokenizer, ParseTree->Children[0]) &&
           ParseDeclarationList(Tokenizer, ParseTree->Children[1]) &&
           ParseCompoundStatement(Tokenizer, ParseTree->Children[2]))
        {
                return(true);
        }

        PARSE_TREE_CLEAR_CHILDREN();

        *Tokenizer = Start;
        if(ParseDeclarator(Tokenizer, ParseTree->Children[0]) &&
           ParseCompoundStatement(Tokenizer, ParseTree->Children[1]))
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

        if(ParseFunctionDefinition(Tokenizer, ParseTree->Children[0])) return(true);

        *Tokenizer = Start;
        if(ParseDeclaration(Tokenizer, ParseTree->Children[0])) return(true);

        *Tokenizer = Start;
        return(false);
}

bool
ParseTranslationUnitI(struct tokenizer *Tokenizer, parse_tree_node *ParseTree)
{
        struct tokenizer Start = *Tokenizer;

        PARSE_TREE_UPDATE("Translation Unit'", 2);

        if(ParseExternalDeclaration(Tokenizer, ParseTree->Children[0]) &&
           ParseTranslationUnitI(Tokenizer, ParseTree->Children[1]))
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

        if(ParseExternalDeclaration(Tokenizer, ParseTree->Children[0]) &&
           ParseTranslationUnitI(Tokenizer, ParseTree->Children[1]))
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
                                if(Result)
                                {
                                        puts("Successfully parsed input");
                                }
                                else
                                {
                                        puts("Input did not parse");
                                        Parsing = false;
                                }
                        } break;
                }
        }
}

#endif /* _PARSER_C */
