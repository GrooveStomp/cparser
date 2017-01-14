#ifndef _PARSER_C
#define _PARSER_C

#include <stdio.h>
#include "bool.c"
#include "lexer.c"

/* NOTE(AARON): Debug include - tree.c */
#include "tree.c"

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

        if(Token_Keyword == Token.Type &&
           GSStringIsEqual("if", Token.Text, Token.TextLength) &&
           Token_OpenParen == GetToken(Tokenizer).Type &&
           ParseExpression(Tokenizer) &&
           Token_CloseParen == GetToken(Tokenizer).Type &&
           ParseStatement(Tokenizer))
        {
                struct tokenizer AtElse = *Tokenizer;
                struct token Token = GetToken(Tokenizer);

                if(Token_Keyword == Token.Type &&
                   GSStringIsEqual("else", Token.Text, Token.TextLength) &&
                   ParseStatement(Tokenizer))
                {
                        return(true);
                }

                *Tokenizer = AtElse;
                return(true);
        }

        *Tokenizer = AtToken;
        if(Token_Keyword == Token.Type &&
           GSStringIsEqual("switch", Token.Text, Token.TextLength) &&
           Token_OpenParen == GetToken(Tokenizer).Type &&
           ParseExpression(Tokenizer) &&
           Token_CloseParen == GetToken(Tokenizer).Type &&
           ParseStatement(Tokenizer))
        {
                return(true);
        }

        *Tokenizer = Start;
        return(false);
}

bool
ParseStatementListI(struct tokenizer *Tokenizer, parse_tree_node *ParseTree)
{
        struct tokenizer Start = *Tokenizer;

        if(ParseStatement(Tokenizer) &&
           ParseStatementListI(Tokenizer))
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

        if(ParseStatement(Tokenizer) &&
           ParseStatementListI(Tokenizer))
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

        if(Token_OpenBrace == GetToken(Tokenizer).Type)
        {
                struct tokenizer Previous = *Tokenizer;
                if(!ParseDeclarationList(Tokenizer))
                {
                        *Tokenizer = Previous;
                }

                Previous = *Tokenizer;
                if(!ParseStatementList(Tokenizer))
                {
                        *Tokenizer = Previous;
                }

                if(Token_CloseBrace == GetToken(Tokenizer).Type)
                {
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

        if(ParseExpression(Tokenizer) &&
           Token_SemiColon == GetToken(Tokenizer).Type)
        {
                return(true);
        }

        *Tokenizer = Start;
        if(Token_SemiColon == GetToken(Tokenizer).Type)
        {
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

        if(ParseIdentifier(Tokenizer) &&
           Token_Colon == GetToken(Tokenizer).Type &&
           ParseStatement(Tokenizer))
        {
                return(true);
        }

        *Tokenizer = Start;
        struct token Token = GetToken(Tokenizer);
        struct tokenizer AtToken = *Tokenizer;

        if(Token_Keyword == Token.Type &&
           GSStringIsEqual("case", Token.Text, Token.TextLength) &&
           ParseConstantExpression(Tokenizer) &&
           Token_Colon == GetToken(Tokenizer).Type &&
           ParseStatement(Tokenizer))
        {
                return(true);
        }

        *Tokenizer = AtToken;
        if(Token_Keyword == Token.Type &&
           GSStringIsEqual("default", Token.Text, Token.TextLength) &&
           Token_Colon == GetToken(Tokenizer).Type &&
           ParseStatement(Tokenizer))
        {
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

        if(ParseLabeledStatement(Tokenizer)) return(true);

        *Tokenizer = Start;
        if(ParseExpressionStatement(Tokenizer)) return(true);

        *Tokenizer = Start;
        if(ParseCompoundStatement(Tokenizer)) return(true);

        *Tokenizer = Start;
        if(ParseSelectionStatement(Tokenizer)) return(true);

        *Tokenizer = Start;
        if(ParseIterationStatement(Tokenizer)) return(true);

        *Tokenizer = Start;
        if(ParseJumpStatement(Tokenizer)) return(true);

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

        if(ParseIdentifier(Tokenizer) && TypedefIsName(Token))
        {
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

        if(Token_OpenBracket == GetToken(Tokenizer).Type &&
           ParseConstantExpression(Tokenizer) &&
           Token_CloseBracket == GetToken(Tokenizer).Type &&
           ParseDirectAbstractDeclaratorI(Tokenizer))
        {
                return(true);
        }

        *Tokenizer = Start;
        if(Token_OpenBracket == GetToken(Tokenizer).Type &&
           Token_CloseBracket == GetToken(Tokenizer).Type &&
           ParseDirectAbstractDeclaratorI(Tokenizer))
        {
                return(true);
        }

        *Tokenizer = Start;
        if(Token_OpenParen == GetToken(Tokenizer).Type &&
           ParseParameterTypeList(Tokenizer) &&
           Token_CloseParen == GetToken(Tokenizer).Type &&
           ParseDirectAbstractDeclaratorI(Tokenizer))
        {
                return(true);
        }

        *Tokenizer = Start;
        if(Token_OpenParen == GetToken(Tokenizer).Type &&
           Token_CloseParen == GetToken(Tokenizer).Type &&
           ParseDirectAbstractDeclaratorI(Tokenizer))
        {
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

        if(Token_OpenParen == GetToken(Tokenizer).Type &&
           ParseAbstractDeclarator(Tokenizer) &&
           Token_CloseParen == GetToken(Tokenizer).Type &&
           ParseDirectAbstractDeclaratorI(Tokenizer))
        {
                return(true);
        }

        *Tokenizer = Start;
        if(Token_OpenBracket == GetToken(Tokenizer).Type &&
           ParseConstantExpression(Tokenizer) &&
           Token_CloseBracket == GetToken(Tokenizer).Type &&
           ParseDirectAbstractDeclaratorI(Tokenizer))
        {
                return(true);
        }

        *Tokenizer = Start;
        if(Token_OpenBracket == GetToken(Tokenizer).Type &&
           Token_CloseBracket == GetToken(Tokenizer).Type &&
           ParseDirectAbstractDeclaratorI(Tokenizer))
        {
                return(true);
        }

        *Tokenizer = Start;
        if(Token_OpenParen == GetToken(Tokenizer).Type &&
           ParseParameterTypeList(Tokenizer) &&
           Token_CloseParen == GetToken(Tokenizer).Type &&
           ParseDirectAbstractDeclaratorI(Tokenizer))
        {
                return(true);
        }

        *Tokenizer = Start;
        if(Token_OpenParen == GetToken(Tokenizer).Type &&
           Token_CloseParen == GetToken(Tokenizer).Type &&
           ParseDirectAbstractDeclaratorI(Tokenizer))
        {
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

        if(ParsePointer(Tokenizer) &&
           ParseDirectAbstractDeclarator(Tokenizer))
        {
                return(true);
        }

        *Tokenizer = Start;
        if(ParseDirectAbstractDeclarator(Tokenizer))
        {
                return(true);
        }

        *Tokenizer = Start;
        if(ParsePointer(Tokenizer))
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

        if(ParseSpecifierQualifierList(Tokenizer) &&
           ParseAbstractDeclarator(Tokenizer))
        {
                return(true);
        }

        *Tokenizer = Start;
        if(ParseSpecifierQualifierList(Tokenizer))
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

        if(Token_Comma == GetToken(Tokenizer).Type &&
           ParseInitializer(Tokenizer) &&
           ParseInitializerListI(Tokenizer))
        {
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

        if(ParseInitializer(Tokenizer) &&
           ParseInitializerListI(Tokenizer))
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

        if(ParseAssignmentExpression(Tokenizer))
        {
                return(true);
        }

        *Tokenizer = Start;
        if(Token_OpenBrace == GetToken(Tokenizer).Type &&
           ParseInitializerList(Tokenizer) &&
           Token_CloseBrace == GetToken(Tokenizer).Type)
        {
                return(true);
        }

        *Tokenizer = Start;
        if(Token_OpenBrace == GetToken(Tokenizer).Type &&
           ParseInitializerList(Tokenizer) &&
           Token_Comma == GetToken(Tokenizer).Type &&
           Token_CloseBrace == GetToken(Tokenizer).Type)
        {
                return(true);
        }

        *Tokenizer = Start;
        return(false);
}

bool
ParseIdentifierListI(struct tokenizer *Tokenizer, parse_tree_node *ParseTree)
{
        struct tokenizer Start = *Tokenizer;

        if(Token_Comma == GetToken(Tokenizer).Type &&
           ParseIdentifier(Tokenizer) &&
           ParseIdentifierListI(Tokenizer))
        {
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

        if(ParseIdentifier(Tokenizer) &&
           ParseIdentifierListI(Tokenizer))
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

        if(ParseDeclarationSpecifiers(Tokenizer) &&
           ParseDeclarator(Tokenizer))
        {
                return(true);
        }

        *Tokenizer = Start;
        if(ParseDeclarationSpecifiers(Tokenizer) &&
           ParseAbstractDeclarator(Tokenizer))
        {
                return(true);
        }

        *Tokenizer = Start;
        if(ParseDeclarationSpecifiers(Tokenizer))
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

        if(Token_Comma == GetToken(Tokenizer).Type &&
           ParseParameterDeclaration(Tokenizer) &&
           ParseParameterListI(Tokenizer))
        {
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

        if(ParseParameterDeclaration(Tokenizer) &&
           ParseParameterListI(Tokenizer))
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

        if(ParseParameterList(Tokenizer))
        {
                struct tokenizer Previous = *Tokenizer;
                if(Token_Comma == GetToken(Tokenizer).Type &&
                   Token_Ellipsis == GetToken(Tokenizer).Type)
                {
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

        if(ParseTypeQualifier(Tokenizer) &&
           ParseTypeQualifierListI(Tokenizer))
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

        if(ParseTypeQualifier(Tokenizer) &&
           ParseTypeQualifierListI(Tokenizer))
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

        if(Token_Asterisk != Token.Type)
        {
                *Tokenizer = Start;
                return(false);
        }

        if(ParseTypeQualifierList(Tokenizer) &&
           ParsePointer(Tokenizer))
        {
                return(true);
        }

        *Tokenizer = AtToken;
        if(ParsePointer(Tokenizer))
        {
                return(true);
        }

        *Tokenizer = AtToken;
        if(ParseTypeQualifierList(Tokenizer))
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

        if(Token_OpenBracket == GetToken(Tokenizer).Type &&
           ParseConstantExpression(Tokenizer) &&
           Token_CloseBracket == GetToken(Tokenizer).Type &&
           ParseDirectDeclaratorI(Tokenizer))
        {
                return(true);
        }

        *Tokenizer = Start;
        if(Token_OpenBracket == GetToken(Tokenizer).Type &&
           Token_CloseBracket == GetToken(Tokenizer).Type &&
           ParseDirectDeclaratorI(Tokenizer))
        {
                return(true);
        }

        *Tokenizer = Start;
        if(Token_OpenParen == GetToken(Tokenizer).Type &&
           ParseParameterTypeList(Tokenizer) &&
           Token_CloseParen == GetToken(Tokenizer).Type &&
           ParseDirectDeclaratorI(Tokenizer))
        {
                return(true);
        }

        *Tokenizer = Start;
        if(Token_OpenParen == GetToken(Tokenizer).Type &&
           ParseIdentifierList(Tokenizer) &&
           Token_CloseParen == GetToken(Tokenizer).Type &&
           ParseDirectDeclaratorI(Tokenizer))
        {
                return(true);
        }

        *Tokenizer = Start;
        if(Token_OpenParen == GetToken(Tokenizer).Type &&
           Token_CloseParen == GetToken(Tokenizer).Type &&
           ParseDirectDeclaratorI(Tokenizer))
        {
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

        if(ParseIdentifier(Tokenizer) &&
           ParseDirectDeclaratorI(Tokenizer))
        {
                return(true);
        }

        *Tokenizer = Start;
        if(Token_OpenParen == GetToken(Tokenizer).Type &&
           ParseDeclarator(Tokenizer) &&
           Token_CloseParen == GetToken(Tokenizer).Type &&
           ParseDirectDeclaratorI(Tokenizer))
        {
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

        if(ParsePointer(Tokenizer) &&
           ParseDirectDeclarator(Tokenizer))
        {
                return(true);
        }

        *Tokenizer = Start;
        if(ParseDirectDeclarator(Tokenizer))
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

        if(ParseIdentifier(Tokenizer) &&
           Token_EqualSign == GetToken(Tokenizer).Type &&
           ParseConstantExpression(Tokenizer))
        {
                return(true);
        }

        *Tokenizer = Start;
        if(ParseIdentifier(Tokenizer))
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

        if(Token_Comma == GetToken(Tokenizer).Type &&
           ParseEnumerator(Tokenizer) &&
           ParseEnumeratorListI(Tokenizer))
        {
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

        if(ParseEnumerator(Tokenizer) &&
           ParseEnumeratorListI(Tokenizer))
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

        if(!(Token_Keyword == Token.Type &&
             GSStringIsEqual("enum", Token.Text, Token.TextLength)))
        {
                *Tokenizer = Start;
                return(false);
        }

        if(ParseIdentifier(Tokenizer) &&
           Token_OpenBrace == GetToken(Tokenizer).Type &&
           ParseEnumeratorList(Tokenizer) &&
           Token_CloseBrace == GetToken(Tokenizer).Type)
        {
                return(true);
        }

        *Tokenizer = AtToken;
        if(Token_OpenBrace == GetToken(Tokenizer).Type &&
           ParseEnumeratorList(Tokenizer) &&
           Token_CloseBrace == GetToken(Tokenizer).Type)
        {
                return(true);
        }

        *Tokenizer = AtToken;
        if(ParseIdentifier(Tokenizer))
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

        if(ParseDeclarator(Tokenizer) &&
           Token_Colon == GetToken(Tokenizer).Type &&
           ParseConstantExpression(Tokenizer))
        {
                return(true);
        }

        *Tokenizer = Start;
        if(Token_Colon == GetToken(Tokenizer).Type &&
           ParseConstantExpression(Tokenizer))
        {
                return(true);
        }

        *Tokenizer = Start;
        if(ParseDeclarator(Tokenizer))
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

        if(Token_Comma == GetToken(Tokenizer).Type &&
           ParseStructDeclarator(Tokenizer) &&
           ParseStructDeclaratorListI(Tokenizer))
        {
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

        if(ParseStructDeclarator(Tokenizer) &&
           ParseStructDeclaratorListI(Tokenizer))
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

        if(ParseTypeSpecifier(Tokenizer) &&
           ParseSpecifierQualifierList(Tokenizer))
        {
                return(true);
        }

        *Tokenizer = Start;
        if(ParseTypeSpecifier(Tokenizer))
        {
                return(true);
        }

        *Tokenizer = Start;
        if(ParseTypeQualifier(Tokenizer) &&
           ParseSpecifierQualifierList(Tokenizer))
        {
                return(true);
        }

        *Tokenizer = Start;
        if(ParseTypeQualifier(Tokenizer))
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

        if(ParseSpecifierQualifierList(Tokenizer) &&
           ParseStructDeclaratorList(Tokenizer) &&
           Token_SemiColon == GetToken(Tokenizer).Type)
        {
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

        if(ParseDeclarator(Tokenizer) &&
           Token_EqualSign == GetToken(Tokenizer).Type &&
           ParseInitializer(Tokenizer))
        {
                return(true);
        }

        *Tokenizer = Start;
        if(ParseDeclarator(Tokenizer))
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

        if(Token_Comma == GetToken(Tokenizer).Type &&
           ParseInitDeclarator(Tokenizer))
        {
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

        if(ParseInitDeclarator(Tokenizer) &&
           ParseInitDeclaratorListI(Tokenizer))
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

        if(ParseStructDeclaration(Tokenizer) &&
           ParseStructDeclarationListI(Tokenizer))
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

        if(ParseStructDeclaration(Tokenizer) &&
           ParseStructDeclarationListI(Tokenizer))
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

        ParseTreeSet(ParseTree, NULL, "Struct or Union Specifier", GSStringLength("Struct or Union Specifier"));
        ParseTreeNewChildren(ParseTree, 5);

        if(ParseStructOrUnion(Tokenizer, ParseTree->Children[0]) &&
           ParseIdentifier(Tokenizer, ParseTree->Children[1]) &&
           Token_OpenBrace == GetToken(Tokenizer).Type &&
           ParseStructDeclarationList(Tokenizer, ParseTree->Children[3]) &&
           Token_CloseBrace == GetToken(Tokenizer).Type)
        {
                ParseTreeSet(ParseTree->Children[2], NULL, "(", 1);
                ParseTreeSet(ParseTree->Children[4], NULL, ")", 1);
                return(true);
        }

        *Tokenizer = Start;
        if(ParseStructOrUnion(Tokenizer, ParseTree->Children[0]) &&
           Token_OpenBrace == GetToken(Tokenizer).Type &&
           ParseStructDeclarationList(Tokenizer, ParseTree->Children[2]) &&
           Token_CloseBrace == GetToken(Tokenizer).Type)
        {
                ParseTreeSet(ParseTree->Children[1], NULL, "(", 1);
                ParseTreeSet(ParseTree->Children[3], NULL, ")", 1);
                ParseTreeSet(ParseTree->Children[4], NULL, "Empty", 5);
                return(true);
        }

        *Tokenizer = Start;
        if(ParseStructOrUnion(Tokenizer, ParseTree->Children[0]) &&
           ParseIdentifier(Tokenizer, ParseTree->Children[1]))
        {
                ParseTreeSet(ParseTree->Children[2], NULL, "Empty", 5);
                ParseTreeSet(ParseTree->Children[3], NULL, "Empty", 5);
                ParseTreeSet(ParseTree->Children[4], NULL, "Empty", 5);
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
                        ParseTreeSet(ParseTree, Token, "Type Qualifier", GSStringLength("Type Qualifier"));
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
                                ParseTreeSet(ParseTree, &Token, "Type Specifier", GSStringLength("Type Specifier"));
                                return(true);
                        }
                }
        }

        ParseTreeSet(ParseTree, NULL, "Type Specifier", GSStringLength("Type Specifier"));
        ParseTreeNewChild(ParseTree);

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
                                ParseTreeSet(ParseTree, &Token, "Storage Class Specifier", GSStringLength("Storage Class Specifier"));
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

        ParseTreeSet(ParseTree, NULL, "Declaration Specifiers", GSStringLength("Declaration Specifiers"));
        ParseTreeNewChildren(ParseTree, 2);

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

        ParseTreeSet(ParseTree->Children[1], NULL, "Empty", 5);

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

        ParseTreeSet(ParseTree, NULL, "Declaration List'", GSStringLength("Declaration List'"));
        ParseTreeNewChildren(ParseTree, 2);

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

        ParseTreeSet(ParseTree, NULL, "Declaration List", GSStringLength("Declaration List"));
        ParseTreeNewChildren(ParseTree, 2);

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

        ParseTreeSet(ParseTree, NULL, "Declaration", GSStringLength("Declaration"));
        ParseTreeNewChildren(ParseTree, 3);

        if(ParseDeclarationSpecifiers(Tokenizer, ParseTree->Children[0]) &&
           ParseInitDeclaratorList(Tokenizer, ParseTree->Children[1]) &&
           Token_SemiColon == GetToken(Tokenizer).Type)
        {
                ParseTreeSet(ParseTree->Children[2], NULL, "SemiColon", GSStringLength("SemiColon"));
                return(true);
        }

        *Tokenizer = Start;
        if(ParseDeclarationSpecifiers(Tokenizer, ParseTree->Children[0]) &&
           Token_SemiColon == GetToken(Tokenizer).Type)
        {
                ParseTreeSet(ParseTree->Children[1], NULL, "SemiColon", GSStringLength("SemiColon"));
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

        ParseTreeSet(ParseTree, NULL, "Function Definition", GSStringLength("Function Definition"));
        ParseTreeNewChildren(ParseTree, 4);

        if(ParseDeclarationSpecifiers(Tokenizer, ParseTree->Children[0]) &&
           ParseDeclarator(Tokenizer, ParseTree->Children[1]) &&
           ParseDeclarationList(Tokenizer, ParseTree->Children[2]) &&
           ParseCompoundStatement(Tokenizer, ParseTree->Children[3]))
        {
                return(true);
        }

        ParseTreeSet(ParseTree->Children[3], NULL, "Empty", 5);

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

        ParseTreeSet(ParseTree->Children[2], NULL, "Empty", 5);

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

        ParseTreeSet(ParseTree, NULL, "External Declaration", GSStringLength("External Declaration"));
        ParseTreeNewChild(ParseTree);

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

        ParseTreeSet(ParseTree, NULL, "Translation Unit'", GSStringLength("Translation Unit'"));
        ParseTreeNewChildren(ParseTree, 2);

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

        ParseTreeSet(ParseTree, NULL, "Translation Unit", GSStringLength("Translation Unit"));
        ParseTreeNewChildren(ParseTree, 2)

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
