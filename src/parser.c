#ifndef _PARSER_C
#define _PARSER_C

#include <stdio.h>
#include "gs.h"
#include "lexer.c"
#include "tree.c"

void __Parser_ParseTreeUpdate(parse_tree_node *ParseTree, char *Name, unsigned int NumChildren) {
        ParseTreeSetName(ParseTree, Name);
        if (NumChildren > 0) {
                ParseTreeNewChildren(ParseTree, NumChildren);
        }
}

void __Parser_ParseTreeClearChildren(parse_tree_node *ParseTree) {
        for (int i = 0; i < ParseTree->NumChildren; i++) {
                ParseTreeSetName(&ParseTree->Children[i], "Empty");
        }
}

gs_bool ParseAssignmentExpression(struct tokenizer *Tokenizer, parse_tree_node *ParseTree);
gs_bool ParseExpression(struct tokenizer *Tokenizer, parse_tree_node *ParseTree);
gs_bool ParseTypeName(struct tokenizer *Tokenizer, parse_tree_node *ParseTree);
gs_bool ParseCastExpression(struct tokenizer *Tokenizer, parse_tree_node *ParseTree);
gs_bool ParseConditionalExpression(struct tokenizer *Tokenizer, parse_tree_node *ParseTree);
gs_bool ParseStatement(struct tokenizer *Tokenizer, parse_tree_node *ParseTree);
gs_bool ParseDeclarationList(struct tokenizer *Tokenizer, parse_tree_node *ParseTree);
gs_bool ParseParameterTypeList(struct tokenizer *Tokenizer, parse_tree_node *ParseTree);
gs_bool ParseAbstractDeclarator(struct tokenizer *Tokenizer, parse_tree_node *ParseTree);
gs_bool ParsePointer(struct tokenizer *Tokenizer, parse_tree_node *ParseTree);
gs_bool ParseSpecifierQualifierList(struct tokenizer *Tokenizer, parse_tree_node *ParseTree);
gs_bool ParseInitializer(struct tokenizer *Tokenizer, parse_tree_node *ParseTree);
gs_bool ParseDeclarationSpecifiers(struct tokenizer *Tokenizer, parse_tree_node *ParseTree);
gs_bool ParseDeclarator(struct tokenizer *Tokenizer, parse_tree_node *ParseTree);
gs_bool ParseTypeQualifier(struct tokenizer *Tokneizer, parse_tree_node *ParseTree);
gs_bool ParseTypeSpecifier(struct tokenizer *Tokenizer, parse_tree_node *ParseTree);
gs_bool ParseDeclaration(struct tokenizer *Tokenizer, parse_tree_node *ParseTree);

struct typedef_names {
        char *Name;
        int *NameIndex;
        int Capacity; /* Total allocated space for Name */
        int NumNames; /* Number of Name */
};
struct typedef_names TypedefNames;

void
TypedefClear() {
        free((void *)TypedefNames.Name);
        free((void *)TypedefNames.NameIndex);
        TypedefNames.Capacity = 0;
        TypedefNames.NumNames = 0;
}

void
TypedefInit() {
        char *Memory = (char *)malloc(1024);
        TypedefNames.Name = Memory;
        TypedefNames.Capacity = 1024;
        TypedefNames.NameIndex = malloc(1024 / 2 * sizeof(int));
        TypedefNames.NumNames = 0;
}

gs_bool TypedefIsName(struct token Token) {
        for(int i = 0; i < TypedefNames.NumNames; i++)
        {
                if(GSStringIsEqual(Token.Text, &TypedefNames.Name[TypedefNames.NameIndex[i]], Token.TextLength))
                {
                        return(true);
                }
        }
        return(false);
}

gs_bool TypedefAddName(char *Name) {
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
gs_bool ParseConstant(struct tokenizer *Tokenizer, parse_tree_node *ParseTree) {
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

gs_bool ParseArgumentExpressionListI(struct tokenizer *Tokenizer, parse_tree_node *ParseTree) {
        struct tokenizer Start = *Tokenizer;
        struct token Token;

        __Parser_ParseTreeUpdate(ParseTree, "Argument Expression List'", 3);

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
gs_bool ParseArgumentExpressionList(struct tokenizer *Tokenizer, parse_tree_node *ParseTree) {
        struct tokenizer Start = *Tokenizer;

        __Parser_ParseTreeUpdate(ParseTree, "Argument Expresion List", 2);

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
gs_bool ParsePrimaryExpression(struct tokenizer *Tokenizer, parse_tree_node *ParseTree) {
        struct tokenizer Start = *Tokenizer;
        struct token Tokens[2];

        __Parser_ParseTreeUpdate(ParseTree, "Primary Expression", 3);

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

gs_bool ParsePostfixExpressionI(struct tokenizer *Tokenizer, parse_tree_node *ParseTree) {
        struct tokenizer Start = *Tokenizer;
        struct token Tokens[2];

        __Parser_ParseTreeUpdate(ParseTree, "Postfix Expression'", 4);

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

        __Parser_ParseTreeClearChildren(ParseTree);

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

        __Parser_ParseTreeClearChildren(ParseTree);

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
gs_bool ParsePostfixExpression(struct tokenizer *Tokenizer, parse_tree_node *ParseTree) {
        struct tokenizer Start = *Tokenizer;

        __Parser_ParseTreeUpdate(ParseTree, "Postfix Expression", 2);

        if(ParsePrimaryExpression(Tokenizer, &ParseTree->Children[0]) &&
           ParsePostfixExpressionI(Tokenizer, &ParseTree->Children[1]))
        {
                return(true);
        }

        *Tokenizer = Start;
        return(false);
}

gs_bool ParseUnaryOperator(struct tokenizer *Tokenizer, parse_tree_node *ParseTree) {
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
gs_bool ParseUnaryExpression(struct tokenizer *Tokenizer, parse_tree_node *ParseTree) {
        struct tokenizer Start = *Tokenizer;
        struct token Tokens[2];

        __Parser_ParseTreeUpdate(ParseTree, "Unary Expression", 4);

        if(ParsePostfixExpression(Tokenizer, &ParseTree->Children[0]))
        {
                return(true);
        }

        __Parser_ParseTreeClearChildren(ParseTree);

        *Tokenizer = Start;
        if(Token_PlusPlus == (Tokens[0] = GetToken(Tokenizer)).Type &&
           ParseUnaryExpression(Tokenizer, &ParseTree->Children[1]))
        {
                ParseTreeSet(&ParseTree->Children[0], "Symbol", Tokens[0]);
                return(true);
        }

        __Parser_ParseTreeClearChildren(ParseTree);

        *Tokenizer = Start;
        if(Token_MinusMinus == (Tokens[0] = GetToken(Tokenizer)).Type &&
           ParseUnaryExpression(Tokenizer, &ParseTree->Children[1]))
        {
                ParseTreeSet(&ParseTree->Children[0], "Symbol", Tokens[0]);
                return(true);
        }

        __Parser_ParseTreeClearChildren(ParseTree);

        *Tokenizer = Start;
        if(ParseUnaryOperator(Tokenizer, &ParseTree->Children[0]) &&
           ParseCastExpression(Tokenizer, &ParseTree->Children[1]))
        {
                return(true);
        }

        __Parser_ParseTreeClearChildren(ParseTree);

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
gs_bool ParseCastExpression(struct tokenizer *Tokenizer, parse_tree_node *ParseTree) {
        struct tokenizer Start = *Tokenizer;
        struct token Tokens[2];

        __Parser_ParseTreeUpdate(ParseTree, "Cast Expression", 4);

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

gs_bool ParseMultiplicativeExpressionI(struct tokenizer *Tokenizer, parse_tree_node *ParseTree) {
        struct tokenizer Start = *Tokenizer;
        struct token Token;

        __Parser_ParseTreeUpdate(ParseTree, "Multiplicative Expression", 3);

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
gs_bool ParseMultiplicativeExpression(struct tokenizer *Tokenizer, parse_tree_node *ParseTree) {
        struct tokenizer Start = *Tokenizer;

        __Parser_ParseTreeUpdate(ParseTree, "Multiplicative Expression", 2);

        if(ParseCastExpression(Tokenizer, &ParseTree->Children[0]) &&
           ParseMultiplicativeExpressionI(Tokenizer, &ParseTree->Children[1]))
        {
                return(true);
        }

        *Tokenizer = Start;
        return(false);
}

gs_bool ParseAdditiveExpressionI(struct tokenizer *Tokenizer, parse_tree_node *ParseTree) {
        struct tokenizer Start = *Tokenizer;
        struct token Token;

        __Parser_ParseTreeUpdate(ParseTree, "Additive Expression'", 3);

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
gs_bool ParseAdditiveExpression(struct tokenizer *Tokenizer, parse_tree_node *ParseTree) {
        struct tokenizer Start = *Tokenizer;

        __Parser_ParseTreeUpdate(ParseTree, "Additive Expression", 2);

        if(ParseMultiplicativeExpression(Tokenizer, &ParseTree->Children[0]) &&
           ParseAdditiveExpressionI(Tokenizer, &ParseTree->Children[1]))
        {
                return(true);
        }

        *Tokenizer = Start;
        return(false);
}

gs_bool ParseShiftExpressionI(struct tokenizer *Tokenizer, parse_tree_node *ParseTree) {
        struct tokenizer Start = *Tokenizer;
        struct token Token;

        __Parser_ParseTreeUpdate(ParseTree, "Shift Expression'", 3);

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
gs_bool ParseShiftExpression(struct tokenizer *Tokenizer, parse_tree_node *ParseTree) {
        struct tokenizer Start = *Tokenizer;

        __Parser_ParseTreeUpdate(ParseTree, "Shift Expression", 2);

        if(ParseAdditiveExpression(Tokenizer, &ParseTree->Children[0]) &&
           ParseShiftExpressionI(Tokenizer, &ParseTree->Children[1]))
        {
                return(true);
        }

        *Tokenizer = Start;
        return(false);
}

gs_bool ParseRelationalExpressionI(struct tokenizer *Tokenizer, parse_tree_node *ParseTree) {
        struct tokenizer Start = *Tokenizer;
        struct token Token;

        __Parser_ParseTreeUpdate(ParseTree, "Relational Expression'", 3);

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
gs_bool ParseRelationalExpression(struct tokenizer *Tokenizer, parse_tree_node *ParseTree) {
        struct tokenizer Start = *Tokenizer;

        __Parser_ParseTreeUpdate(ParseTree, "Relational Expressoin", 2);

        if(ParseShiftExpression(Tokenizer, &ParseTree->Children[0]) &&
           ParseRelationalExpressionI(Tokenizer, &ParseTree->Children[1]))
        {
                return(true);
        }

        *Tokenizer = Start;
        return(false);
}

gs_bool ParseEqualityExpressionI(struct tokenizer *Tokenizer, parse_tree_node *ParseTree) {
        struct tokenizer Start = *Tokenizer;
        struct token Token;

        __Parser_ParseTreeUpdate(ParseTree, "Equality Expression'", 3);

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
gs_bool ParseEqualityExpression(struct tokenizer *Tokenizer, parse_tree_node *ParseTree) {
        struct tokenizer Start = *Tokenizer;

        __Parser_ParseTreeUpdate(ParseTree, "Equality Expression", 2);

        if(ParseRelationalExpression(Tokenizer, &ParseTree->Children[0]) &&
           ParseEqualityExpressionI(Tokenizer, &ParseTree->Children[1]))
        {
                return(true);
        }

        *Tokenizer = Start;
        return(false);
}

gs_bool ParseAndExpressionI(struct tokenizer *Tokenizer, parse_tree_node *ParseTree) {
        struct tokenizer Start = *Tokenizer;
        struct token Token;

        __Parser_ParseTreeUpdate(ParseTree, "And Expression'", 3);

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
gs_bool ParseAndExpression(struct tokenizer *Tokenizer, parse_tree_node *ParseTree) {
        struct tokenizer Start = *Tokenizer;

        __Parser_ParseTreeUpdate(ParseTree, "And Expression", 2);

        if(ParseEqualityExpression(Tokenizer, &ParseTree->Children[0]) &&
           ParseAndExpressionI(Tokenizer, &ParseTree->Children[1]))
        {
                return(true);
        }

        *Tokenizer = Start;
        return(false);
}

gs_bool ParseExclusiveOrExpressionI(struct tokenizer *Tokenizer, parse_tree_node *ParseTree) {
        struct tokenizer Start = *Tokenizer;
        struct token Token;

        __Parser_ParseTreeUpdate(ParseTree, "Exclusive Or Expression'", 3);

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
gs_bool ParseExclusiveOrExpression(struct tokenizer *Tokenizer, parse_tree_node *ParseTree) {
        struct tokenizer Start = *Tokenizer;

        __Parser_ParseTreeUpdate(ParseTree, "Exclusive Or Expression", 2);

        if(ParseAndExpression(Tokenizer, &ParseTree->Children[0]) &&
           ParseExclusiveOrExpressionI(Tokenizer, &ParseTree->Children[1]))
        {
                return(true);
        }

        *Tokenizer = Start;
        return(false);
}

gs_bool ParseInclusiveOrExpressionI(struct tokenizer *Tokenizer, parse_tree_node *ParseTree) {
        struct tokenizer Start = *Tokenizer;
        struct token Token;

        __Parser_ParseTreeUpdate(ParseTree, "Inclusive Or Expression'", 3);

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
gs_bool ParseInclusiveOrExpression(struct tokenizer *Tokenizer, parse_tree_node *ParseTree) {
        struct tokenizer Start = *Tokenizer;

        __Parser_ParseTreeUpdate(ParseTree, "Inclusive Or Expression", 2);

        if(ParseExclusiveOrExpression(Tokenizer, &ParseTree->Children[0]) &&
           ParseInclusiveOrExpressionI(Tokenizer, &ParseTree->Children[1]))
        {
                return(true);
        }

        *Tokenizer = Start;
        return(false);
}

gs_bool ParseLogicalAndExpressionI(struct tokenizer *Tokenizer, parse_tree_node *ParseTree) {
        struct tokenizer Start = *Tokenizer;
        struct token Token;

        __Parser_ParseTreeUpdate(ParseTree, "Logical And Expression'", 3);

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
gs_bool ParseLogicalAndExpression(struct tokenizer *Tokenizer, parse_tree_node *ParseTree) {
        struct tokenizer Start = *Tokenizer;

        __Parser_ParseTreeUpdate(ParseTree, "Logical And Expression", 2);

        if(ParseInclusiveOrExpression(Tokenizer, &ParseTree->Children[0]) &&
           ParseLogicalAndExpressionI(Tokenizer, &ParseTree->Children[1]))
        {
                return(true);
        }

        *Tokenizer = Start;
        return(false);
}

gs_bool ParseLogicalOrExpressionI(struct tokenizer *Tokenizer, parse_tree_node *ParseTree) {
        struct tokenizer Start = *Tokenizer;
        struct token Token;

        __Parser_ParseTreeUpdate(ParseTree, "Logical Or Expression'", 3);

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
gs_bool ParseLogicalOrExpression(struct tokenizer *Tokenizer, parse_tree_node *ParseTree) {
        struct tokenizer Start = *Tokenizer;

        __Parser_ParseTreeUpdate(ParseTree, "Logical Or Expression", 2);

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
gs_bool ParseConstantExpression(struct tokenizer *Tokenizer, parse_tree_node *ParseTree) {
        struct tokenizer Start = *Tokenizer;

        __Parser_ParseTreeUpdate(ParseTree, "Constant Expression", 1);

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
gs_bool ParseConditionalExpression(struct tokenizer *Tokenizer, parse_tree_node *ParseTree) {
        struct tokenizer Start = *Tokenizer;
        struct token Tokens[2];

        __Parser_ParseTreeUpdate(ParseTree, "Conditional Expression", 5);

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

        __Parser_ParseTreeClearChildren(ParseTree);

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
gs_bool ParseAssignmentOperator(struct tokenizer *Tokenizer, parse_tree_node *ParseTree) {
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
gs_bool ParseAssignmentExpression(struct tokenizer *Tokenizer, parse_tree_node *ParseTree) {
        struct tokenizer Start = *Tokenizer;

        __Parser_ParseTreeUpdate(ParseTree, "Assignment Expression", 3);

        if(ParseUnaryExpression(Tokenizer, &ParseTree->Children[0]) &&
           ParseAssignmentOperator(Tokenizer, &ParseTree->Children[1]) &&
           ParseAssignmentExpression(Tokenizer, &ParseTree->Children[2]))
        {
                return(true);
        }

        __Parser_ParseTreeClearChildren(ParseTree);

        *Tokenizer = Start;
        if(ParseConditionalExpression(Tokenizer, &ParseTree->Children[0]))
        {
                return(true);
        }

        *Tokenizer = Start;
        return(false);
}

gs_bool ParseExpressionI(struct tokenizer *Tokenizer, parse_tree_node *ParseTree) {
        struct tokenizer Start = *Tokenizer;
        struct token Token;

        __Parser_ParseTreeUpdate(ParseTree, "Expression'", 3);

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
gs_bool ParseExpression(struct tokenizer *Tokenizer, parse_tree_node *ParseTree) {
        struct tokenizer Start = *Tokenizer;

        __Parser_ParseTreeUpdate(ParseTree, "Expression", 2);

        if(ParseAssignmentExpression(Tokenizer, &ParseTree->Children[0]) &&
           ParseExpressionI(Tokenizer, &ParseTree->Children[1]))
        {
                return(true);
        }

        *Tokenizer = Start;
        return(false);
}

gs_bool ParseIdentifier(struct tokenizer *Tokenizer, parse_tree_node *ParseTree) {
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
gs_bool ParseJumpStatement(struct tokenizer *Tokenizer, parse_tree_node *ParseTree) {
        struct tokenizer Start = *Tokenizer;
        struct token Tokens[2];
        Tokens[0] = GetToken(Tokenizer);
        struct tokenizer AtToken = *Tokenizer;

        __Parser_ParseTreeUpdate(ParseTree, "Jump Statement", 3);

        if(Token_Keyword == Tokens[0].Type &&
           GSStringIsEqual("goto", Tokens[0].Text, Tokens[0].TextLength) &&
           ParseIdentifier(Tokenizer, &ParseTree->Children[1]) &&
           Token_SemiColon == (Tokens[1] = GetToken(Tokenizer)).Type)
        {
                ParseTreeSet(&ParseTree->Children[0], "Keyword", Tokens[0]);
                ParseTreeSet(&ParseTree->Children[2], "Symbol", Tokens[1]);
                return(true);
        }

        __Parser_ParseTreeClearChildren(ParseTree);

        *Tokenizer = AtToken;
        if(Token_Keyword == Tokens[0].Type &&
           GSStringIsEqual("continue", Tokens[0].Text, Tokens[0].TextLength) &&
           Token_SemiColon == (Tokens[1] = GetToken(Tokenizer)).Type)
        {
                ParseTreeSet(&ParseTree->Children[0], "Keyword", Tokens[0]);
                ParseTreeSet(&ParseTree->Children[1], "Symbol", Tokens[1]);
                return(true);
        }

        __Parser_ParseTreeClearChildren(ParseTree);

        *Tokenizer = AtToken;
        if(Token_Keyword == Tokens[0].Type &&
           GSStringIsEqual("break", Tokens[0].Text, Tokens[0].TextLength) &&
           Token_SemiColon == (Tokens[1] = GetToken(Tokenizer)).Type)
        {
                ParseTreeSet(&ParseTree->Children[0], "Keyword", Tokens[0]);
                ParseTreeSet(&ParseTree->Children[1], "Symbol", Tokens[1]);
                return(true);
        }

        __Parser_ParseTreeClearChildren(ParseTree);

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
gs_bool ParseIterationStatement(struct tokenizer *Tokenizer, parse_tree_node *ParseTree) {
        struct tokenizer Start = *Tokenizer;
        struct token Tokens[5];
        Tokens[0] = GetToken(Tokenizer);
        struct tokenizer AtToken = *Tokenizer;

        __Parser_ParseTreeUpdate(ParseTree, "Iteration Statement", 10); /* TODO(AARON): Magic Number! */

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

        __Parser_ParseTreeClearChildren(ParseTree);

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

        __Parser_ParseTreeClearChildren(ParseTree);

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
gs_bool ParseSelectionStatement(struct tokenizer *Tokenizer, parse_tree_node *ParseTree) {
        struct tokenizer Start = *Tokenizer;
        struct token Tokens[3];
        Tokens[0] = GetToken(Tokenizer);
        struct tokenizer AtToken = *Tokenizer;

        __Parser_ParseTreeUpdate(ParseTree, "Selection Statement", 6);

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

        __Parser_ParseTreeClearChildren(ParseTree);

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

gs_bool ParseStatementListI(struct tokenizer *Tokenizer, parse_tree_node *ParseTree) {
        struct tokenizer Start = *Tokenizer;

        __Parser_ParseTreeUpdate(ParseTree, "Statement List'", 2);

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
gs_bool ParseStatementList(struct tokenizer *Tokenizer, parse_tree_node *ParseTree) {
        struct tokenizer Start = *Tokenizer;

        __Parser_ParseTreeUpdate(ParseTree, "Statement List", 2);

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
gs_bool ParseCompoundStatement(struct tokenizer *Tokenizer, parse_tree_node *ParseTree) {
        struct tokenizer Start = *Tokenizer;
        token Token;

        __Parser_ParseTreeUpdate(ParseTree, "Compound Statement", 3);

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
gs_bool ParseExpressionStatement(struct tokenizer *Tokenizer, parse_tree_node *ParseTree) {
        struct tokenizer Start = *Tokenizer;
        token Token;

        __Parser_ParseTreeUpdate(ParseTree, "Expression Statement", 2);

        if(ParseExpression(Tokenizer, &ParseTree->Children[0]) &&
           Token_SemiColon == (Token = GetToken(Tokenizer)).Type)
        {
                ParseTreeSet(&ParseTree->Children[1], "Symbol", Token);
                return(true);
        }

        __Parser_ParseTreeClearChildren(ParseTree);

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
gs_bool ParseLabeledStatement(struct tokenizer *Tokenizer, parse_tree_node *ParseTree) {
        struct tokenizer Start = *Tokenizer;
        token Tokens[2];

        __Parser_ParseTreeUpdate(ParseTree, "Labeled Statement", 4);

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

        __Parser_ParseTreeClearChildren(ParseTree);

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
gs_bool ParseStatement(struct tokenizer *Tokenizer, parse_tree_node *ParseTree) {
        struct tokenizer Start = *Tokenizer;

        __Parser_ParseTreeUpdate(ParseTree, "Statement", 1);
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
gs_bool ParseTypedefName(struct tokenizer *Tokenizer, parse_tree_node *ParseTree) {
        struct tokenizer Start = *Tokenizer;
        struct token Token = GetToken(Tokenizer);
        *Tokenizer = Start;

        __Parser_ParseTreeUpdate(ParseTree, "Typedef Name", 1);

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
gs_bool ParseDirectAbstractDeclaratorI(struct tokenizer *Tokenizer, parse_tree_node *ParseTree) {
        struct tokenizer Start = *Tokenizer;
        token Tokens[2];

        __Parser_ParseTreeUpdate(ParseTree, "Direct Abstract Declarator'", 4);

        if(Token_OpenBracket == (Tokens[0] = GetToken(Tokenizer)).Type &&
           ParseConstantExpression(Tokenizer, &ParseTree->Children[1]) &&
           Token_CloseBracket == (Tokens[1] = GetToken(Tokenizer)).Type &&
           ParseDirectAbstractDeclaratorI(Tokenizer, &ParseTree->Children[2]))
        {
                ParseTreeSet(&ParseTree->Children[0], "Symbol", Tokens[0]);
                ParseTreeSet(&ParseTree->Children[2], "Symbol", Tokens[1]);
                return(true);
        }

        __Parser_ParseTreeClearChildren(ParseTree);

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

        __Parser_ParseTreeClearChildren(ParseTree);

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
gs_bool ParseDirectAbstractDeclarator(struct tokenizer *Tokenizer, parse_tree_node *ParseTree) {
        struct tokenizer Start = *Tokenizer;
        token Tokens[2];

        __Parser_ParseTreeUpdate(ParseTree, "Direct Abstract Declarator", 4);

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

        __Parser_ParseTreeClearChildren(ParseTree);

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
gs_bool ParseAbstractDeclarator(struct tokenizer *Tokenizer, parse_tree_node *ParseTree) {
        struct tokenizer Start = *Tokenizer;

        __Parser_ParseTreeUpdate(ParseTree, "Abstract Declarator", 2);

        if(ParsePointer(Tokenizer, &ParseTree->Children[0]) &&
           ParseDirectAbstractDeclarator(Tokenizer, &ParseTree->Children[1]))
        {
                return(true);
        }

        __Parser_ParseTreeClearChildren(ParseTree);

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
gs_bool ParseTypeName(struct tokenizer *Tokenizer, parse_tree_node *ParseTree) {
        struct tokenizer Start = *Tokenizer;

        __Parser_ParseTreeUpdate(ParseTree, "Type Name", 2);

        if(ParseSpecifierQualifierList(Tokenizer, &ParseTree->Children[0]) &&
           ParseAbstractDeclarator(Tokenizer, &ParseTree->Children[1]))
        {
                return(true);
        }

        __Parser_ParseTreeClearChildren(ParseTree);

        *Tokenizer = Start;
        if(ParseSpecifierQualifierList(Tokenizer, &ParseTree->Children[0]))
        {
                return(true);
        }

        *Tokenizer = Start;
        return(false);
}

gs_bool ParseInitializerListI(struct tokenizer *Tokenizer, parse_tree_node *ParseTree) {
        struct tokenizer Start = *Tokenizer;
        token Token;

        __Parser_ParseTreeUpdate(ParseTree, "Initializer List'", 3);

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
gs_bool ParseInitializerList(struct tokenizer *Tokenizer, parse_tree_node *ParseTree) {
        struct tokenizer Start = *Tokenizer;

        __Parser_ParseTreeUpdate(ParseTree, "Initializer List", 2);

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
gs_bool ParseInitializer(struct tokenizer *Tokenizer, parse_tree_node *ParseTree) {
        struct tokenizer Start = *Tokenizer;
        token Tokens[3];

        __Parser_ParseTreeUpdate(ParseTree, "Initializer", 4);

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

gs_bool ParseIdentifierListI(struct tokenizer *Tokenizer, parse_tree_node *ParseTree) {
        struct tokenizer Start = *Tokenizer;
        token Token;

        __Parser_ParseTreeUpdate(ParseTree, "Identifier List'", 3);

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
gs_bool ParseIdentifierList(struct tokenizer *Tokenizer, parse_tree_node *ParseTree) {
        struct tokenizer Start = *Tokenizer;

        __Parser_ParseTreeUpdate(ParseTree, "Identifier List", 2);

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
gs_bool ParseParameterDeclaration(struct tokenizer *Tokenizer, parse_tree_node *ParseTree) {
        struct tokenizer Start = *Tokenizer;

        __Parser_ParseTreeUpdate(ParseTree, "Parameter Declaration", 2);

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

        __Parser_ParseTreeClearChildren(ParseTree);

        *Tokenizer = Start;
        if(ParseDeclarationSpecifiers(Tokenizer, &ParseTree->Children[0]))
        {
                return(true);
        }

        *Tokenizer = Start;
        return(false);
}

gs_bool ParseParameterListI(struct tokenizer *Tokenizer, parse_tree_node *ParseTree) {
        struct tokenizer Start = *Tokenizer;
        token Token;

        __Parser_ParseTreeUpdate(ParseTree, "Parameter List'", 3);

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
gs_bool ParseParameterList(struct tokenizer *Tokenizer, parse_tree_node *ParseTree) {
        struct tokenizer Start = *Tokenizer;

        __Parser_ParseTreeUpdate(ParseTree, "Parameter List", 2);

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
gs_bool ParseParameterTypeList(struct tokenizer *Tokenizer, parse_tree_node *ParseTree) {
        struct tokenizer Start = *Tokenizer;
        token Tokens[2];

        __Parser_ParseTreeUpdate(ParseTree, "Parameter Type List", 3);

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

gs_bool ParseTypeQualifierListI(struct tokenizer *Tokenizer, parse_tree_node *ParseTree) {
        struct tokenizer Start = *Tokenizer;

        __Parser_ParseTreeUpdate(ParseTree, "Type Qualifier List'", 2);

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
gs_bool ParseTypeQualifierList(struct tokenizer *Tokenizer, parse_tree_node *ParseTree) {
        struct tokenizer Start = *Tokenizer;

        __Parser_ParseTreeUpdate(ParseTree, "Type Qualifier List", 2);

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
gs_bool ParsePointer(struct tokenizer *Tokenizer, parse_tree_node *ParseTree) {
        struct tokenizer Start = *Tokenizer;
        struct token Token = GetToken(Tokenizer);
        struct tokenizer AtToken = *Tokenizer;

        __Parser_ParseTreeUpdate(ParseTree, "Pointer", 2);

        if(Token_Asterisk != Token.Type)
        {
                *Tokenizer = Start;
                return(false);
        }

        ParseTreeSet(ParseTree, "Pointer", Token);

        if(ParseTypeQualifierList(Tokenizer, &ParseTree->Children[0]) &&
           ParsePointer(Tokenizer, &ParseTree->Children[1]))
        {
                return(true);
        }

        __Parser_ParseTreeClearChildren(ParseTree);

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
gs_bool ParseDirectDeclaratorI(struct tokenizer *Tokenizer, parse_tree_node *ParseTree) {
        struct tokenizer Start = *Tokenizer;
        token Tokens[2];

        __Parser_ParseTreeUpdate(ParseTree, "Direct Declarator'", 4);

        if(Token_OpenBracket == (Tokens[0] = GetToken(Tokenizer)).Type &&
           ParseConstantExpression(Tokenizer, &ParseTree->Children[1]) &&
           Token_CloseBracket == (Tokens[1] = GetToken(Tokenizer)).Type &&
           ParseDirectDeclaratorI(Tokenizer, &ParseTree->Children[3]))
        {
                ParseTreeSet(&ParseTree->Children[0], "Symbol", Tokens[0]);
                ParseTreeSet(&ParseTree->Children[2], "Symbol", Tokens[1]);
                return(true);
        }

        __Parser_ParseTreeClearChildren(ParseTree);

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

        __Parser_ParseTreeClearChildren(ParseTree);

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
gs_bool ParseDirectDeclarator(struct tokenizer *Tokenizer, parse_tree_node *ParseTree) {
        struct tokenizer Start = *Tokenizer;
        token Tokens[2];

        __Parser_ParseTreeUpdate(ParseTree, "Direct Declarator", 4);

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
gs_bool ParseDeclarator(struct tokenizer *Tokenizer, parse_tree_node *ParseTree) {
        struct tokenizer Start = *Tokenizer;

        __Parser_ParseTreeUpdate(ParseTree, "Declarator", 2);

        if(ParsePointer(Tokenizer, &ParseTree->Children[0]) &&
           ParseDirectDeclarator(Tokenizer, &ParseTree->Children[1]))
        {
                return(true);
        }

        __Parser_ParseTreeClearChildren(ParseTree);

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
gs_bool ParseEnumerator(struct tokenizer *Tokenizer, parse_tree_node *ParseTree) {
        struct tokenizer Start = *Tokenizer;
        token Token;

        __Parser_ParseTreeUpdate(ParseTree, "Enumerator", 3);

        if(ParseIdentifier(Tokenizer, &ParseTree->Children[0]) &&
           Token_EqualSign == (Token = GetToken(Tokenizer)).Type &&
           ParseConstantExpression(Tokenizer, &ParseTree->Children[2]))
        {
                ParseTreeSet(&ParseTree->Children[1], "Symbol", Token);
                return(true);
        }

        __Parser_ParseTreeClearChildren(ParseTree);

        *Tokenizer = Start;
        if(ParseIdentifier(Tokenizer, &ParseTree->Children[0]))
        {
                return(true);
        }

        *Tokenizer = Start;
        return(false);
}

gs_bool ParseEnumeratorListI(struct tokenizer *Tokenizer, parse_tree_node *ParseTree) {
        struct tokenizer Start = *Tokenizer;
        token Token;

        __Parser_ParseTreeUpdate(ParseTree, "Enumerator List'", 3);

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
gs_bool ParseEnumeratorList(struct tokenizer *Tokenizer, parse_tree_node *ParseTree) {
        struct tokenizer Start = *Tokenizer;

        __Parser_ParseTreeUpdate(ParseTree, "Enumerator List", 2);

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
gs_bool ParseEnumSpecifier(struct tokenizer *Tokenizer, parse_tree_node *ParseTree) {
        struct tokenizer Start = *Tokenizer;
        struct token Token = GetToken(Tokenizer);
        struct tokenizer AtToken = *Tokenizer;
        struct token Tokens[2];

        __Parser_ParseTreeUpdate(ParseTree, "Enum Specifier", 5);

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


        __Parser_ParseTreeClearChildren(ParseTree);

        *Tokenizer = AtToken;
        if(Token_OpenBrace == (Tokens[0] = GetToken(Tokenizer)).Type &&
           ParseEnumeratorList(Tokenizer, &ParseTree->Children[2]) &&
           Token_CloseBrace == (Tokens[1] = GetToken(Tokenizer)).Type)
        {
                ParseTreeSet(&ParseTree->Children[1], "Symbol", Tokens[0]);
                ParseTreeSet(&ParseTree->Children[3], "Symbol", Tokens[1]);
                return(true);
        }

        __Parser_ParseTreeClearChildren(ParseTree);

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
gs_bool ParseStructDeclarator(struct tokenizer *Tokenizer, parse_tree_node *ParseTree) {
        struct tokenizer Start = *Tokenizer;
        token Token;

        __Parser_ParseTreeUpdate(ParseTree, "Struct Declarator", 3);

        if(ParseDeclarator(Tokenizer, &ParseTree->Children[0]) &&
           Token_Colon == (Token = GetToken(Tokenizer)).Type &&
           ParseConstantExpression(Tokenizer, &ParseTree->Children[2]))
        {
                ParseTreeSet(&ParseTree->Children[1], "Symbol", Token);
                return(true);
        }

        __Parser_ParseTreeClearChildren(ParseTree);

        *Tokenizer = Start;
        if(Token_Colon == (Token = GetToken(Tokenizer)).Type &&
           ParseConstantExpression(Tokenizer, &ParseTree->Children[1]))
        {
                ParseTreeSet(&ParseTree->Children[0], "Symbol", Token);
                return(true);
        }

        __Parser_ParseTreeClearChildren(ParseTree);

        *Tokenizer = Start;
        if(ParseDeclarator(Tokenizer, &ParseTree->Children[0]))
        {
                return(true);
        }

        *Tokenizer = Start;
        return(false);
}

gs_bool ParseStructDeclaratorListI(struct tokenizer *Tokenizer, parse_tree_node *ParseTree) {
        struct tokenizer Start = *Tokenizer;
        token Token;

        __Parser_ParseTreeUpdate(ParseTree, "Struct Declarator List'", 3);

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
gs_bool ParseStructDeclaratorList(struct tokenizer *Tokenizer, parse_tree_node *ParseTree) {
        struct tokenizer Start = *Tokenizer;

        __Parser_ParseTreeUpdate(ParseTree, "Struct Declarator List", 2);

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
gs_bool ParseSpecifierQualifierList(struct tokenizer *Tokenizer, parse_tree_node *ParseTree) {
        struct tokenizer Start = *Tokenizer;

        __Parser_ParseTreeUpdate(ParseTree, "Specifier Qualifier List", 2);

        if(ParseTypeSpecifier(Tokenizer, &ParseTree->Children[0]) &&
           ParseSpecifierQualifierList(Tokenizer, &ParseTree->Children[1]))
        {
                return(true);
        }

        __Parser_ParseTreeClearChildren(ParseTree);

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

        __Parser_ParseTreeClearChildren(ParseTree);

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
gs_bool ParseStructDeclaration(struct tokenizer *Tokenizer, parse_tree_node *ParseTree) {
        struct tokenizer Start = *Tokenizer;
        token Token;

        __Parser_ParseTreeUpdate(ParseTree, "Struct Declaration", 3);

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
gs_bool ParseInitDeclarator(struct tokenizer *Tokenizer, parse_tree_node *ParseTree) {
        struct tokenizer Start = *Tokenizer;
        token Token;

        __Parser_ParseTreeUpdate(ParseTree, "Init Declarator", 3);

        if(ParseDeclarator(Tokenizer, &ParseTree->Children[0]) &&
           Token_EqualSign == (Token = GetToken(Tokenizer)).Type &&
           ParseInitializer(Tokenizer, &ParseTree->Children[2]))
        {
                ParseTreeSet(&ParseTree->Children[1], "Symbol", Token);
                return(true);
        }

        __Parser_ParseTreeClearChildren(ParseTree);

        *Tokenizer = Start;
        if(ParseDeclarator(Tokenizer, &ParseTree->Children[0]))
        {
                return(true);
        }

        *Tokenizer = Start;
        return(false);
}

gs_bool ParseInitDeclaratorListI(struct tokenizer *Tokenizer, parse_tree_node *ParseTree) {
        struct tokenizer Start = *Tokenizer;
        token Token;

        __Parser_ParseTreeUpdate(ParseTree, "Init Declarator List'", 2);

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
gs_bool ParseInitDeclaratorList(struct tokenizer *Tokenizer, parse_tree_node *ParseTree) {
        struct tokenizer Start = *Tokenizer;

        __Parser_ParseTreeUpdate(ParseTree, "Init Declaration List", 2);

        if(ParseInitDeclarator(Tokenizer, &ParseTree->Children[0]) &&
           ParseInitDeclaratorListI(Tokenizer, &ParseTree->Children[1]))
        {
                return(true);
        }

        *Tokenizer = Start;
        return(false);
}

gs_bool ParseStructDeclarationListI(struct tokenizer *Tokenizer, parse_tree_node *ParseTree) {
        struct tokenizer Start = *Tokenizer;

        __Parser_ParseTreeUpdate(ParseTree, "Struct Declaration List'", 2);

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
gs_bool ParseStructDeclarationList(struct tokenizer *Tokenizer, parse_tree_node *ParseTree) {
        struct tokenizer Start = *Tokenizer;

        __Parser_ParseTreeUpdate(ParseTree, "Struct Declaration List", 2);

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
gs_bool ParseStructOrUnion(struct tokenizer *Tokenizer, parse_tree_node *ParseTree) {
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
gs_bool ParseStructOrUnionSpecifier(struct tokenizer *Tokenizer, parse_tree_node *ParseTree) {
        struct tokenizer Start = *Tokenizer;

        __Parser_ParseTreeUpdate(ParseTree, "Struct or Union Specifier", 5);

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

        __Parser_ParseTreeClearChildren(ParseTree);

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

        __Parser_ParseTreeClearChildren(ParseTree);

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
gs_bool ParseTypeQualifier(struct tokenizer *Tokenizer, parse_tree_node *ParseTree) {
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
gs_bool ParseTypeSpecifier(struct tokenizer *Tokenizer, parse_tree_node *ParseTree) {
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

        __Parser_ParseTreeUpdate(ParseTree, "Type Specifier", 1);

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
gs_bool ParseStorageClassSpecifier(struct tokenizer *Tokenizer, parse_tree_node *ParseTree) {
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
gs_bool ParseDeclarationSpecifiers(struct tokenizer *Tokenizer, parse_tree_node *ParseTree) {
        struct tokenizer Start = *Tokenizer;

        __Parser_ParseTreeUpdate(ParseTree, "Declaration Specifiers", 2);

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

        __Parser_ParseTreeClearChildren(ParseTree);

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

gs_bool ParseDeclarationListI(struct tokenizer *Tokenizer, parse_tree_node *ParseTree) {
        struct tokenizer Start = *Tokenizer;

        __Parser_ParseTreeUpdate(ParseTree, "Declaration List'", 2);

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
gs_bool ParseDeclarationList(struct tokenizer *Tokenizer, parse_tree_node *ParseTree) {
        struct tokenizer Start = *Tokenizer;

        __Parser_ParseTreeUpdate(ParseTree, "Declaration List", 2);

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
gs_bool ParseDeclaration(struct tokenizer *Tokenizer, parse_tree_node *ParseTree) {
        struct tokenizer Start = *Tokenizer;
        struct token Token;

        __Parser_ParseTreeUpdate(ParseTree, "Declaration", 3);

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
gs_bool ParseFunctionDefinition(struct tokenizer *Tokenizer, parse_tree_node *ParseTree) {
        struct tokenizer Start = *Tokenizer;

        __Parser_ParseTreeUpdate(ParseTree, "Function Definition", 4);

        if(ParseDeclarationSpecifiers(Tokenizer, &ParseTree->Children[0]) &&
           ParseDeclarator(Tokenizer, &ParseTree->Children[1]) &&
           ParseDeclarationList(Tokenizer, &ParseTree->Children[2]) &&
           ParseCompoundStatement(Tokenizer, &ParseTree->Children[3]))
        {
                return(true);
        }

        __Parser_ParseTreeClearChildren(ParseTree);

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

        __Parser_ParseTreeClearChildren(ParseTree);

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
gs_bool ParseExternalDeclaration(struct tokenizer *Tokenizer, parse_tree_node *ParseTree) {
        struct tokenizer Start = *Tokenizer;

        __Parser_ParseTreeUpdate(ParseTree, "External Declaration", 1);

        if(ParseFunctionDefinition(Tokenizer, &ParseTree->Children[0])) return(true);

        *Tokenizer = Start;
        if(ParseDeclaration(Tokenizer, &ParseTree->Children[0])) return(true);

        *Tokenizer = Start;
        return(false);
}

gs_bool ParseTranslationUnitI(struct tokenizer *Tokenizer, parse_tree_node *ParseTree) {
        struct tokenizer Start = *Tokenizer;

        __Parser_ParseTreeUpdate(ParseTree, "Translation Unit'", 2);

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
gs_bool ParseTranslationUnit(struct tokenizer *Tokenizer, parse_tree_node *ParseTree) {
        struct tokenizer Start = *Tokenizer;

        __Parser_ParseTreeUpdate(ParseTree, "Translation Unit", 2);

        if(ParseExternalDeclaration(Tokenizer, &ParseTree->Children[0]) &&
           ParseTranslationUnitI(Tokenizer, &ParseTree->Children[1]))
        {
                return(true);
        }

        *Tokenizer = Start;
        return(false);
}

void Parse(gs_buffer *FileContents, gs_bool ShowParseTree) {
        parse_tree_allocator Allocator;
        Allocator.Alloc = malloc;
        Allocator.Free = free;
        parse_tree_node *ParseTree = ParseTreeInit(Allocator);

        struct tokenizer Tokenizer;
        Tokenizer.Beginning = Tokenizer.At = FileContents->Start;
        Tokenizer.Line = Tokenizer.Column = 1;
        char *FileEnd = FileContents->Start + FileContents->Length - 1;

        TypedefInit(TypedefNames);

        gs_bool Parsing = true;
        while(Parsing)
        {
                struct tokenizer Start = Tokenizer;
                struct token Token = GetToken(&Tokenizer);
                struct tokenizer AfterToken = Tokenizer;
                Tokenizer = Start;

                switch(Token.Type)
                {
                        /* Done! */
                        case Token_EndOfStream:
                        {
                                Parsing = false;
                        } break;

                        /* Skip this input. */
                        case Token_PreprocessorCommand:
                        case Token_Comment:
                        case Token_Unknown:
                        {
                                Tokenizer = AfterToken;
                        } break;

                        /* Okay, let's parse! */
                        default:
                        {
                                gs_bool Result = ParseTranslationUnit(&Tokenizer, ParseTree);

                                if(Result && Tokenizer.At == FileEnd)
                                {

                                        puts("Successfully parsed input");
                                }
                                else
                                {
                                        puts("Input did not parse");
                                        if(!Result)
                                        {
                                                puts("Parsing failed");
                                        }
                                        else if(Tokenizer.At != FileEnd)
                                        {
                                                puts("Parsing terminated early");
                                                printf("Tokenizer->At(%p), File End(%p)\n", Tokenizer.At, FileEnd);
                                        }
                                }

                                if(ShowParseTree)
                                {
                                        puts("--------------------------------------------------------------------------------");
                                        ParseTreePrint(ParseTree, 0, 2);
                                        puts("--------------------------------------------------------------------------------");
                                }
                                Parsing = false;
                        } break;
                }
        }

        ParseTreeDeinit(ParseTree);
}

#endif /* _PARSER_C */
