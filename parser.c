#ifndef _PARSER_C
#define _PARSER_C

#include <stdio.h>
#include "bool.c"
#include "lexer.c"

bool ParseAssignmentExpression(struct tokenizer *Tokenizer);
bool ParseExpression(struct tokenizer *Tokenizer);
bool ParseTypeName(struct tokenizer *Tokenizer);
bool ParseCastExpression(struct tokenizer *Tokenizer);
bool ParseConditionalExpression(struct tokenizer *Tokenizer);
bool ParseStatement(struct tokenizer *Tokenizer);
bool ParseDeclarationList(struct tokenizer *Tokenizer);
bool ParseParameterTypeList(struct tokenizer *Tokenizer);
bool ParseAbstractDeclarator(struct tokenizer *Tokenizer);
bool ParsePointer(struct tokenizer *Tokenizer);
bool ParseSpecifierQualifierList(struct tokenizer *Tokenizer);
bool ParseInitializer(struct tokenizer *Tokenizer);
bool ParseDeclarationSpecifiers(struct tokenizer *Tokenizer);
bool ParseDeclarator(struct tokenizer *Tokenizer);
bool ParseTypeQualifier(struct tokenizer *Tokneizer);
bool ParseTypeSpecifier(struct tokenizer *Tokenizer);
bool ParseDeclaration(struct tokenizer *Tokenizer);

void ParseError(char *Name, struct tokenizer *Tokenizer)
{
#if 0
        printf("%s Failure (%i,%i)\n", Name, Tokenizer->Line, Tokenizer->Column);
#endif
}

/*
  constant:
          integer-constant
          character-constant
          floating-constant
          enumeration-constant
*/
bool
ParseConstant(struct tokenizer *Tokenizer)
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
ParseArgumentExpressionListI(struct tokenizer *Tokenizer)
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
ParseArgumentExpressionList(struct tokenizer *Tokenizer)
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
ParsePrimaryExpression(struct tokenizer *Tokenizer)
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
ParsePostfixExpressionI(struct tokenizer *Tokenizer)
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
ParsePostfixExpression(struct tokenizer *Tokenizer)
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
ParseUnaryOperator(struct tokenizer *Tokenizer)
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
ParseUnaryExpression(struct tokenizer *Tokenizer)
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
           IsStringEqual("sizeof", Token.Text, StringLength("sizeof")))
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
ParseCastExpression(struct tokenizer *Tokenizer)
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
ParseMultiplicativeExpressionI(struct tokenizer *Tokenizer)
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
ParseMultiplicativeExpression(struct tokenizer *Tokenizer)
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
ParseAdditiveExpressionI(struct tokenizer *Tokenizer)
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
ParseAdditiveExpression(struct tokenizer *Tokenizer)
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
ParseShiftExpressionI(struct tokenizer *Tokenizer)
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
ParseShiftExpression(struct tokenizer *Tokenizer)
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
ParseRelationalExpressionI(struct tokenizer *Tokenizer)
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
ParseRelationalExpression(struct tokenizer *Tokenizer)
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
ParseEqualityExpressionI(struct tokenizer *Tokenizer)
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
ParseEqualityExpression(struct tokenizer *Tokenizer)
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
ParseAndExpressionI(struct tokenizer *Tokenizer)
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
ParseAndExpression(struct tokenizer *Tokenizer)
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
ParseExclusiveOrExpressionI(struct tokenizer *Tokenizer)
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
ParseExclusiveOrExpression(struct tokenizer *Tokenizer)
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
ParseInclusiveOrExpressionI(struct tokenizer *Tokenizer)
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
ParseInclusiveOrExpression(struct tokenizer *Tokenizer)
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
ParseLogicalAndExpressionI(struct tokenizer *Tokenizer)
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
ParseLogicalAndExpression(struct tokenizer *Tokenizer)
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
ParseLogicalOrExpressionI(struct tokenizer *Tokenizer)
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
ParseLogicalOrExpression(struct tokenizer *Tokenizer)
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
ParseConstantExpression(struct tokenizer *Tokenizer)
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
ParseConditionalExpression(struct tokenizer *Tokenizer)
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
ParseAssignmentOperator(struct tokenizer *Tokenizer)
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
ParseAssignmentExpression(struct tokenizer *Tokenizer)
{
        struct tokenizer Start = *Tokenizer;

        if(ParseConditionalExpression(Tokenizer))
        {
                return(true);
        }

        *Tokenizer = Start;
        if(ParseUnaryExpression(Tokenizer) &&
           ParseAssignmentOperator(Tokenizer) &&
           ParseAssignmentExpression(Tokenizer))
        {
                return(true);
        }

        *Tokenizer = Start;
        return(false);
}

bool
ParseExpressionI(struct tokenizer *Tokenizer)
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
ParseExpression(struct tokenizer *Tokenizer)
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
ParseIdentifier(struct tokenizer *Tokenizer)
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
ParseJumpStatement(struct tokenizer *Tokenizer)
{
        struct tokenizer Start = *Tokenizer;
        struct token Token = GetToken(Tokenizer);
        struct tokenizer AtToken = *Tokenizer;

        if(Token_Keyword == Token.Type &&
           IsStringEqual("goto", Token.Text, Token.TextLength) &&
           ParseIdentifier(Tokenizer) &&
           Token_SemiColon == GetToken(Tokenizer).Type)
        {
                return(true);
        }

        *Tokenizer = AtToken;
        if(Token_Keyword == Token.Type &&
           IsStringEqual("continue", Token.Text, Token.TextLength) &&
           Token_SemiColon == GetToken(Tokenizer).Type)
        {
                return(true);
        }

        *Tokenizer = AtToken;
        if(Token_Keyword == Token.Type &&
           IsStringEqual("break", Token.Text, Token.TextLength) &&
           Token_SemiColon == GetToken(Tokenizer).Type)
        {
                return(true);
        }

        *Tokenizer = AtToken;
        if(Token_Keyword == Token.Type &&
           IsStringEqual("return", Token.Text, Token.TextLength))
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
ParseIterationStatement(struct tokenizer *Tokenizer)
{
        struct tokenizer Start = *Tokenizer;
        struct token Token = GetToken(Tokenizer);
        struct tokenizer AtToken = *Tokenizer;

        if(Token_Keyword == Token.Type &&
           IsStringEqual("while", Token.Text, Token.TextLength) &&
           Token_OpenParen == GetToken(Tokenizer).Type &&
           ParseExpression(Tokenizer) &&
           Token_CloseParen == GetToken(Tokenizer).Type &&
           ParseStatement(Tokenizer))
        {
                return(true);
        }

        *Tokenizer = AtToken;
        if(Token_Keyword == Token.Type &&
           IsStringEqual("do", Token.Text, Token.TextLength) &&
           ParseStatement(Tokenizer))
        {
                struct token Token = GetToken(Tokenizer);
                if(Token_Keyword == Token.Type &&
                   IsStringEqual("while", Token.Text, Token.TextLength) &&
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
           IsStringEqual("for", Token.Text, Token.TextLength) &&
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
ParseSelectionStatement(struct tokenizer *Tokenizer)
{
        struct tokenizer Start = *Tokenizer;
        struct token Token = GetToken(Tokenizer);
        struct tokenizer AtToken = *Tokenizer;

        if(Token_Keyword == Token.Type &&
           IsStringEqual("if", Token.Text, Token.TextLength) &&
           Token_OpenParen == GetToken(Tokenizer).Type &&
           ParseExpression(Tokenizer) &&
           Token_CloseParen == GetToken(Tokenizer).Type &&
           ParseStatement(Tokenizer))
        {
                struct tokenizer AtElse = *Tokenizer;
                struct token Token = GetToken(Tokenizer);

                if(Token_Keyword == Token.Type &&
                   IsStringEqual("else", Token.Text, Token.TextLength) &&
                   ParseStatement(Tokenizer))
                {
                        return(true);
                }

                *Tokenizer = AtElse;
                return(true);
        }

        *Tokenizer = AtToken;
        if(Token_Keyword == Token.Type &&
           IsStringEqual("switch", Token.Text, Token.TextLength) &&
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
ParseStatementListI(struct tokenizer *Tokenizer)
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
ParseStatementList(struct tokenizer *Tokenizer)
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
ParseCompoundStatement(struct tokenizer *Tokenizer)
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
ParseExpressionStatement(struct tokenizer *Tokenizer)
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
ParseLabeledStatement(struct tokenizer *Tokenizer)
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
           IsStringEqual("case", Token.Text, Token.TextLength) &&
           ParseConstantExpression(Tokenizer) &&
           Token_Colon == GetToken(Tokenizer).Type &&
           ParseStatement(Tokenizer))
        {
                return(true);
        }

        *Tokenizer = AtToken;
        if(Token_Keyword == Token.Type &&
           IsStringEqual("default", Token.Text, Token.TextLength) &&
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
ParseStatement(struct tokenizer *Tokenizer)
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
ParseTypedefName(struct tokenizer *Tokenizer)
{
        struct tokenizer Start = *Tokenizer;

        if(ParseIdentifier(Tokenizer))
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
ParseDirectAbstractDeclaratorI(struct tokenizer *Tokenizer)
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
ParseDirectAbstractDeclarator(struct tokenizer *Tokenizer)
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
ParseAbstractDeclarator(struct tokenizer *Tokenizer)
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
ParseTypeName(struct tokenizer *Tokenizer)
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
ParseInitializerListI(struct tokenizer *Tokenizer)
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
ParseInitializerList(struct tokenizer *Tokenizer)
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
ParseInitializer(struct tokenizer *Tokenizer)
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
ParseIdentifierListI(struct tokenizer *Tokenizer)
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
ParseIdentifierList(struct tokenizer *Tokenizer)
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
ParseParameterDeclaration(struct tokenizer *Tokenizer)
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
ParseParameterListI(struct tokenizer *Tokenizer)
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
ParseParameterList(struct tokenizer *Tokenizer)
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
ParseParameterTypeList(struct tokenizer *Tokenizer)
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
ParseTypeQualifierListI(struct tokenizer *Tokenizer)
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
ParseTypeQualifierList(struct tokenizer *Tokenizer)
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
ParsePointer(struct tokenizer *Tokenizer)
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
ParseDirectDeclaratorI(struct tokenizer *Tokenizer)
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
ParseDirectDeclarator(struct tokenizer *Tokenizer)
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
ParseDeclarator(struct tokenizer *Tokenizer)
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
ParseEnumerator(struct tokenizer *Tokenizer)
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
ParseEnumeratorListI(struct tokenizer *Tokenizer)
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
ParseEnumeratorList(struct tokenizer *Tokenizer)
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
ParseEnumSpecifier(struct tokenizer *Tokenizer)
{
        struct tokenizer Start = *Tokenizer;
        struct token Token = GetToken(Tokenizer);
        struct tokenizer AtToken = *Tokenizer;

        if(!(Token_Keyword == Token.Type &&
             IsStringEqual("enum", Token.Text, Token.TextLength)))
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
ParseStructDeclarator(struct tokenizer *Tokenizer)
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
ParseStructDeclaratorListI(struct tokenizer *Tokenizer)
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
ParseStructDeclaratorList(struct tokenizer *Tokenizer)
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
ParseSpecifierQualifierList(struct tokenizer *Tokenizer)
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
ParseStructDeclaration(struct tokenizer *Tokenizer)
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
ParseInitDeclarator(struct tokenizer *Tokenizer)
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
ParseInitDeclaratorListI(struct tokenizer *Tokenizer)
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
ParseInitDeclaratorList(struct tokenizer *Tokenizer)
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
ParseStructDeclarationListI(struct tokenizer *Tokenizer)
{
        struct tokenizer Start = *Tokenizer;

        if(ParseStructDeclaration(Tokenizer) &&
           ParseStructDeclarationListI(Tokenizer))
        {
                return(false);
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
ParseStructDeclarationList(struct tokenizer *Tokenizer)
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
ParseStructOrUnion(struct tokenizer *Tokenizer)
{
        struct tokenizer Start = *Tokenizer;
        struct token Token = GetToken(Tokenizer);

        if(Token_Keyword == Token.Type)
        {
                if(IsStringEqual(Token.Text, "struct", Token.TextLength) ||
                   IsStringEqual(Token.Text, "union", Token.TextLength))
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
ParseStructOrUnionSpecifier(struct tokenizer *Tokenizer)
{
        struct tokenizer Start = *Tokenizer;

        if(ParseStructOrUnion(Tokenizer) &&
           ParseIdentifier(Tokenizer) &&
           Token_OpenBrace == GetToken(Tokenizer).Type &&
           ParseStructDeclarationList(Tokenizer) &&
           Token_CloseBrace == GetToken(Tokenizer).Type)
        {
                return(true);
        }

        *Tokenizer = Start;
        if(ParseStructOrUnion(Tokenizer) &&
           Token_OpenBrace == GetToken(Tokenizer).Type &&
           ParseStructDeclarationList(Tokenizer) &&
           Token_CloseBrace == GetToken(Tokenizer).Type)
        {
                return(true);
        }

        *Tokenizer = Start;
        if(ParseStructOrUnion(Tokenizer) &&
           ParseIdentifier(Tokenizer))
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
ParseTypeQualifier(struct tokenizer *Tokenizer)
{
        struct tokenizer Start = *Tokenizer;
        struct token Token = GetToken(Tokenizer);

        if(Token.Type == Token_Keyword)
        {
                if(IsStringEqual(Token.Text, "const", Token.TextLength) ||
                   IsStringEqual(Token.Text, "volatile", Token.TextLength))
                {
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
ParseTypeSpecifier(struct tokenizer *Tokenizer)
{
        struct tokenizer Start = *Tokenizer;
        char *Keywords[] = { "void", "char", "short", "int", "long", "float",
                             "double", "signed", "unsigned" };

        struct token Token = GetToken(Tokenizer);
        if(Token.Type == Token_Keyword)
        {
                for(int Index = 0; Index < ArraySize(Keywords); Index++)
                {
                        if(IsStringEqual(Token.Text, Keywords[Index], Token.TextLength))
                        {
                                return(true);
                        }
                }
        }

        *Tokenizer = Start;
        if(ParseStructOrUnionSpecifier(Tokenizer))
        {
                return(true);
        }

        *Tokenizer = Start;
        if(ParseEnumSpecifier(Tokenizer))
        {
                return(true);
        }

        *Tokenizer = Start;
        if(ParseTypedefName(Tokenizer))
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
ParseStorageClassSpecifier(struct tokenizer *Tokenizer)
{
        struct tokenizer Start = *Tokenizer;
        char *Keywords[] = { "auto", "register", "static", "extern", "typedef" };

        struct token Token = GetToken(Tokenizer);
        if(Token.Type == Token_Keyword)
        {
                for(int Index = 0; Index < ArraySize(Keywords); Index++)
                {
                        if(IsStringEqual(Token.Text, Keywords[Index], Token.TextLength))
                        {
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
ParseDeclarationSpecifiers(struct tokenizer *Tokenizer)
{
        struct tokenizer Start = *Tokenizer;

        if(ParseStorageClassSpecifier(Tokenizer) &&
           ParseDeclarationSpecifiers(Tokenizer))
        {
                return(true);
        }

        *Tokenizer = Start;
        if(ParseStorageClassSpecifier(Tokenizer))
        {
                return(true);
        }

        *Tokenizer = Start;
        if(ParseTypeSpecifier(Tokenizer) &&
           ParseDeclarationSpecifiers(Tokenizer))
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
           ParseDeclarationSpecifiers(Tokenizer))
        {
                return(true);
        }

        *Tokenizer = Start;
        if(ParseTypeQualifier(Tokenizer))
        {
                return(true);
        }

        ParseError("ParseDeclarationSpecifiers", Tokenizer);
        *Tokenizer = Start;
        return(false);
}

bool
ParseDeclarationListI(struct tokenizer *Tokenizer)
{
        struct tokenizer Start = *Tokenizer;

        if(ParseDeclaration(Tokenizer) &&
           ParseDeclarationListI(Tokenizer))
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
ParseDeclarationList(struct tokenizer *Tokenizer)
{
        struct tokenizer Start = *Tokenizer;

        if(ParseDeclaration(Tokenizer) &&
           ParseDeclarationListI(Tokenizer))
        {
                return(true);
        }

        ParseError("ParseDeclarationList", Tokenizer);
        *Tokenizer = Start;
        return(false);
}

/*
  declaration:
          declaration-specifiers init-declarator-list(opt) ;
*/
bool
ParseDeclaration(struct tokenizer *Tokenizer)
{
        struct tokenizer Start = *Tokenizer;

        if(ParseDeclarationSpecifiers(Tokenizer) &&
           ParseInitDeclaratorList(Tokenizer) &&
           Token_SemiColon == GetToken(Tokenizer).Type)
        {
                return(true);
        }

        *Tokenizer = Start;
        if(ParseDeclarationSpecifiers(Tokenizer) &&
           Token_SemiColon == GetToken(Tokenizer).Type)
        {
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
ParseFunctionDefinition(struct tokenizer *Tokenizer)
{
        struct tokenizer Start = *Tokenizer;

        if(ParseDeclarationSpecifiers(Tokenizer) &&
           ParseDeclarator(Tokenizer) &&
           ParseDeclarationList(Tokenizer) &&
           ParseCompoundStatement(Tokenizer))
        {
                return(true);
        }

        *Tokenizer = Start;
        if(ParseDeclarationSpecifiers(Tokenizer) &&
           ParseDeclarator(Tokenizer) &&
           ParseCompoundStatement(Tokenizer))
        {
                return(true);
        }

        *Tokenizer = Start;
        if(ParseDeclarator(Tokenizer) &&
           ParseDeclarationList(Tokenizer) &&
           ParseCompoundStatement(Tokenizer))
        {
                return(true);
        }

        *Tokenizer = Start;
        if(ParseDeclarator(Tokenizer) &&
           ParseCompoundStatement(Tokenizer))
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
ParseExternalDeclaration(struct tokenizer *Tokenizer)
{
        struct tokenizer Start = *Tokenizer;

        if(ParseFunctionDefinition(Tokenizer)) return(true);

        *Tokenizer = Start;
        if(ParseDeclaration(Tokenizer)) return(true);

        ParseError("ParseExternalDeclaration", Tokenizer);
        *Tokenizer = Start;
        return(false);
}

bool
ParseTranslationUnitI(struct tokenizer *Tokenizer)
{
        struct tokenizer Start = *Tokenizer;

        if(ParseExternalDeclaration(Tokenizer) &&
           ParseTranslationUnitI(Tokenizer))
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
ParseTranslationUnit(struct tokenizer *Tokenizer)
{
        struct tokenizer Start = *Tokenizer;

        if(ParseExternalDeclaration(Tokenizer) &&
           ParseTranslationUnitI(Tokenizer))
        {
                return(true);
        }

        ParseError("ParseTranslationUnit", Tokenizer);
        *Tokenizer = Start;
        return(false);
}

void
Parse(struct buffer *FileContents)
{
        struct tokenizer Tokenizer;
        Tokenizer.Beginning = Tokenizer.At = FileContents->Data;
        Tokenizer.Line = Tokenizer.Column = 1;

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
                                bool Result = ParseTranslationUnit(&Tokenizer);
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
