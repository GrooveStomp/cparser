/******************************************************************************
 * File: parser.c
 * Created: 2016-07-06
 * Updated: 2020-11-17
 * Package: C-Parser
 * Creator: Aaron Oman (GrooveStomp)
 * Homepage: https://git.sr.ht/~groovestomp/c-parser
 * Copyright 2016 - 2020, Aaron Oman and the C-Parser contributors
 * SPDX-License-Identifier: LGPL-3.0-only
 ******************************************************************************/

/******************************************************************************
 * This is a top-down recursive descent parser for the C language.
 * It cannot handle left-recursion, so left-recursive rules are
 * modified via substitution.
 * An example reduction:
 *     A  → B | AB
 *     A  → BA'
 *     A' → B | ε
 ******************************************************************************/

#ifndef PARSER_C
#define PARSER_C

#include "gs.h"
#include "lexer.c"
#include "parse_tree.c"

gs_Allocator __parser_allocator;

void __parser_ParseTreeClearChildren(ParseTreeNode *node) {
        gs_TreeNode *tree_node = &node->tree;
        while (tree_node->sibling != GS_NULL_PTR) {
                ParseTreeNode *container = gs_TreeContainer(tree_node, ParseTreeNode, tree);
                container->type = ParseTreeNode_Unknown;
                tree_node = tree_node->sibling;
        }
}

bool ParseAssignmentExpression(Tokenizer *tokenizer, ParseTreeNode *parse_tree);
bool ParseExpression(Tokenizer *tokenizer, ParseTreeNode *parse_tree);
bool ParseTypeName(Tokenizer *tokenizer, ParseTreeNode *parse_tree);
bool ParseCastExpression(Tokenizer *tokenizer, ParseTreeNode *parse_tree);
bool ParseConditionalExpression(Tokenizer *tokenizer, ParseTreeNode *parse_tree);
bool ParseStatement(Tokenizer *tokenizer, ParseTreeNode *parse_tree);
bool ParseDeclarationList(Tokenizer *tokenizer, ParseTreeNode *parse_tree);
bool ParseParameterTypeList(Tokenizer *tokenizer, ParseTreeNode *parse_tree);
bool ParseAbstractDeclarator(Tokenizer *tokenizer, ParseTreeNode *parse_tree);
bool ParsePointer(Tokenizer *tokenizer, ParseTreeNode *parse_tree);
bool ParseSpecifierQualifierList(Tokenizer *tokenizer, ParseTreeNode *parse_tree);
bool ParseInitializer(Tokenizer *tokenizer, ParseTreeNode *parse_tree);
bool ParseDeclarationSpecifiers(Tokenizer *tokenizer, ParseTreeNode *parse_tree);
bool ParseDeclarator(Tokenizer *tokenizer, ParseTreeNode *parse_tree);
bool ParseTypeQualifier(Tokenizer *Tokneizer, ParseTreeNode *parse_tree);
bool ParseTypeSpecifier(Tokenizer *tokenizer, ParseTreeNode *parse_tree);
bool ParseDeclaration(Tokenizer *tokenizer, ParseTreeNode *parse_tree);

typedef struct TypedefNames {
        char *name;
        int *name_index;
        int capacity; /* Total allocated space for Name */
        int num_names; /* Number of Name */
} TypedefNames;

static TypedefNames __parser_typedef_names;

void TypedefClear() {
        __parser_allocator.free((void *)__parser_typedef_names.name);
        __parser_allocator.free((void *)__parser_typedef_names.name_index);
        __parser_typedef_names.capacity = 0;
        __parser_typedef_names.num_names = 0;
}

void TypedefInit() {
        char *Memory = (char *)__parser_allocator.malloc(1024);
        __parser_typedef_names.name = Memory;
        __parser_typedef_names.capacity = 1024;
        __parser_typedef_names.name_index = __parser_allocator.malloc(1024 / 2 * sizeof(int));
        __parser_typedef_names.num_names = 0;
}

bool TypedefIsName(Token token) {
        for (int i = 0; i < __parser_typedef_names.num_names; i++) {
                if (gs_StringIsEqual(token.text, &__parser_typedef_names.name[__parser_typedef_names.name_index[i]], token.text_length)) {
                        return true;
                }
        }
        return false;
}

bool TypedefAddName(char *name) {
        if (__parser_typedef_names.num_names == 0) {
                gs_StringCopy(name, __parser_typedef_names.name, gs_StringLength(name));
                __parser_typedef_names.num_names++;
                return true;
        }

        int current_name_index = __parser_typedef_names.name_index[__parser_typedef_names.num_names - 1];
        /* NameLength doesn't account for trailing NULL */
        int name_length = gs_StringLength(&__parser_typedef_names.name[current_name_index]);
        int used_space = &__parser_typedef_names.name[current_name_index] - __parser_typedef_names.name + name_length + 1;
        int remaining_capacity = __parser_typedef_names.capacity - used_space;

        int new_name_length = gs_StringLength(name);
        if (new_name_length + 1 > remaining_capacity) {
                return false;
        }

        gs_StringCopy(name, &__parser_typedef_names.name[current_name_index] + name_length + 1, gs_StringLength(name));
        __parser_typedef_names.num_names++;
        return true;
}

/*
  constant:
  integer-constant
  character-constant
  floating-constant
  enumeration-constant
*/
bool ParseConstant(Tokenizer *tokenizer, ParseTreeNode *parse_tree) {
        Tokenizer start = *tokenizer;
        Token token = GetToken(tokenizer);

        switch (token.type) {
                case Token_Integer:
                case Token_Character:
                case Token_PrecisionNumber:
/*      TODO:   case Token_Enumeration:*/
                {
                        ParseTreeSet(parse_tree, ParseTreeNode_Constant, token);
                        return true;
                } break;
                default:
                {
                        *tokenizer = start;
                        return false;
                }
        }
}

bool ParseArgumentExpressionListI(Tokenizer *tokenizer, ParseTreeNode *parse_tree) {
        Tokenizer start = *tokenizer;
        Token token;
        ParseTreeNode *child1, *child2, *child3;

        parse_tree->type = ParseTreeNode_ArgumentExpressionListI;
        child1 = ParseTreeAddChild(parse_tree);
        child2 = ParseTreeAddChild(parse_tree);
        child3 = ParseTreeAddChild(parse_tree);

        if (Token_Comma == (token = GetToken(tokenizer)).type &&
            ParseAssignmentExpression(tokenizer, child2) &&
            ParseArgumentExpressionListI(tokenizer, child3)) {
                ParseTreeSet(child1, ParseTreeNode_Symbol, token);
                return true;
        }

        ParseTreeRemoveAllChildren(parse_tree);
        *tokenizer = start;

        return true;
}

/*
  argument-expression-list:
  assignment-expression
  argument-expression-list , assignment-expression
*/
bool ParseArgumentExpressionList(Tokenizer *tokenizer, ParseTreeNode *parse_tree) {
        Tokenizer start = *tokenizer;
        ParseTreeNode *child1, *child2;

        parse_tree->type = ParseTreeNode_ArgumentExpressionList;
        child1 = ParseTreeAddChild(parse_tree);
        child2 = ParseTreeAddChild(parse_tree);

        if (ParseAssignmentExpression(tokenizer, child1) &&
            ParseArgumentExpressionListI(tokenizer, child2)) {
                return true;
        }

        ParseTreeRemoveAllChildren(parse_tree);
        *tokenizer = start;

        return false;
}

/*
  primary-expression:
  identifier
  constant
  string
  ( expression )
*/
bool ParsePrimaryExpression(Tokenizer *tokenizer, ParseTreeNode *parse_tree) {
        Tokenizer start = *tokenizer;
        Token tokens[2];
        ParseTreeNode *child1, *child2, *child3;

        parse_tree->type = ParseTreeNode_PrimaryExpression;
        child1 = ParseTreeAddChild(parse_tree);
        child2 = ParseTreeAddChild(parse_tree);
        child3 = ParseTreeAddChild(parse_tree);

        if (Token_Identifier == (tokens[0] = GetToken(tokenizer)).type) {
                ParseTreeSet(child1, ParseTreeNode_Identifier, tokens[0]);
                return true;
        }

        ParseTreeRemoveAllChildren(parse_tree);
        child1 = ParseTreeAddChild(parse_tree);
        *tokenizer = start;

        if (ParseConstant(tokenizer, child1)) return true;

        ParseTreeRemoveAllChildren(parse_tree);
        child1 = ParseTreeAddChild(parse_tree);
        *tokenizer = start;

        if (Token_String == (tokens[0] = GetToken(tokenizer)).type) {
                ParseTreeSet(child1, ParseTreeNode_String, tokens[0]);
                return true;
        }

        ParseTreeRemoveAllChildren(parse_tree);
        child1 = ParseTreeAddChild(parse_tree);
        child2 = ParseTreeAddChild(parse_tree);
        child3 = ParseTreeAddChild(parse_tree);
        *tokenizer = start;

        if (Token_OpenParen == (tokens[0] = GetToken(tokenizer)).type &&
            ParseExpression(tokenizer, child2) &&
            Token_CloseParen == (tokens[1] = GetToken(tokenizer)).type) {
                ParseTreeSet(child1, ParseTreeNode_Symbol, tokens[0]);
                ParseTreeSet(child3, ParseTreeNode_Symbol, tokens[1]);
                return true;
        }

        ParseTreeRemoveAllChildren(parse_tree);
        *tokenizer = start;

        return false;
}

bool ParsePostfixExpressionI(Tokenizer *tokenizer, ParseTreeNode *parse_tree) {
        Tokenizer start = *tokenizer;
        Token tokens[2];
        ParseTreeNode *child1, *child2, *child3, *child4;

        parse_tree->type = ParseTreeNode_PostfixExpressionI;
        child1 = ParseTreeAddChild(parse_tree);
        child2 = ParseTreeAddChild(parse_tree);
        child3 = ParseTreeAddChild(parse_tree);
        child4 = ParseTreeAddChild(parse_tree);

        if (Token_OpenBracket == (tokens[0] = GetToken(tokenizer)).type &&
            ParseExpression(tokenizer, child2) &&
            Token_CloseBracket == (tokens[0] = GetToken(tokenizer)).type &&
            ParsePostfixExpressionI(tokenizer, child4)) {
                ParseTreeSet(child1, ParseTreeNode_Symbol, tokens[0]);
                ParseTreeSet(child3, ParseTreeNode_Symbol, tokens[1]);
                return true;
        }

        ParseTreeRemoveAllChildren(parse_tree);
        child1 = ParseTreeAddChild(parse_tree);
        child2 = ParseTreeAddChild(parse_tree);
        child3 = ParseTreeAddChild(parse_tree);
        child4 = ParseTreeAddChild(parse_tree);
        *tokenizer = start;

        if (Token_OpenParen == (tokens[0] = GetToken(tokenizer)).type &&
            ParseArgumentExpressionList(tokenizer, child2) &&
            Token_CloseParen == (tokens[1] = GetToken(tokenizer)).type &&
            ParsePostfixExpressionI(tokenizer, child4)) {
                ParseTreeSet(child1, ParseTreeNode_Symbol, tokens[0]);
                ParseTreeSet(child3, ParseTreeNode_Symbol, tokens[1]);
                return true;
        }

        ParseTreeRemoveAllChildren(parse_tree);
        child1 = ParseTreeAddChild(parse_tree);
        child2 = ParseTreeAddChild(parse_tree);
        child3 = ParseTreeAddChild(parse_tree);
        child4 = ParseTreeAddChild(parse_tree);
        *tokenizer = start;

        if (Token_OpenParen == (tokens[0] = GetToken(tokenizer)).type &&
            Token_CloseParen == (tokens[1] = GetToken(tokenizer)).type &&
            ParsePostfixExpressionI(tokenizer, child3)) {
                ParseTreeSet(child1, ParseTreeNode_Symbol, tokens[0]);
                ParseTreeSet(child2, ParseTreeNode_Symbol, tokens[1]);
                return true;
        }

        ParseTreeRemoveAllChildren(parse_tree);
        child1 = ParseTreeAddChild(parse_tree);
        child2 = ParseTreeAddChild(parse_tree);
        child3 = ParseTreeAddChild(parse_tree);
        child4 = ParseTreeAddChild(parse_tree);
        *tokenizer = start;

        if (Token_Dot == (tokens[0] = GetToken(tokenizer)).type &&
            Token_Identifier == (tokens[1] = GetToken(tokenizer)).type &&
            ParsePostfixExpressionI(tokenizer, child3)) {
                ParseTreeSet(child1, ParseTreeNode_Symbol, tokens[0]);
                ParseTreeSet(child2, ParseTreeNode_Identifier, tokens[1]);
                return true;
        }

        ParseTreeRemoveAllChildren(parse_tree);
        child1 = ParseTreeAddChild(parse_tree);
        child2 = ParseTreeAddChild(parse_tree);
        child3 = ParseTreeAddChild(parse_tree);
        child4 = ParseTreeAddChild(parse_tree);
        *tokenizer = start;

        if (Token_Arrow == (tokens[0] = GetToken(tokenizer)).type &&
            Token_Identifier == (tokens[1] = GetToken(tokenizer)).type &&
            ParsePostfixExpressionI(tokenizer, child3)) {
                ParseTreeSet(child1, ParseTreeNode_Symbol, tokens[0]);
                ParseTreeSet(child2, ParseTreeNode_Identifier, tokens[1]);
                return true;
        }

        ParseTreeRemoveAllChildren(parse_tree);
        child1 = ParseTreeAddChild(parse_tree);
        child2 = ParseTreeAddChild(parse_tree);
        child3 = ParseTreeAddChild(parse_tree);
        child4 = ParseTreeAddChild(parse_tree);
        *tokenizer = start;

        if (Token_PlusPlus == (tokens[0] = GetToken(tokenizer)).type &&
            ParsePostfixExpressionI(tokenizer, child2)) {
                ParseTreeSet(child1, ParseTreeNode_Symbol, tokens[0]);
                return true;
        }

        ParseTreeRemoveAllChildren(parse_tree);
        child1 = ParseTreeAddChild(parse_tree);
        child2 = ParseTreeAddChild(parse_tree);
        child3 = ParseTreeAddChild(parse_tree);
        child4 = ParseTreeAddChild(parse_tree);
        *tokenizer = start;

        if (Token_MinusMinus == (tokens[0] = GetToken(tokenizer)).type &&
            ParsePostfixExpressionI(tokenizer, child2)) {
                ParseTreeSet(child1, ParseTreeNode_Symbol, tokens[0]);
                return true;
        }

        ParseTreeRemoveAllChildren(parse_tree);
        *tokenizer = start;

        return true;
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
bool ParsePostfixExpression(Tokenizer *tokenizer, ParseTreeNode *parse_tree) {
        Tokenizer start = *tokenizer;
        ParseTreeNode *child1, *child2;

        parse_tree->type = ParseTreeNode_PostfixExpression;
        child1 = ParseTreeAddChild(parse_tree);
        child2 = ParseTreeAddChild(parse_tree);

        if (ParsePrimaryExpression(tokenizer, child1) &&
            ParsePostfixExpressionI(tokenizer, child2)) {
                return true;
        }

        ParseTreeRemoveAllChildren(parse_tree);
        *tokenizer = start;

        return false;
}

bool ParseUnaryOperator(Tokenizer *tokenizer, ParseTreeNode *parse_tree) {
        Tokenizer start = *tokenizer;
        Token token = GetToken(tokenizer);
        switch (token.type) {
                case Token_Ampersand:
                case Token_Asterisk:
                case Token_Cross:
                case Token_Dash:
                case Token_Tilde:
                case Token_Bang:
                {
                        ParseTreeSet(parse_tree, ParseTreeNode_UnaryOperator, token);
                        return true;
                }
                default:
                {
                        *tokenizer = start;
                        return false;
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
bool ParseUnaryExpression(Tokenizer *tokenizer, ParseTreeNode *parse_tree) {
        Tokenizer start = *tokenizer;
        Token tokens[2];
        ParseTreeNode *child1, *child2, *child3, *child4;

        parse_tree->type = ParseTreeNode_UnaryExpression;
        child1 = ParseTreeAddChild(parse_tree);

        if (ParsePostfixExpression(tokenizer, child1)) return true;

        ParseTreeRemoveAllChildren(parse_tree);
        child1 = ParseTreeAddChild(parse_tree);
        child2 = ParseTreeAddChild(parse_tree);
        *tokenizer = start;

        if (Token_PlusPlus == (tokens[0] = GetToken(tokenizer)).type &&
            ParseUnaryExpression(tokenizer, child2)) {
                ParseTreeSet(child1, ParseTreeNode_Symbol, tokens[0]);
                return true;
        }

        ParseTreeRemoveAllChildren(parse_tree);
        child1 = ParseTreeAddChild(parse_tree);
        child2 = ParseTreeAddChild(parse_tree);
        *tokenizer = start;

        if (Token_MinusMinus == (tokens[0] = GetToken(tokenizer)).type &&
            ParseUnaryExpression(tokenizer, child2)) {
                ParseTreeSet(child1, ParseTreeNode_Symbol, tokens[0]);
                return true;
        }

        ParseTreeRemoveAllChildren(parse_tree);
        child1 = ParseTreeAddChild(parse_tree);
        child2 = ParseTreeAddChild(parse_tree);
        *tokenizer = start;

        if (ParseUnaryOperator(tokenizer, child1) &&
            ParseCastExpression(tokenizer, child2)) {
                return true;
        }

        ParseTreeRemoveAllChildren(parse_tree);
        child1 = ParseTreeAddChild(parse_tree);
        child2 = ParseTreeAddChild(parse_tree);
        child3 = ParseTreeAddChild(parse_tree);
        child4 = ParseTreeAddChild(parse_tree);
        *tokenizer = start;

        tokens[0] = GetToken(tokenizer);
        if (Token_Keyword == tokens[0].type &&
            gs_StringIsEqual("sizeof", tokens[0].text, gs_StringLength("sizeof"))) {
                ParseTreeSet(child1, ParseTreeNode_Keyword, tokens[0]);

                Tokenizer Previous = *tokenizer;
                if (ParseUnaryExpression(tokenizer, child2)) {
                        return true;
                }

                *tokenizer = Previous;
                if (Token_OpenParen == (tokens[0] = GetToken(tokenizer)).type &&
                    ParseTypeName(tokenizer, child3) &&
                    Token_CloseParen == (tokens[1] = GetToken(tokenizer)).type) {
                        ParseTreeSet(child2, ParseTreeNode_Symbol, tokens[0]);
                        ParseTreeSet(child4, ParseTreeNode_Symbol, tokens[1]);
                        return true;
                }
        }

        ParseTreeRemoveAllChildren(parse_tree);
        *tokenizer = start;
        return false;
}

/*
  cast-expression:
  unary-expression
  ( type-name ) cast-expression
*/
bool ParseCastExpression(Tokenizer *tokenizer, ParseTreeNode *parse_tree) {
        Tokenizer start = *tokenizer;
        Token tokens[2];
        ParseTreeNode *child1, *child2, *child3, *child4;

        parse_tree->type = ParseTreeNode_CastExpression;
        child1 = ParseTreeAddChild(parse_tree);

        if (ParseUnaryExpression(tokenizer, child1)) return true;

        ParseTreeRemoveAllChildren(parse_tree);
        child1 = ParseTreeAddChild(parse_tree);
        child2 = ParseTreeAddChild(parse_tree);
        child3 = ParseTreeAddChild(parse_tree);
        child4 = ParseTreeAddChild(parse_tree);
        *tokenizer = start;

        if (Token_OpenParen == (tokens[0] = GetToken(tokenizer)).type &&
            ParseTypeName(tokenizer, child2) &&
            Token_CloseParen == (tokens[1] = GetToken(tokenizer)).type &&
            ParseCastExpression(tokenizer, child4)) {
                ParseTreeSet(child1, ParseTreeNode_Symbol, tokens[0]);
                ParseTreeSet(child3, ParseTreeNode_Symbol, tokens[1]);
                return true;
        }

        ParseTreeRemoveAllChildren(parse_tree);
        *tokenizer = start;
        return false;
}

bool ParseMultiplicativeExpressionI(Tokenizer *tokenizer, ParseTreeNode *parse_tree) {
        Tokenizer start = *tokenizer;
        Token token;
        ParseTreeNode *child1, *child2, *child3;

        parse_tree->type = ParseTreeNode_MultiplicativeExpressionI;
        child1 = ParseTreeAddChild(parse_tree);
        child2 = ParseTreeAddChild(parse_tree);
        child3 = ParseTreeAddChild(parse_tree);

        if (Token_Asterisk == (token = GetToken(tokenizer)).type &&
            ParseCastExpression(tokenizer, child2) &&
            ParseMultiplicativeExpressionI(tokenizer, child3)) {
                ParseTreeSet(child1, ParseTreeNode_Symbol, token);
                return true;
        }

        ParseTreeRemoveAllChildren(parse_tree);
        child1 = ParseTreeAddChild(parse_tree);
        child2 = ParseTreeAddChild(parse_tree);
        child3 = ParseTreeAddChild(parse_tree);
        *tokenizer = start;

        if (Token_Slash == (token = GetToken(tokenizer)).type &&
            ParseCastExpression(tokenizer, child2) &&
            ParseMultiplicativeExpressionI(tokenizer, child3)) {
                ParseTreeSet(child1, ParseTreeNode_Symbol, token);
                return true;
        }

        ParseTreeRemoveAllChildren(parse_tree);
        child1 = ParseTreeAddChild(parse_tree);
        child2 = ParseTreeAddChild(parse_tree);
        child3 = ParseTreeAddChild(parse_tree);
        *tokenizer = start;

        if (Token_PercentSign == (token = GetToken(tokenizer)).type &&
            ParseCastExpression(tokenizer, child2) &&
            ParseMultiplicativeExpressionI(tokenizer, child3)) {
                ParseTreeSet(child1, ParseTreeNode_Symbol, token);
                return true;
        }

        ParseTreeRemoveAllChildren(parse_tree);
        *tokenizer = start;

        return true;
}

/*
  multiplicative-expression:
  cast-expression
  multiplicative-expression * cast-expression
  multiplicative-expression / cast-expression
  multiplicative-expression % cast-expression
*/
bool ParseMultiplicativeExpression(Tokenizer *tokenizer, ParseTreeNode *parse_tree) {
        Tokenizer start = *tokenizer;
        ParseTreeNode *child1, *child2;

        parse_tree->type = ParseTreeNode_MultiplicativeExpression;
        child1 = ParseTreeAddChild(parse_tree);
        child2 = ParseTreeAddChild(parse_tree);

        if (ParseCastExpression(tokenizer, child1) &&
            ParseMultiplicativeExpressionI(tokenizer, child2)) {
                return true;
        }

        ParseTreeRemoveAllChildren(parse_tree);
        *tokenizer = start;

        return false;
}

bool ParseAdditiveExpressionI(Tokenizer *tokenizer, ParseTreeNode *parse_tree) {
        Tokenizer start = *tokenizer;
        Token token;
        ParseTreeNode *child1, *child2, *child3;

        parse_tree->type = ParseTreeNode_AdditiveExpressionI;
        child1 = ParseTreeAddChild(parse_tree);
        child2 = ParseTreeAddChild(parse_tree);
        child3 = ParseTreeAddChild(parse_tree);

        if (Token_Cross == (token = GetToken(tokenizer)).type &&
            ParseMultiplicativeExpression(tokenizer, child2) &&
            ParseAdditiveExpressionI(tokenizer, child3)) {
                ParseTreeSet(child1, ParseTreeNode_Symbol, token);
                return true;
        }

        ParseTreeRemoveAllChildren(parse_tree);
        child1 = ParseTreeAddChild(parse_tree);
        child2 = ParseTreeAddChild(parse_tree);
        child3 = ParseTreeAddChild(parse_tree);
        *tokenizer = start;

        if (Token_Dash == (token = GetToken(tokenizer)).type &&
            ParseMultiplicativeExpression(tokenizer, child2) &&
            ParseAdditiveExpressionI(tokenizer, child3)) {
                ParseTreeSet(child1, ParseTreeNode_Symbol, token);
                return true;
        }

        ParseTreeRemoveAllChildren(parse_tree);
        *tokenizer = start;

        return true;
}

/*
  additive-expression:
  multiplicative-expression
  additive-expression + multiplicative-expression
  additive-expression - multiplicative-expression
*/
bool ParseAdditiveExpression(Tokenizer *tokenizer, ParseTreeNode *parse_tree) {
        Tokenizer start = *tokenizer;
        ParseTreeNode *child1, *child2;

        parse_tree->type = ParseTreeNode_AdditiveExpression;
        child1 = ParseTreeAddChild(parse_tree);
        child2 = ParseTreeAddChild(parse_tree);

        if (ParseMultiplicativeExpression(tokenizer, child1) &&
            ParseAdditiveExpressionI(tokenizer, child2)) {
                return true;
        }

        ParseTreeRemoveAllChildren(parse_tree);
        *tokenizer = start;

        return false;
}

bool ParseShiftExpressionI(Tokenizer *tokenizer, ParseTreeNode *parse_tree) {
        Tokenizer start = *tokenizer;
        Token token;
        ParseTreeNode *child1, *child2, *child3;

        parse_tree->type = ParseTreeNode_ShiftExpressionI;
        child1 = ParseTreeAddChild(parse_tree);
        child2 = ParseTreeAddChild(parse_tree);
        child3 = ParseTreeAddChild(parse_tree);

        if (Token_BitShiftLeft == (token = GetToken(tokenizer)).type &&
            ParseAdditiveExpression(tokenizer, child2) &&
            ParseShiftExpressionI(tokenizer, child3)) {
                ParseTreeSet(child1, ParseTreeNode_Symbol, token);
                return true;
        }

        ParseTreeRemoveAllChildren(parse_tree);
        child1 = ParseTreeAddChild(parse_tree);
        child2 = ParseTreeAddChild(parse_tree);
        child3 = ParseTreeAddChild(parse_tree);
        *tokenizer = start;

        if (Token_BitShiftRight == (token = GetToken(tokenizer)).type &&
            ParseAdditiveExpression(tokenizer, child2) &&
            ParseShiftExpressionI(tokenizer, child3)) {
                ParseTreeSet(child1, ParseTreeNode_Symbol, token);
                return true;
        }

        ParseTreeRemoveAllChildren(parse_tree);
        *tokenizer = start;

        return true;
}

/*
  shift-expression:
  additive-expression
  shift-expression << additive-expression
  shift-expression >> additive-expression
*/
bool ParseShiftExpression(Tokenizer *tokenizer, ParseTreeNode *parse_tree) {
        Tokenizer start = *tokenizer;
        ParseTreeNode *child1, *child2;

        parse_tree->type = ParseTreeNode_ShiftExpression;
        child1 = ParseTreeAddChild(parse_tree);
        child2 = ParseTreeAddChild(parse_tree);

        if (ParseAdditiveExpression(tokenizer, child1) &&
            ParseShiftExpressionI(tokenizer, child2)) {
                return true;
        }

        ParseTreeRemoveAllChildren(parse_tree);
        *tokenizer = start;
        return false;
}

bool ParseRelationalExpressionI(Tokenizer *tokenizer, ParseTreeNode *parse_tree) {
        Tokenizer start = *tokenizer;
        Token token;
        ParseTreeNode *child1, *child2, *child3;

        parse_tree->type = ParseTreeNode_RelationalExpressionI;
        child1 = ParseTreeAddChild(parse_tree);
        child2 = ParseTreeAddChild(parse_tree);
        child3 = ParseTreeAddChild(parse_tree);

        if (Token_LessThan == (token = GetToken(tokenizer)).type &&
            ParseShiftExpression(tokenizer, child2) &&
            ParseRelationalExpressionI(tokenizer, child3)) {
                ParseTreeSet(child1, ParseTreeNode_Symbol, token);
                return true;
        }

        ParseTreeRemoveAllChildren(parse_tree);
        child1 = ParseTreeAddChild(parse_tree);
        child2 = ParseTreeAddChild(parse_tree);
        child3 = ParseTreeAddChild(parse_tree);
        *tokenizer = start;

        if (Token_GreaterThan == (token = GetToken(tokenizer)).type &&
            ParseShiftExpression(tokenizer, child2) &&
            ParseRelationalExpressionI(tokenizer, child3)) {
                ParseTreeSet(child1, ParseTreeNode_Symbol, token);
                return true;
        }

        ParseTreeRemoveAllChildren(parse_tree);
        child1 = ParseTreeAddChild(parse_tree);
        child2 = ParseTreeAddChild(parse_tree);
        child3 = ParseTreeAddChild(parse_tree);
        *tokenizer = start;

        if (Token_LessThanEqual == (token = GetToken(tokenizer)).type &&
            ParseShiftExpression(tokenizer, child2) &&
            ParseRelationalExpressionI(tokenizer, child3)) {
                ParseTreeSet(child1, ParseTreeNode_Symbol, token);
                return true;
        }

        ParseTreeRemoveAllChildren(parse_tree);
        child1 = ParseTreeAddChild(parse_tree);
        child2 = ParseTreeAddChild(parse_tree);
        child3 = ParseTreeAddChild(parse_tree);
        *tokenizer = start;

        if (Token_GreaterThanEqual == (token = GetToken(tokenizer)).type &&
            ParseShiftExpression(tokenizer, child2) &&
            ParseRelationalExpressionI(tokenizer, child3)) {
                ParseTreeSet(child1, ParseTreeNode_Symbol, token);
                return true;
        }

        ParseTreeRemoveAllChildren(parse_tree);
        *tokenizer = start;

        return true;
}

/*
  relational-expression:
  shift-expression
  relational-expression < shift-expression
  relational-expression > shift-expression
  relational-expression <= shift-exression
  relational-expression >= shift-expression
*/
bool ParseRelationalExpression(Tokenizer *tokenizer, ParseTreeNode *parse_tree) {
        Tokenizer start = *tokenizer;
        ParseTreeNode *child1, *child2;

        parse_tree->type = ParseTreeNode_RelationalExpression;
        child1 = ParseTreeAddChild(parse_tree);
        child2 = ParseTreeAddChild(parse_tree);

        if (ParseShiftExpression(tokenizer, child1) &&
            ParseRelationalExpressionI(tokenizer, child2)) {
                return true;
        }

        ParseTreeRemoveAllChildren(parse_tree);
        *tokenizer = start;

        return false;
}

bool ParseEqualityExpressionI(Tokenizer *tokenizer, ParseTreeNode *parse_tree) {
        Tokenizer start = *tokenizer;
        Token token;
        ParseTreeNode *child1, *child2, *child3;

        parse_tree->type = ParseTreeNode_EqualityExpressionI;
        child1 = ParseTreeAddChild(parse_tree);
        child2 = ParseTreeAddChild(parse_tree);
        child3 = ParseTreeAddChild(parse_tree);

        if (Token_LogicalEqual == (token = GetToken(tokenizer)).type &&
            ParseRelationalExpression(tokenizer, child2) &&
            ParseEqualityExpressionI(tokenizer, child3)) {
                ParseTreeSet(child1, ParseTreeNode_Symbol, token);
                return true;
        }

        ParseTreeRemoveAllChildren(parse_tree);
        child1 = ParseTreeAddChild(parse_tree);
        child2 = ParseTreeAddChild(parse_tree);
        child3 = ParseTreeAddChild(parse_tree);
        *tokenizer = start;

        if (Token_NotEqual == (token = GetToken(tokenizer)).type &&
            ParseRelationalExpression(tokenizer, child2) &&
            ParseEqualityExpressionI(tokenizer, child3)) {
                ParseTreeSet(child1, ParseTreeNode_Symbol, token);
                return true;
        }

        ParseTreeRemoveAllChildren(parse_tree);
        *tokenizer = start;

        return true;
}

/*
  equality-expression:
  relational-expression
  equality-expression == relational-expression
  equality-expression != relational-expression
*/
bool ParseEqualityExpression(Tokenizer *tokenizer, ParseTreeNode *parse_tree) {
        Tokenizer start = *tokenizer;
        ParseTreeNode *child1, *child2;

        parse_tree->type = ParseTreeNode_EqualityExpression;
        child1 = ParseTreeAddChild(parse_tree);
        child2 = ParseTreeAddChild(parse_tree);

        if (ParseRelationalExpression(tokenizer, child1) &&
            ParseEqualityExpressionI(tokenizer, child2)) {
                return true;
        }

        ParseTreeRemoveAllChildren(parse_tree);
        *tokenizer = start;

        return false;
}

bool ParseAndExpressionI(Tokenizer *tokenizer, ParseTreeNode *parse_tree) {
        Tokenizer start = *tokenizer;
        Token token;
        ParseTreeNode *child1, *child2, *child3;

        parse_tree->type = ParseTreeNode_AndExpressionI;
        child1 = ParseTreeAddChild(parse_tree);
        child2 = ParseTreeAddChild(parse_tree);
        child3 = ParseTreeAddChild(parse_tree);

        if (Token_Ampersand == (token = GetToken(tokenizer)).type &&
            ParseEqualityExpression(tokenizer, child2) &&
            ParseAndExpressionI(tokenizer, child3)) {
                ParseTreeSet(child1, ParseTreeNode_Symbol, token);
                return true;
        }

        ParseTreeRemoveAllChildren(parse_tree);
        *tokenizer = start;

        return true;
}

/*
  AND-expression:
  equality-expression
  AND-expression & equality-expression
*/
bool ParseAndExpression(Tokenizer *tokenizer, ParseTreeNode *parse_tree) {
        Tokenizer start = *tokenizer;
        ParseTreeNode *child1, *child2;

        parse_tree->type = ParseTreeNode_AndExpression;
        child1 = ParseTreeAddChild(parse_tree);
        child2 = ParseTreeAddChild(parse_tree);

        if (ParseEqualityExpression(tokenizer, child1) &&
            ParseAndExpressionI(tokenizer, child2)) {
                return true;
        }

        ParseTreeRemoveAllChildren(parse_tree);
        *tokenizer = start;

        return false;
}

bool ParseExclusiveOrExpressionI(Tokenizer *tokenizer, ParseTreeNode *parse_tree) {
        Tokenizer start = *tokenizer;
        Token token;
        ParseTreeNode *child1, *child2, *child3;

        parse_tree->type = ParseTreeNode_ExclusiveOrExpressionI;
        child1 = ParseTreeAddChild(parse_tree);
        child2 = ParseTreeAddChild(parse_tree);
        child3 = ParseTreeAddChild(parse_tree);

        if (Token_Carat == (token = GetToken(tokenizer)).type &&
            ParseAndExpression(tokenizer, child2) &&
            ParseExclusiveOrExpressionI(tokenizer, child3)) {
                ParseTreeSet(child1, ParseTreeNode_Symbol, token);
                return true;
        }

        ParseTreeRemoveAllChildren(parse_tree);
        *tokenizer = start;

        return true;
}

/*
  exclusive-OR-expression
  AND-expression
  exclusive-OR-expression ^ AND-expression
*/
bool ParseExclusiveOrExpression(Tokenizer *tokenizer, ParseTreeNode *parse_tree) {
        Tokenizer start = *tokenizer;
        ParseTreeNode *child1, *child2;

        parse_tree->type = ParseTreeNode_ExclusiveOrExpression;
        child1 = ParseTreeAddChild(parse_tree);
        child2 = ParseTreeAddChild(parse_tree);

        if (ParseAndExpression(tokenizer, child1) &&
            ParseExclusiveOrExpressionI(tokenizer, child2)) {
                return true;
        }

        ParseTreeRemoveAllChildren(parse_tree);
        *tokenizer = start;

        return false;
}

bool ParseInclusiveOrExpressionI(Tokenizer *tokenizer, ParseTreeNode *parse_tree) {
        Tokenizer start = *tokenizer;
        Token token;
        ParseTreeNode *child1, *child2, *child3;

        parse_tree->type = ParseTreeNode_InclusiveOrExpressionI;
        child1 = ParseTreeAddChild(parse_tree);
        child2 = ParseTreeAddChild(parse_tree);
        child3 = ParseTreeAddChild(parse_tree);

        if (Token_Pipe == (token = GetToken(tokenizer)).type &&
            ParseExclusiveOrExpression(tokenizer, child2) &&
            ParseInclusiveOrExpressionI(tokenizer, child3)) {
                ParseTreeSet(child1, ParseTreeNode_Symbol, token);
                return true;
        }

        ParseTreeRemoveAllChildren(parse_tree);
        *tokenizer = start;

        return true;
}

/*
  inclusive-OR-expression:
  exclusive-OR-expression
  inclusive-OR-expression | exclusive-OR-expression
*/
bool ParseInclusiveOrExpression(Tokenizer *tokenizer, ParseTreeNode *parse_tree) {
        Tokenizer start = *tokenizer;
        ParseTreeNode *child1, *child2;

        parse_tree->type = ParseTreeNode_InclusiveOrExpression;
        child1 = ParseTreeAddChild(parse_tree);
        child2 = ParseTreeAddChild(parse_tree);

        if (ParseExclusiveOrExpression(tokenizer, child1) &&
            ParseInclusiveOrExpressionI(tokenizer, child2)) {
                return true;
        }

        ParseTreeRemoveAllChildren(parse_tree);
        *tokenizer = start;

        return false;
}

bool ParseLogicalAndExpressionI(Tokenizer *tokenizer, ParseTreeNode *parse_tree) {
        Tokenizer start = *tokenizer;
        Token token;
        ParseTreeNode *child1, *child2, *child3;

        parse_tree->type = ParseTreeNode_LogicalAndExpressionI;
        child1 = ParseTreeAddChild(parse_tree);
        child2 = ParseTreeAddChild(parse_tree);
        child3 = ParseTreeAddChild(parse_tree);

        if (Token_LogicalAnd == (token = GetToken(tokenizer)).type &&
            ParseInclusiveOrExpression(tokenizer, child2) &&
            ParseLogicalAndExpressionI(tokenizer, child3)) {
                ParseTreeSet(child1, ParseTreeNode_Symbol, token);
                return true;
        }

        ParseTreeRemoveAllChildren(parse_tree);
        *tokenizer = start;

        return true;
}

/*
  logical-AND-expression:
  inclusive-OR-expression
  logical-AND-expression && inclusive-OR-expression
*/
bool ParseLogicalAndExpression(Tokenizer *tokenizer, ParseTreeNode *parse_tree) {
        Tokenizer start = *tokenizer;
        ParseTreeNode *child1, *child2;

        parse_tree->type = ParseTreeNode_LogicalAndExpression;
        child1 = ParseTreeAddChild(parse_tree);
        child2 = ParseTreeAddChild(parse_tree);

        if (ParseInclusiveOrExpression(tokenizer, child1) &&
            ParseLogicalAndExpressionI(tokenizer, child2)) {
                return true;
        }

        ParseTreeRemoveAllChildren(parse_tree);
        *tokenizer = start;

        return false;
}

bool ParseLogicalOrExpressionI(Tokenizer *tokenizer, ParseTreeNode *parse_tree) {
        Tokenizer start = *tokenizer;
        Token token;
        ParseTreeNode *child1, *child2, *child3;

        parse_tree->type = ParseTreeNode_LogicalOrExpressionI;
        child1 = ParseTreeAddChild(parse_tree);
        child2 = ParseTreeAddChild(parse_tree);
        child3 = ParseTreeAddChild(parse_tree);

        if (Token_LogicalOr == (token = GetToken(tokenizer)).type &&
            ParseLogicalAndExpression(tokenizer, child2) &&
            ParseLogicalOrExpressionI(tokenizer, child3)) {
                ParseTreeSet(child1, ParseTreeNode_Symbol, token);
                return true;
        }

        ParseTreeRemoveAllChildren(parse_tree);
        *tokenizer = start;

        return true;
}

/*
  logical-OR-expression:
  logical-AND-expression
  logical-OR-expression || logical-AND-expression
*/
bool ParseLogicalOrExpression(Tokenizer *tokenizer, ParseTreeNode *parse_tree) {
        Tokenizer start = *tokenizer;
        ParseTreeNode *child1, *child2;

        parse_tree->type = ParseTreeNode_LogicalOrExpression;
        child1 = ParseTreeAddChild(parse_tree);
        child2 = ParseTreeAddChild(parse_tree);

        if (ParseLogicalAndExpression(tokenizer, child1) &&
            ParseLogicalOrExpressionI(tokenizer, child2)) {
                return true;
        }

        ParseTreeRemoveAllChildren(parse_tree);
        *tokenizer = start;

        return false;
}

/*
  constant-expression:
  conditional-expression
*/
bool ParseConstantExpression(Tokenizer *tokenizer, ParseTreeNode *parse_tree) {
        Tokenizer start = *tokenizer;
        ParseTreeNode *child1;

        parse_tree->type = ParseTreeNode_ConstantExpression;
        child1 = ParseTreeAddChild(parse_tree);

        if (ParseConditionalExpression(tokenizer, child1)) {
                return true;
        }

        ParseTreeRemoveAllChildren(parse_tree);
        *tokenizer = start;

        return false;
}

/*
  conditional-expression:
  logical-OR-expression
  logical-OR-expression ? expression : conditional-expression
*/
bool ParseConditionalExpression(Tokenizer *tokenizer, ParseTreeNode *parse_tree) {
        Tokenizer start = *tokenizer;
        Token tokens[2];
        ParseTreeNode *child1, *child2, *child3, *child4, *child5;

        parse_tree->type = ParseTreeNode_ConditionalExpression;
        child1 = ParseTreeAddChild(parse_tree);
        child2 = ParseTreeAddChild(parse_tree);
        child3 = ParseTreeAddChild(parse_tree);
        child4 = ParseTreeAddChild(parse_tree);
        child5 = ParseTreeAddChild(parse_tree);

        if (ParseLogicalOrExpression(tokenizer, child1) &&
            Token_QuestionMark == (tokens[0] = GetToken(tokenizer)).type &&
            ParseExpression(tokenizer, child3) &&
            Token_Colon == (tokens[1] = GetToken(tokenizer)).type &&
            ParseConditionalExpression(tokenizer, child5)) {
                ParseTreeSet(child2, ParseTreeNode_Symbol, tokens[0]);
                ParseTreeSet(child4, ParseTreeNode_Symbol, tokens[1]);
                return true;
        }

        ParseTreeRemoveAllChildren(parse_tree);
        child1 = ParseTreeAddChild(parse_tree);
        *tokenizer = start;

        if (ParseLogicalOrExpression(tokenizer, child1)) {
                return true;
        }

        ParseTreeRemoveAllChildren(parse_tree);
        *tokenizer = start;

        return false;
}

/*
  assignment-operator:
  one of: = *= /= %= += -= <<= >>= &= ^= |=
*/
bool ParseAssignmentOperator(Tokenizer *tokenizer, ParseTreeNode *parse_tree) {
        Tokenizer start = *tokenizer;
        Token token = GetToken(tokenizer);

        switch (token.type) {
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
                case Token_PipeEquals: {
                        ParseTreeSet(parse_tree, ParseTreeNode_AssignmentOperator, token);
                        return true;
                }
        }

        *tokenizer = start;

        return false;
}

/*
  assignment-expression:
  conditional-expression
  unary-expression assignment-operator assignment-expression
*/
bool ParseAssignmentExpression(Tokenizer *tokenizer, ParseTreeNode *parse_tree) {
        Tokenizer start = *tokenizer;
        ParseTreeNode *child1, *child2, *child3;

        parse_tree->type = ParseTreeNode_AssignmentExpression;
        child1 = ParseTreeAddChild(parse_tree);
        child2 = ParseTreeAddChild(parse_tree);
        child3 = ParseTreeAddChild(parse_tree);

        if (ParseUnaryExpression(tokenizer, child1) &&
            ParseAssignmentOperator(tokenizer, child2) &&
            ParseAssignmentExpression(tokenizer, child3)) {
                return true;
        }

        ParseTreeRemoveAllChildren(parse_tree);
        child1 = ParseTreeAddChild(parse_tree);
        *tokenizer = start;

        if (ParseConditionalExpression(tokenizer, child1)) {
                return true;
        }

        ParseTreeRemoveAllChildren(parse_tree);
        *tokenizer = start;

        return false;
}

bool ParseExpressionI(Tokenizer *tokenizer, ParseTreeNode *parse_tree) {
        Tokenizer start = *tokenizer;
        Token token;
        ParseTreeNode *child1, *child2, *child3;

        parse_tree->type = ParseTreeNode_ExpressionI;
        child1 = ParseTreeAddChild(parse_tree);
        child2 = ParseTreeAddChild(parse_tree);
        child3 = ParseTreeAddChild(parse_tree);

        if (Token_Comma == (token = GetToken(tokenizer)).type &&
            ParseAssignmentExpression(tokenizer, child2) &&
            ParseExpressionI(tokenizer, child3)) {
                ParseTreeSet(child1, ParseTreeNode_Symbol, token);
                return true;
        }

        ParseTreeRemoveAllChildren(parse_tree);
        *tokenizer = start;

        return true;
}

/*
  expression:
  assignment-expression
  expression , assignment-expression
*/
bool ParseExpression(Tokenizer *tokenizer, ParseTreeNode *parse_tree) {
        Tokenizer start = *tokenizer;
        ParseTreeNode *child1, *child2;

        parse_tree->type = ParseTreeNode_Expression;
        child1 = ParseTreeAddChild(parse_tree);
        child2 = ParseTreeAddChild(parse_tree);

        if (ParseAssignmentExpression(tokenizer, child1) &&
            ParseExpressionI(tokenizer, child2)) {
                return true;
        }

        ParseTreeRemoveAllChildren(parse_tree);
        *tokenizer = start;

        return false;
}

bool ParseIdentifier(Tokenizer *tokenizer, ParseTreeNode *parse_tree) {
        Tokenizer start = *tokenizer;
        Token token;

        if (Token_Identifier == (token = GetToken(tokenizer)).type) {
                ParseTreeSet(parse_tree, ParseTreeNode_Identifier, token);
                return true;
        }

        *tokenizer = start;
        return false;
}

/*
  jump-statement:
  goto identifier ;
  continue ;
  break ;
  return expression(opt) ;
*/
bool ParseJumpStatement(Tokenizer *tokenizer, ParseTreeNode *parse_tree) {
        Tokenizer start = *tokenizer;
        Token tokens[2];
        tokens[0] = GetToken(tokenizer);
        Tokenizer at_token = *tokenizer;
        ParseTreeNode *child1, *child2, *child3;

        parse_tree->type = ParseTreeNode_JumpStatement;
        child1 = ParseTreeAddChild(parse_tree);
        child2 = ParseTreeAddChild(parse_tree);
        child3 = ParseTreeAddChild(parse_tree);

        if (Token_Keyword == tokens[0].type &&
            gs_StringIsEqual("goto", tokens[0].text, tokens[0].text_length) &&
            ParseIdentifier(tokenizer, child2) &&
            Token_SemiColon == (tokens[1] = GetToken(tokenizer)).type) {
                ParseTreeSet(child1, ParseTreeNode_Keyword, tokens[0]);
                ParseTreeSet(child3, ParseTreeNode_Symbol, tokens[1]);
                return true;
        }

        ParseTreeRemoveAllChildren(parse_tree);
        child1 = ParseTreeAddChild(parse_tree);
        child2 = ParseTreeAddChild(parse_tree);
        child3 = ParseTreeAddChild(parse_tree);
        *tokenizer = at_token;

        if (Token_Keyword == tokens[0].type &&
            gs_StringIsEqual("continue", tokens[0].text, tokens[0].text_length) &&
            Token_SemiColon == (tokens[1] = GetToken(tokenizer)).type) {
                ParseTreeSet(child1, ParseTreeNode_Keyword, tokens[0]);
                ParseTreeSet(child2, ParseTreeNode_Symbol, tokens[1]);
                return true;
        }

        ParseTreeRemoveAllChildren(parse_tree);
        child1 = ParseTreeAddChild(parse_tree);
        child2 = ParseTreeAddChild(parse_tree);
        child3 = ParseTreeAddChild(parse_tree);
        *tokenizer = at_token;

        if (Token_Keyword == tokens[0].type &&
            gs_StringIsEqual("break", tokens[0].text, tokens[0].text_length) &&
            Token_SemiColon == (tokens[1] = GetToken(tokenizer)).type) {
                ParseTreeSet(child1, ParseTreeNode_Keyword, tokens[0]);
                ParseTreeSet(child2, ParseTreeNode_Symbol, tokens[1]);
                return true;
        }

        ParseTreeRemoveAllChildren(parse_tree);
        child1 = ParseTreeAddChild(parse_tree);
        child2 = ParseTreeAddChild(parse_tree);
        child3 = ParseTreeAddChild(parse_tree);
        *tokenizer = at_token;

        if (Token_Keyword == tokens[0].type &&
            gs_StringIsEqual("return", tokens[0].text, tokens[0].text_length)) {
                ParseTreeSet(child1, ParseTreeNode_Keyword, tokens[0]);
                int ChildIndex = 1;
                Tokenizer Previous = *tokenizer;

                ParseTreeNode *child = child3;
                if (!ParseExpression(tokenizer, child2)) {
                        child = child2;
                        *tokenizer = Previous;
                }

                if (Token_SemiColon == (tokens[1] = GetToken(tokenizer)).type) {
                        ParseTreeSet(child, ParseTreeNode_Symbol, tokens[1]);
                        return true;
                }
        }

        ParseTreeRemoveAllChildren(parse_tree);
        *tokenizer = start;

        return false;
}

/*
  iteration-statement:
  while ( expression ) statement
  do statement while ( expression) ;
  for ( expression(opt) ; expression(opt) ; expression(opt) ) statement
*/
bool ParseIterationStatement(Tokenizer *tokenizer, ParseTreeNode *parse_tree) {
        Tokenizer start = *tokenizer;
        Token tokens[5];
        tokens[0] = GetToken(tokenizer);
        Tokenizer at_token = *tokenizer;
        ParseTreeNode *child1, *child2, *child3, *child4, *child5;

        parse_tree->type = ParseTreeNode_IterationStatement;
        child1 = ParseTreeAddChild(parse_tree);
        child2 = ParseTreeAddChild(parse_tree);
        child3 = ParseTreeAddChild(parse_tree);
        child4 = ParseTreeAddChild(parse_tree);
        child5 = ParseTreeAddChild(parse_tree);

        /* TODO(AARON): Not sure how many child nodes to create! */
        ParseTreeAddChild(parse_tree);
        ParseTreeAddChild(parse_tree);
        ParseTreeAddChild(parse_tree);
        ParseTreeAddChild(parse_tree);
        ParseTreeAddChild(parse_tree);

        if (Token_Keyword == tokens[0].type &&
            gs_StringIsEqual("while", tokens[0].text, tokens[0].text_length) &&
            Token_OpenParen == (tokens[1] = GetToken(tokenizer)).type &&
            ParseExpression(tokenizer, child3) &&
            Token_CloseParen == (tokens[2] = GetToken(tokenizer)).type &&
            ParseStatement(tokenizer, child5)) {
                ParseTreeSet(child1, ParseTreeNode_Keyword, tokens[0]);
                ParseTreeSet(child2, ParseTreeNode_Symbol, tokens[1]);
                ParseTreeSet(child4, ParseTreeNode_Symbol, tokens[2]);
                return true;
        }

        ParseTreeRemoveAllChildren(parse_tree);
        child1 = ParseTreeAddChild(parse_tree);
        child2 = ParseTreeAddChild(parse_tree);
        child3 = ParseTreeAddChild(parse_tree);
        child4 = ParseTreeAddChild(parse_tree);
        child5 = ParseTreeAddChild(parse_tree);

        /* TODO(AARON): Not sure how many child nodes to create! */
        ParseTreeAddChild(parse_tree);
        ParseTreeAddChild(parse_tree);
        ParseTreeAddChild(parse_tree);
        ParseTreeAddChild(parse_tree);
        ParseTreeAddChild(parse_tree);

        *tokenizer = at_token;
        if (Token_Keyword == tokens[0].type &&
            gs_StringIsEqual("do", tokens[0].text, tokens[0].text_length) &&
            ParseStatement(tokenizer, child2)) {
                ParseTreeNode *child_node = child1;
                ParseTreeSet(child_node, ParseTreeNode_Keyword, tokens[0]);

                tokens[1] = GetToken(tokenizer);
                if (Token_Keyword == tokens[1].type &&
                    gs_StringIsEqual("while", tokens[1].text, tokens[1].text_length) &&
                    Token_OpenParen == (tokens[2] = GetToken(tokenizer)).type &&
                    ParseExpression(tokenizer, child5) &&
                    Token_CloseParen == (tokens[3] = GetToken(tokenizer)).type &&
                    Token_SemiColon == (tokens[4] = GetToken(tokenizer)).type) {
                        ParseTreeSet(++child_node, ParseTreeNode_Keyword, tokens[1]);
                        ParseTreeSet(++child_node, ParseTreeNode_Symbol, tokens[2]);
                        ++child_node; // Child #4 is set in the `if' above.
                        ParseTreeSet(++child_node, ParseTreeNode_Symbol, tokens[3]);
                        ParseTreeSet(++child_node, ParseTreeNode_Symbol, tokens[4]);
                        return true;
                }
        }

        ParseTreeRemoveAllChildren(parse_tree);
        child1 = ParseTreeAddChild(parse_tree);
        child2 = ParseTreeAddChild(parse_tree);
        child3 = ParseTreeAddChild(parse_tree);
        child4 = ParseTreeAddChild(parse_tree);
        child5 = ParseTreeAddChild(parse_tree);

        /* TODO(AARON): Not sure how many child nodes to create! */
        ParseTreeAddChild(parse_tree);
        ParseTreeAddChild(parse_tree);
        ParseTreeAddChild(parse_tree);
        ParseTreeAddChild(parse_tree);
        ParseTreeAddChild(parse_tree);
        *tokenizer = at_token;

        if (Token_Keyword == tokens[0].type &&
            gs_StringIsEqual("for", tokens[0].text, tokens[0].text_length) &&
            Token_OpenParen == (tokens[1] = GetToken(tokenizer)).type) {
                ParseTreeSet(child1, ParseTreeNode_Keyword, tokens[0]);
                ParseTreeSet(child2, ParseTreeNode_Symbol, tokens[1]);

                int i = 2;

                Tokenizer Previous = *tokenizer;
                if (!ParseExpression(tokenizer, gs_TreeChildAt(parse_tree, ParseTreeNode, tree, i))) {
                        *tokenizer = Previous;
                } else {
                        i++;
                }

                if (Token_SemiColon != (tokens[2] = GetToken(tokenizer)).type) {
                        *tokenizer = start;
                        return false;
                }
                ParseTreeSet(gs_TreeChildAt(parse_tree, ParseTreeNode, tree, i++), ParseTreeNode_Symbol, tokens[2]);

                Previous = *tokenizer;
                if (!ParseExpression(tokenizer, gs_TreeChildAt(parse_tree, ParseTreeNode, tree, i++))) {
                        --i;
                        *tokenizer = Previous;
                }

                if (Token_SemiColon != (tokens[3] = GetToken(tokenizer)).type) {
                        *tokenizer = start;
                        return false;
                }
                ParseTreeSet(gs_TreeChildAt(parse_tree, ParseTreeNode, tree, i++), ParseTreeNode_Symbol, tokens[3]);

                Previous = *tokenizer;
                if (!ParseExpression(tokenizer, gs_TreeChildAt(parse_tree, ParseTreeNode, tree, i++))) {
                        --i;
                        *tokenizer = Previous;
                }

                if (Token_CloseParen == (tokens[4] = GetToken(tokenizer)).type &&
                    ParseStatement(tokenizer, gs_TreeChildAt(parse_tree, ParseTreeNode, tree, i + 1))) {
                        ParseTreeSet(gs_TreeChildAt(parse_tree, ParseTreeNode, tree, i), ParseTreeNode_Symbol, tokens[4]);
                        return true;
                }
        }

        ParseTreeRemoveAllChildren(parse_tree);
        *tokenizer = start;

        return false;
}

/*
  selection-statement:
  if ( expression ) statement
  if ( expression ) statement else statement
  switch ( expression ) statement
*/
bool ParseSelectionStatement(Tokenizer *tokenizer, ParseTreeNode *parse_tree) {
        Tokenizer start = *tokenizer;
        Token tokens[3];
        tokens[0] = GetToken(tokenizer);
        Tokenizer at_token = *tokenizer;
        ParseTreeNode *child1, *child2, *child3, *child4, *child5, *child6;

        parse_tree->type = ParseTreeNode_SelectionStatement;
        child1 = ParseTreeAddChild(parse_tree);
        child2 = ParseTreeAddChild(parse_tree);
        child3 = ParseTreeAddChild(parse_tree);
        child4 = ParseTreeAddChild(parse_tree);
        child5 = ParseTreeAddChild(parse_tree);
        child6 = ParseTreeAddChild(parse_tree);

        if (Token_Keyword == tokens[0].type &&
            gs_StringIsEqual("if", tokens[0].text, tokens[0].text_length) &&
            Token_OpenParen == (tokens[1] = GetToken(tokenizer)).type &&
            ParseExpression(tokenizer, child3) &&
            Token_CloseParen == (tokens[2] = GetToken(tokenizer)).type &&
            ParseStatement(tokenizer, child5)) {
                Tokenizer at_else = *tokenizer;
                Token token = GetToken(tokenizer);

                ParseTreeSet(child1, ParseTreeNode_Keyword, tokens[0]);
                ParseTreeSet(child2, ParseTreeNode_Symbol, tokens[1]);
                ParseTreeSet(child4, ParseTreeNode_Symbol, tokens[2]);

                if (Token_Keyword == token.type &&
                    gs_StringIsEqual("else", token.text, token.text_length) &&
                    ParseStatement(tokenizer, child6)) {
                        ParseTreeSet(child5, ParseTreeNode_Keyword, token);
                        return true;
                }

                *tokenizer = at_else;
                return true;
        }

        ParseTreeRemoveAllChildren(parse_tree);
        child1 = ParseTreeAddChild(parse_tree);
        child2 = ParseTreeAddChild(parse_tree);
        child3 = ParseTreeAddChild(parse_tree);
        child4 = ParseTreeAddChild(parse_tree);
        child5 = ParseTreeAddChild(parse_tree);
        child6 = ParseTreeAddChild(parse_tree);
        *tokenizer = at_token;

        if (Token_Keyword == tokens[0].type &&
            gs_StringIsEqual("switch", tokens[0].text, tokens[0].text_length) &&
            Token_OpenParen == (tokens[1] = GetToken(tokenizer)).type &&
            ParseExpression(tokenizer, child3) &&
            Token_CloseParen == (tokens[2] = GetToken(tokenizer)).type &&
            ParseStatement(tokenizer, child5)) {
                ParseTreeSet(child1, ParseTreeNode_Keyword, tokens[0]);
                ParseTreeSet(child2, ParseTreeNode_Symbol, tokens[1]);
                ParseTreeSet(child4, ParseTreeNode_Symbol, tokens[2]);
                return true;
        }

        ParseTreeRemoveAllChildren(parse_tree);
        *tokenizer = start;

        return false;
}

bool ParseStatementListI(Tokenizer *tokenizer, ParseTreeNode *parse_tree) {
        Tokenizer start = *tokenizer;
        ParseTreeNode *child1, *child2;

        parse_tree->type = ParseTreeNode_StatementListI;
        child1 = ParseTreeAddChild(parse_tree);
        child2 = ParseTreeAddChild(parse_tree);

        if (ParseStatement(tokenizer, child1) &&
            ParseStatementListI(tokenizer, child2)) {
                return true;
        }

        ParseTreeRemoveAllChildren(parse_tree);
        *tokenizer = start;

        return true;
}

/*
  statement-list:
  statement
  statement-list statement
*/
bool ParseStatementList(Tokenizer *tokenizer, ParseTreeNode *parse_tree) {
        Tokenizer start = *tokenizer;
        ParseTreeNode *child1, *child2;

        parse_tree->type = ParseTreeNode_StatementList;
        child1 = ParseTreeAddChild(parse_tree);
        child2 = ParseTreeAddChild(parse_tree);

        if (ParseStatement(tokenizer, child1) &&
            ParseStatementListI(tokenizer, child2)) {
                return true;
        }

        ParseTreeRemoveAllChildren(parse_tree);
        *tokenizer = start;

        return false;
}

/*
  compound-statement:
  { declaration-list(opt) statement-list(opt) }
  TODO: AARON: Walking children this way is not good
*/
bool ParseCompoundStatement(Tokenizer *tokenizer, ParseTreeNode *parse_tree) {
        Tokenizer start = *tokenizer;
        Token token;
        ParseTreeNode *child1, *child2, *child3;

        parse_tree->type = ParseTreeNode_CompoundStatement;
        child1 = ParseTreeAddChild(parse_tree);
        child2 = ParseTreeAddChild(parse_tree);
        child3 = ParseTreeAddChild(parse_tree);

        if (Token_OpenBrace == (token = GetToken(tokenizer)).type) {
                ParseTreeSet(child1, ParseTreeNode_Symbol, token);
                int i = 0;

                Tokenizer Previous = *tokenizer;
                if (!ParseDeclarationList(tokenizer, gs_TreeChildAt(parse_tree, ParseTreeNode, tree, i++))) {
                        --i;
                        *tokenizer = Previous;
                }

                Previous = *tokenizer;
                if (!ParseStatementList(tokenizer, gs_TreeChildAt(parse_tree, ParseTreeNode, tree, i++))) {
                        --i;
                        *tokenizer = Previous;
                }

                if (Token_CloseBrace == (token = GetToken(tokenizer)).type) {
                        ParseTreeSet(gs_TreeChildAt(parse_tree, ParseTreeNode, tree, i), ParseTreeNode_Symbol, token);
                        return true;
                }
        }

        ParseTreeRemoveAllChildren(parse_tree);
        *tokenizer = start;

        return false;
}

/*
  expression-statement:
  expression(opt) ;
*/
bool ParseExpressionStatement(Tokenizer *tokenizer, ParseTreeNode *parse_tree) {
        Tokenizer start = *tokenizer;
        Token token;
        ParseTreeNode *child1, *child2;

        parse_tree->type = ParseTreeNode_ExpressionStatement;
        child1 = ParseTreeAddChild(parse_tree);
        child2 = ParseTreeAddChild(parse_tree);

        if (ParseExpression(tokenizer, child1) &&
            Token_SemiColon == (token = GetToken(tokenizer)).type) {
                ParseTreeSet(child2, ParseTreeNode_Symbol, token);
                return true;
        }

        ParseTreeRemoveAllChildren(parse_tree);
        child1 = ParseTreeAddChild(parse_tree);
        child2 = ParseTreeAddChild(parse_tree);
        *tokenizer = start;

        if (Token_SemiColon == (token = GetToken(tokenizer)).type) {
                ParseTreeSet(child1, ParseTreeNode_Symbol, token);
                return true;
        }

        ParseTreeRemoveAllChildren(parse_tree);
        *tokenizer = start;

        return false;
}

/*
  labeled-statement:
  identifier : statement
  case constant-expression : statement
  default : statement
*/
bool ParseLabeledStatement(Tokenizer *tokenizer, ParseTreeNode *parse_tree) {
        Tokenizer start = *tokenizer;
        Token tokens[2];
        ParseTreeNode *child1, *child2, *child3, *child4;

        parse_tree->type = ParseTreeNode_LabeledStatement;
        child1 = ParseTreeAddChild(parse_tree);
        child2 = ParseTreeAddChild(parse_tree);
        child3 = ParseTreeAddChild(parse_tree);
        child4 = ParseTreeAddChild(parse_tree);

        if (ParseIdentifier(tokenizer, child1) &&
            Token_Colon == (tokens[0] = GetToken(tokenizer)).type &&
            ParseStatement(tokenizer, child3)) {
                ParseTreeSet(child2, ParseTreeNode_Symbol, tokens[0]);
                return true;
        }

        ParseTreeRemoveAllChildren(parse_tree);
        child1 = ParseTreeAddChild(parse_tree);
        child2 = ParseTreeAddChild(parse_tree);
        child3 = ParseTreeAddChild(parse_tree);
        child4 = ParseTreeAddChild(parse_tree);
        *tokenizer = start;

        tokens[0] = GetToken(tokenizer);
        Tokenizer at_token = *tokenizer;

        if (Token_Keyword == tokens[0].type &&
            gs_StringIsEqual("case", tokens[0].text, tokens[0].text_length) &&
            ParseConstantExpression(tokenizer, child2) &&
            Token_Colon == (tokens[1] = GetToken(tokenizer)).type &&
            ParseStatement(tokenizer, child4)) {
                ParseTreeSet(child1, ParseTreeNode_Keyword, tokens[0]);
                ParseTreeSet(child3, ParseTreeNode_Symbol, tokens[1]);
                return true;
        }

        ParseTreeRemoveAllChildren(parse_tree);
        child1 = ParseTreeAddChild(parse_tree);
        child2 = ParseTreeAddChild(parse_tree);
        child3 = ParseTreeAddChild(parse_tree);
        child4 = ParseTreeAddChild(parse_tree);
        *tokenizer = at_token;

        if (Token_Keyword == tokens[0].type &&
            gs_StringIsEqual("default", tokens[0].text, tokens[0].text_length) &&
            Token_Colon == (tokens[1] = GetToken(tokenizer)).type &&
            ParseStatement(tokenizer, child3)) {
                ParseTreeSet(child1, ParseTreeNode_Keyword, tokens[0]);
                ParseTreeSet(child3, ParseTreeNode_Symbol, tokens[1]);
                return true;
        }

        ParseTreeRemoveAllChildren(parse_tree);
        *tokenizer = start;

        return false;
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
bool ParseStatement(Tokenizer *tokenizer, ParseTreeNode *parse_tree) {
        Tokenizer start = *tokenizer;
        ParseTreeNode *child1;

        parse_tree->type = ParseTreeNode_Statement;
        child1 = ParseTreeAddChild(parse_tree);

        if (ParseLabeledStatement(tokenizer, child1)) return true;

        ParseTreeRemoveAllChildren(parse_tree);
        child1 = ParseTreeAddChild(parse_tree);
        *tokenizer = start;

        if (ParseExpressionStatement(tokenizer, child1)) return true;

        ParseTreeRemoveAllChildren(parse_tree);
        child1 = ParseTreeAddChild(parse_tree);
        *tokenizer = start;

        if (ParseCompoundStatement(tokenizer, child1)) return true;

        ParseTreeRemoveAllChildren(parse_tree);
        child1 = ParseTreeAddChild(parse_tree);
        *tokenizer = start;

        if (ParseSelectionStatement(tokenizer, child1)) return true;

        ParseTreeRemoveAllChildren(parse_tree);
        child1 = ParseTreeAddChild(parse_tree);
        *tokenizer = start;

        if (ParseIterationStatement(tokenizer, child1)) return true;

        ParseTreeRemoveAllChildren(parse_tree);
        child1 = ParseTreeAddChild(parse_tree);
        *tokenizer = start;

        if (ParseJumpStatement(tokenizer, child1)) return true;

        ParseTreeRemoveAllChildren(parse_tree);
        *tokenizer = start;

        return false;
}

/*
  typedef-name:
  identifier
*/
bool ParseTypedefName(Tokenizer *tokenizer, ParseTreeNode *parse_tree) {
        Tokenizer start = *tokenizer;
        Token token = GetToken(tokenizer);
        ParseTreeNode *child1;

        *tokenizer = start;

        parse_tree->type = ParseTreeNode_TypedefName;
        child1 = ParseTreeAddChild(parse_tree);

        if (ParseIdentifier(tokenizer, child1) && TypedefIsName(token)) {
                ParseTreeSet(parse_tree, ParseTreeNode_TypedefName, token);
                return true;
        }

        ParseTreeRemoveAllChildren(parse_tree);
        *tokenizer = start;

        return false;
}

/*
  direct-abstract-declarator:
  ( abstract-declarator )
  direct-abstract-declarator(opt) [ constant-expression(opt) ]
  direct-abstract-declarator(opt) ( parameter-type-list(opt) )
*/
bool ParseDirectAbstractDeclaratorI(Tokenizer *tokenizer, ParseTreeNode *parse_tree) {
        Tokenizer start = *tokenizer;
        Token tokens[2];
        ParseTreeNode *child1, *child2, *child3, *child4;

        parse_tree->type = ParseTreeNode_DirectAbstractDeclaratorI;
        child1 = ParseTreeAddChild(parse_tree);
        child2 = ParseTreeAddChild(parse_tree);
        child3 = ParseTreeAddChild(parse_tree);
        child4 = ParseTreeAddChild(parse_tree);

        if (Token_OpenBracket == (tokens[0] = GetToken(tokenizer)).type &&
            ParseConstantExpression(tokenizer, child2) &&
            Token_CloseBracket == (tokens[1] = GetToken(tokenizer)).type &&
            ParseDirectAbstractDeclaratorI(tokenizer, child3)) {
                ParseTreeSet(child1, ParseTreeNode_Symbol, tokens[0]);
                ParseTreeSet(child3, ParseTreeNode_Symbol, tokens[1]);
                return true;
        }

        ParseTreeRemoveAllChildren(parse_tree);
        child1 = ParseTreeAddChild(parse_tree);
        child2 = ParseTreeAddChild(parse_tree);
        child3 = ParseTreeAddChild(parse_tree);
        child4 = ParseTreeAddChild(parse_tree);
        *tokenizer = start;

        if (Token_OpenBracket == (tokens[0] = GetToken(tokenizer)).type &&
            Token_CloseBracket == (tokens[1] = GetToken(tokenizer)).type &&
            ParseDirectAbstractDeclaratorI(tokenizer, child3)) {
                ParseTreeSet(child1, ParseTreeNode_Symbol, tokens[0]);
                ParseTreeSet(child2, ParseTreeNode_Symbol, tokens[1]);
                return true;
        }

        ParseTreeRemoveAllChildren(parse_tree);
        child1 = ParseTreeAddChild(parse_tree);
        child2 = ParseTreeAddChild(parse_tree);
        child3 = ParseTreeAddChild(parse_tree);
        child4 = ParseTreeAddChild(parse_tree);
        *tokenizer = start;

        if (Token_OpenParen == (tokens[0] = GetToken(tokenizer)).type &&
            ParseParameterTypeList(tokenizer, child2) &&
            Token_CloseParen == (tokens[1] = GetToken(tokenizer)).type &&
            ParseDirectAbstractDeclaratorI(tokenizer, child4)) {
                ParseTreeSet(child1, ParseTreeNode_Symbol, tokens[0]);
                ParseTreeSet(child3, ParseTreeNode_Symbol, tokens[1]);
                return true;
        }

        ParseTreeRemoveAllChildren(parse_tree);
        child1 = ParseTreeAddChild(parse_tree);
        child2 = ParseTreeAddChild(parse_tree);
        child3 = ParseTreeAddChild(parse_tree);
        child4 = ParseTreeAddChild(parse_tree);
        *tokenizer = start;

        if (Token_OpenParen == (tokens[0] = GetToken(tokenizer)).type &&
            Token_CloseParen == (tokens[1] = GetToken(tokenizer)).type &&
            ParseDirectAbstractDeclaratorI(tokenizer, child3)) {
                ParseTreeSet(child1, ParseTreeNode_Symbol, tokens[0]);
                ParseTreeSet(child2, ParseTreeNode_Symbol, tokens[1]);
                return true;
        }

        ParseTreeRemoveAllChildren(parse_tree);
        *tokenizer = start;

        return true;
}

/*
  direct-abstract-declarator:
  ( abstract-declarator )
  direct-abstract-declarator(opt) [ constant-expression(opt) ]
  direct-abstract-declarator(opt) ( parameter-type-list(opt) )
*/
bool ParseDirectAbstractDeclarator(Tokenizer *tokenizer, ParseTreeNode *parse_tree) {
        Tokenizer start = *tokenizer;
        Token tokens[2];
        ParseTreeNode *child1, *child2, *child3, *child4;

        parse_tree->type = ParseTreeNode_DirectAbstractDeclarator;
        child1 = ParseTreeAddChild(parse_tree);
        child2 = ParseTreeAddChild(parse_tree);
        child3 = ParseTreeAddChild(parse_tree);
        child4 = ParseTreeAddChild(parse_tree);

        if (Token_OpenParen == (tokens[0] = GetToken(tokenizer)).type &&
            ParseAbstractDeclarator(tokenizer, child2) &&
            Token_CloseParen == (tokens[1] = GetToken(tokenizer)).type &&
            ParseDirectAbstractDeclaratorI(tokenizer, child4)) {
                ParseTreeSet(child1, ParseTreeNode_Symbol, tokens[0]);
                ParseTreeSet(child3, ParseTreeNode_Symbol, tokens[1]);
                return true;
        }

        ParseTreeRemoveAllChildren(parse_tree);
        child1 = ParseTreeAddChild(parse_tree);
        child2 = ParseTreeAddChild(parse_tree);
        child3 = ParseTreeAddChild(parse_tree);
        child4 = ParseTreeAddChild(parse_tree);
        *tokenizer = start;

        if (Token_OpenBracket == (tokens[0] = GetToken(tokenizer)).type &&
            ParseConstantExpression(tokenizer, child2) &&
            Token_CloseBracket == (tokens[1] = GetToken(tokenizer)).type &&
            ParseDirectAbstractDeclaratorI(tokenizer, child4)) {
                ParseTreeSet(child1, ParseTreeNode_Symbol, tokens[0]);
                ParseTreeSet(child3, ParseTreeNode_Symbol, tokens[1]);
                return true;
        }

        ParseTreeRemoveAllChildren(parse_tree);
        child1 = ParseTreeAddChild(parse_tree);
        child2 = ParseTreeAddChild(parse_tree);
        child3 = ParseTreeAddChild(parse_tree);
        child4 = ParseTreeAddChild(parse_tree);
        *tokenizer = start;

        if (Token_OpenBracket == (tokens[0] = GetToken(tokenizer)).type &&
            Token_CloseBracket == (tokens[1] = GetToken(tokenizer)).type &&
            ParseDirectAbstractDeclaratorI(tokenizer, child3)) {
                ParseTreeSet(child1, ParseTreeNode_Symbol, tokens[0]);
                ParseTreeSet(child2, ParseTreeNode_Symbol, tokens[1]);
                return true;
        }

        ParseTreeRemoveAllChildren(parse_tree);
        child1 = ParseTreeAddChild(parse_tree);
        child2 = ParseTreeAddChild(parse_tree);
        child3 = ParseTreeAddChild(parse_tree);
        child4 = ParseTreeAddChild(parse_tree);
        *tokenizer = start;

        if (Token_OpenParen == (tokens[0] = GetToken(tokenizer)).type &&
            ParseParameterTypeList(tokenizer, child2) &&
            Token_CloseParen == (tokens[1] = GetToken(tokenizer)).type &&
            ParseDirectAbstractDeclaratorI(tokenizer, child3)) {
                ParseTreeSet(child1, ParseTreeNode_Symbol, tokens[0]);
                ParseTreeSet(child3, ParseTreeNode_Symbol, tokens[1]);
                return true;
        }

        ParseTreeRemoveAllChildren(parse_tree);
        child1 = ParseTreeAddChild(parse_tree);
        child2 = ParseTreeAddChild(parse_tree);
        child3 = ParseTreeAddChild(parse_tree);
        child4 = ParseTreeAddChild(parse_tree);
        *tokenizer = start;

        if (Token_OpenParen == (tokens[0] = GetToken(tokenizer)).type &&
            Token_CloseParen == (tokens[1] = GetToken(tokenizer)).type &&
            ParseDirectAbstractDeclaratorI(tokenizer, child3)) {
                ParseTreeSet(child1, ParseTreeNode_Symbol, tokens[0]);
                ParseTreeSet(child2, ParseTreeNode_Symbol, tokens[1]);
                return true;
        }

        ParseTreeRemoveAllChildren(parse_tree);
        *tokenizer = start;

        return true;
}

/*
  abstract-declarator:
  pointer
  pointer(opt) direct-abstract-declarator
*/
bool ParseAbstractDeclarator(Tokenizer *tokenizer, ParseTreeNode *parse_tree) {
        Tokenizer start = *tokenizer;
        ParseTreeNode *child1, *child2;

        parse_tree->type = ParseTreeNode_AbstractDeclarator;
        child1 = ParseTreeAddChild(parse_tree);
        child2 = ParseTreeAddChild(parse_tree);

        if (ParsePointer(tokenizer, child1) &&
            ParseDirectAbstractDeclarator(tokenizer, child2)) {
                return true;
        }

        ParseTreeRemoveAllChildren(parse_tree);
        child1 = ParseTreeAddChild(parse_tree);
        *tokenizer = start;

        if (ParseDirectAbstractDeclarator(tokenizer, child1)) {
                return true;
        }

        ParseTreeRemoveAllChildren(parse_tree);
        child1 = ParseTreeAddChild(parse_tree);
        *tokenizer = start;

        if (ParsePointer(tokenizer, child1)) {
                return true;
        }

        ParseTreeRemoveAllChildren(parse_tree);
        *tokenizer = start;

        return false;
}

/*
  type-name:
  specifier-qualifier-list abstract-declarator(opt)
*/
bool ParseTypeName(Tokenizer *tokenizer, ParseTreeNode *parse_tree) {
        Tokenizer start = *tokenizer;
        ParseTreeNode *child1, *child2;

        parse_tree->type = ParseTreeNode_TypeName;
        child1 = ParseTreeAddChild(parse_tree);
        child2 = ParseTreeAddChild(parse_tree);

        if (ParseSpecifierQualifierList(tokenizer, child1) &&
            ParseAbstractDeclarator(tokenizer, child2)) {
                return true;
        }

        ParseTreeRemoveAllChildren(parse_tree);
        child1 = ParseTreeAddChild(parse_tree);
        child2 = ParseTreeAddChild(parse_tree);
        *tokenizer = start;

        if (ParseSpecifierQualifierList(tokenizer, child1)) {
                return true;
        }

        ParseTreeRemoveAllChildren(parse_tree);
        *tokenizer = start;

        return false;
}

bool ParseInitializerListI(Tokenizer *tokenizer, ParseTreeNode *parse_tree) {
        Tokenizer start = *tokenizer;
        Token token;
        ParseTreeNode *child1, *child2, *child3;

        parse_tree->type = ParseTreeNode_InitializerListI;
        child1 = ParseTreeAddChild(parse_tree);
        child2 = ParseTreeAddChild(parse_tree);
        child3 = ParseTreeAddChild(parse_tree);

        if (Token_Comma == (token = GetToken(tokenizer)).type &&
            ParseInitializer(tokenizer, child2) &&
            ParseInitializerListI(tokenizer, child3)) {
                ParseTreeSet(child1, ParseTreeNode_Symbol, token);
                return true;
        }

        ParseTreeRemoveAllChildren(parse_tree);
        *tokenizer = start;

        return true;
}

/*
  initializer-list:
  initializer
  initializer-list , initializer
*/
bool ParseInitializerList(Tokenizer *tokenizer, ParseTreeNode *parse_tree) {
        Tokenizer start = *tokenizer;
        ParseTreeNode *child1, *child2;

        parse_tree->type = ParseTreeNode_InitializerList;
        child1 = ParseTreeAddChild(parse_tree);
        child2 = ParseTreeAddChild(parse_tree);

        if (ParseInitializer(tokenizer, child1) &&
            ParseInitializerListI(tokenizer, child2)) {
                return true;
        }

        ParseTreeRemoveAllChildren(parse_tree);
        *tokenizer = start;

        return false;
}

/*
  initializer:
  assignment-expression
  { initializer-list }
  { initializer-list , }
*/
bool ParseInitializer(Tokenizer *tokenizer, ParseTreeNode *parse_tree) {
        Tokenizer start = *tokenizer;
        Token tokens[3];
        ParseTreeNode *child1, *child2, *child3, *child4;

        parse_tree->type = ParseTreeNode_Initializer;
        child1 = ParseTreeAddChild(parse_tree);
        child2 = ParseTreeAddChild(parse_tree);
        child3 = ParseTreeAddChild(parse_tree);
        child4 = ParseTreeAddChild(parse_tree);

        if (ParseAssignmentExpression(tokenizer, child1)) {
                return true;
        }

        ParseTreeRemoveAllChildren(parse_tree);
        child1 = ParseTreeAddChild(parse_tree);
        child2 = ParseTreeAddChild(parse_tree);
        child3 = ParseTreeAddChild(parse_tree);
        child4 = ParseTreeAddChild(parse_tree);
        *tokenizer = start;

        if (Token_OpenBrace == (tokens[0] = GetToken(tokenizer)).type &&
            ParseInitializerList(tokenizer, child2) &&
            Token_CloseBrace == (tokens[1] = GetToken(tokenizer)).type) {
                ParseTreeSet(child1, ParseTreeNode_Symbol, tokens[0]);
                ParseTreeSet(child3, ParseTreeNode_Symbol, tokens[1]);
                return true;
        }

        ParseTreeRemoveAllChildren(parse_tree);
        child1 = ParseTreeAddChild(parse_tree);
        child2 = ParseTreeAddChild(parse_tree);
        child3 = ParseTreeAddChild(parse_tree);
        child4 = ParseTreeAddChild(parse_tree);
        *tokenizer = start;

        if (Token_OpenBrace == (tokens[0] = GetToken(tokenizer)).type &&
            ParseInitializerList(tokenizer, child2) &&
            Token_Comma == (tokens[1] = GetToken(tokenizer)).type &&
            Token_CloseBrace == (tokens[2] = GetToken(tokenizer)).type) {
                ParseTreeSet(child1, ParseTreeNode_Symbol, tokens[0]);
                ParseTreeSet(child3, ParseTreeNode_Symbol, tokens[1]);
                ParseTreeSet(child4, ParseTreeNode_Symbol, tokens[2]);
                return true;
        }

        ParseTreeRemoveAllChildren(parse_tree);
        *tokenizer = start;

        return false;
}

bool ParseIdentifierListI(Tokenizer *tokenizer, ParseTreeNode *parse_tree) {
        Tokenizer start = *tokenizer;
        Token token;
        ParseTreeNode *child1, *child2, *child3;

        parse_tree->type = ParseTreeNode_IdentifierListI;
        child1 = ParseTreeAddChild(parse_tree);
        child2 = ParseTreeAddChild(parse_tree);
        child3 = ParseTreeAddChild(parse_tree);

        if (Token_Comma == (token = GetToken(tokenizer)).type &&
            ParseIdentifier(tokenizer, child2) &&
            ParseIdentifierListI(tokenizer, child3)) {
                ParseTreeSet(child1, ParseTreeNode_Symbol, token);
                return true;
        }

        ParseTreeRemoveAllChildren(parse_tree);
        *tokenizer = start;

        return true;
}

/*
  identifier-list:
  identifier
  identifier-list , identifier
*/
bool ParseIdentifierList(Tokenizer *tokenizer, ParseTreeNode *parse_tree) {
        Tokenizer start = *tokenizer;
        ParseTreeNode *child1, *child2;

        parse_tree->type = ParseTreeNode_IdentifierList;
        child1 = ParseTreeAddChild(parse_tree);
        child2 = ParseTreeAddChild(parse_tree);

        if (ParseIdentifier(tokenizer, child1) &&
            ParseIdentifierListI(tokenizer, child2)) {
                return true;
        }

        ParseTreeRemoveAllChildren(parse_tree);
        *tokenizer = start;

        return false;
}

/*
  parameter-declaration:
  declaration-specifiers declarator
  declaration-specifiers abstract-declarator(opt)
*/
bool ParseParameterDeclaration(Tokenizer *tokenizer, ParseTreeNode *parse_tree) {
        Tokenizer start = *tokenizer;
        ParseTreeNode *child1, *child2;

        parse_tree->type = ParseTreeNode_ParameterDeclaration;
        child1 = ParseTreeAddChild(parse_tree);
        child2 = ParseTreeAddChild(parse_tree);

        if (ParseDeclarationSpecifiers(tokenizer, child1) &&
            ParseDeclarator(tokenizer, child2)) {
                return true;
        }

        ParseTreeRemoveAllChildren(parse_tree);
        child1 = ParseTreeAddChild(parse_tree);
        child2 = ParseTreeAddChild(parse_tree);
        *tokenizer = start;

        if (ParseDeclarationSpecifiers(tokenizer, child1) &&
            ParseAbstractDeclarator(tokenizer, child2)) {
                return true;
        }

        ParseTreeRemoveAllChildren(parse_tree);
        child1 = ParseTreeAddChild(parse_tree);
        child2 = ParseTreeAddChild(parse_tree);
        *tokenizer = start;

        if (ParseDeclarationSpecifiers(tokenizer, child1)) {
                return true;
        }

        ParseTreeRemoveAllChildren(parse_tree);
        *tokenizer = start;

        return false;
}

bool ParseParameterListI(Tokenizer *tokenizer, ParseTreeNode *parse_tree) {
        Tokenizer start = *tokenizer;
        Token token;
        ParseTreeNode *child1, *child2, *child3;

        parse_tree->type = ParseTreeNode_ParameterListI;
        child1 = ParseTreeAddChild(parse_tree);
        child2 = ParseTreeAddChild(parse_tree);
        child3 = ParseTreeAddChild(parse_tree);

        if (Token_Comma == (token = GetToken(tokenizer)).type &&
            ParseParameterDeclaration(tokenizer, child2) &&
            ParseParameterListI(tokenizer, child3)) {
                ParseTreeSet(child1, ParseTreeNode_Symbol, token);
                return true;
        }

        ParseTreeRemoveAllChildren(parse_tree);
        *tokenizer = start;

        return true;
}

/*
  parameter-list:
  parameter-declaration
  parameter-list , parameter-declaration
*/
bool ParseParameterList(Tokenizer *tokenizer, ParseTreeNode *parse_tree) {
        Tokenizer start = *tokenizer;
        ParseTreeNode *child1, *child2;

        parse_tree->type = ParseTreeNode_ParameterList;
        child1 = ParseTreeAddChild(parse_tree);
        child2 = ParseTreeAddChild(parse_tree);

        if (ParseParameterDeclaration(tokenizer, child1) &&
            ParseParameterListI(tokenizer, child2)) {
                return true;
        }

        ParseTreeRemoveAllChildren(parse_tree);
        *tokenizer = start;

        return false;
}

/*
  parameter-type-list:
  parameter-list
  parameter-list , ...
*/
bool ParseParameterTypeList(Tokenizer *tokenizer, ParseTreeNode *parse_tree) {
        Tokenizer start = *tokenizer;
        Token tokens[2];
        ParseTreeNode *child1, *child2, *child3;

        parse_tree->type = ParseTreeNode_ParameterTypeList;
        child1 = ParseTreeAddChild(parse_tree);
        child2 = ParseTreeAddChild(parse_tree);
        child3 = ParseTreeAddChild(parse_tree);

        if (ParseParameterList(tokenizer, child1)) {
                Tokenizer Previous = *tokenizer;
                if (Token_Comma == (tokens[0] = GetToken(tokenizer)).type &&
                    Token_Ellipsis == (tokens[1] = GetToken(tokenizer)).type) {
                        ParseTreeSet(child2, ParseTreeNode_Symbol, tokens[0]);
                        ParseTreeSet(child3, ParseTreeNode_Symbol, tokens[1]);
                        return true;
                }

                *tokenizer = Previous;
                return true;
        }

        ParseTreeRemoveAllChildren(parse_tree);
        *tokenizer = start;

        return false;
}

bool ParseTypeQualifierListI(Tokenizer *tokenizer, ParseTreeNode *parse_tree) {
        Tokenizer start = *tokenizer;
        ParseTreeNode *child1, *child2;

        parse_tree->type = ParseTreeNode_TypeQualifierListI;
        child1 = ParseTreeAddChild(parse_tree);
        child2 = ParseTreeAddChild(parse_tree);

        if (ParseTypeQualifier(tokenizer, child1) &&
            ParseTypeQualifierListI(tokenizer, child2)) {
                return true;
        }

        ParseTreeRemoveAllChildren(parse_tree);
        *tokenizer = start;

        return true;
}

/*
  type-qualifier-list:
  type-qualifier
  type-qualifier-list type-qualifier
*/
bool ParseTypeQualifierList(Tokenizer *tokenizer, ParseTreeNode *parse_tree) {
        Tokenizer start = *tokenizer;
        ParseTreeNode *child1, *child2;

        parse_tree->type = ParseTreeNode_TypeQualifierList;
        child1 = ParseTreeAddChild(parse_tree);
        child2 = ParseTreeAddChild(parse_tree);

        if (ParseTypeQualifier(tokenizer, child1) &&
            ParseTypeQualifierListI(tokenizer, child2)) {
                return true;
        }

        ParseTreeRemoveAllChildren(parse_tree);
        *tokenizer = start;

        return false;
}

/*
  pointer:
  * type-qualifier-list(opt)
  * type-qualifier-list(opt) pointer
  */
bool ParsePointer(Tokenizer *tokenizer, ParseTreeNode *parse_tree) {
        Tokenizer start = *tokenizer;
        Token token = GetToken(tokenizer);
        Tokenizer at_token = *tokenizer;
        ParseTreeNode *child1, *child2;

        parse_tree->type = ParseTreeNode_Pointer;
        child1 = ParseTreeAddChild(parse_tree);
        child2 = ParseTreeAddChild(parse_tree);

        if (Token_Asterisk != token.type) {
                *tokenizer = start;
                return false;
        }

        ParseTreeSet(parse_tree, ParseTreeNode_Pointer, token);

        if (ParseTypeQualifierList(tokenizer, child1) &&
            ParsePointer(tokenizer, child2)) {
                return true;
        }

        ParseTreeRemoveAllChildren(parse_tree);
        child1 = ParseTreeAddChild(parse_tree);
        child2 = ParseTreeAddChild(parse_tree);
        *tokenizer = at_token;

        if (ParsePointer(tokenizer, child1)) {
                return true;
        }

        ParseTreeRemoveAllChildren(parse_tree);
        child1 = ParseTreeAddChild(parse_tree);
        child2 = ParseTreeAddChild(parse_tree);
        *tokenizer = at_token;

        if (ParseTypeQualifierList(tokenizer, child1)) {
                return true;
        }

        ParseTreeRemoveAllChildren(parse_tree);
        *tokenizer = at_token;

        return true;
}

/*
  direct-declarator:
  identifier
  ( declarator )
  direct-declarator [ constant-expression(opt) ]
  direct-declarator ( parameter-type-list )
  direct-declarator ( identifier-list(opt) )

  A  → B | (C) | A[D] | A(E) | A(F)
  ---------------------------------
  A  → BA'
  A  → (C)A'
  A' → [D]A' | ε
  A' → (E)A' | ε
  A' → (F)A' | ε
*/
bool ParseDirectDeclaratorI(Tokenizer *tokenizer, ParseTreeNode *parse_tree) {
        Tokenizer start = *tokenizer;
        Token tokens[2];
        ParseTreeNode *child1, *child2, *child3, *child4;

        parse_tree->type = ParseTreeNode_DirectDeclaratorI;
        child1 = ParseTreeAddChild(parse_tree);
        child2 = ParseTreeAddChild(parse_tree);
        child3 = ParseTreeAddChild(parse_tree);
        child4 = ParseTreeAddChild(parse_tree);

        if (Token_OpenBracket == (tokens[0] = GetToken(tokenizer)).type &&
            ParseConstantExpression(tokenizer, child2) &&
            Token_CloseBracket == (tokens[1] = GetToken(tokenizer)).type &&
            ParseDirectDeclaratorI(tokenizer, child4)) {
                ParseTreeSet(child1, ParseTreeNode_Symbol, tokens[0]);
                ParseTreeSet(child3, ParseTreeNode_Symbol, tokens[1]);
                return true;
        }

        ParseTreeRemoveAllChildren(parse_tree);
        child1 = ParseTreeAddChild(parse_tree);
        child2 = ParseTreeAddChild(parse_tree);
        child3 = ParseTreeAddChild(parse_tree);
        child4 = ParseTreeAddChild(parse_tree);
        *tokenizer = start;

        if (Token_OpenBracket == (tokens[0] = GetToken(tokenizer)).type &&
            Token_CloseBracket == (tokens[1] = GetToken(tokenizer)).type &&
            ParseDirectDeclaratorI(tokenizer, child3)) {
                ParseTreeSet(child1, ParseTreeNode_Symbol, tokens[0]);
                ParseTreeSet(child2, ParseTreeNode_Symbol, tokens[1]);
                return true;
        }

        ParseTreeRemoveAllChildren(parse_tree);
        child1 = ParseTreeAddChild(parse_tree);
        child2 = ParseTreeAddChild(parse_tree);
        child3 = ParseTreeAddChild(parse_tree);
        child4 = ParseTreeAddChild(parse_tree);
        *tokenizer = start;

        if (Token_OpenParen == (tokens[0] = GetToken(tokenizer)).type &&
            ParseParameterTypeList(tokenizer, child2) &&
            Token_CloseParen == (tokens[1] = GetToken(tokenizer)).type &&
            ParseDirectDeclaratorI(tokenizer, child4)) {
                ParseTreeSet(child1, ParseTreeNode_Symbol, tokens[0]);
                ParseTreeSet(child3, ParseTreeNode_Symbol, tokens[1]);
                return true;
        }

        ParseTreeRemoveAllChildren(parse_tree);
        child1 = ParseTreeAddChild(parse_tree);
        child2 = ParseTreeAddChild(parse_tree);
        child3 = ParseTreeAddChild(parse_tree);
        child4 = ParseTreeAddChild(parse_tree);
        *tokenizer = start;

        if (Token_OpenParen == (tokens[0] = GetToken(tokenizer)).type &&
            ParseIdentifierList(tokenizer, child2) &&
            Token_CloseParen == (tokens[1] = GetToken(tokenizer)).type &&
            ParseDirectDeclaratorI(tokenizer, child4)) {
                ParseTreeSet(child1, ParseTreeNode_Symbol, tokens[0]);
                ParseTreeSet(child3, ParseTreeNode_Symbol, tokens[1]);
                return true;
        }

        ParseTreeRemoveAllChildren(parse_tree);
        child1 = ParseTreeAddChild(parse_tree);
        child2 = ParseTreeAddChild(parse_tree);
        child3 = ParseTreeAddChild(parse_tree);
        child4 = ParseTreeAddChild(parse_tree);
        *tokenizer = start;

        if (Token_OpenParen == (tokens[0] = GetToken(tokenizer)).type &&
            Token_CloseParen == (tokens[1] = GetToken(tokenizer)).type &&
            ParseDirectDeclaratorI(tokenizer, child3)) {
                ParseTreeSet(child1, ParseTreeNode_Symbol, tokens[0]);
                ParseTreeSet(child2, ParseTreeNode_Symbol, tokens[1]);
                return true;
        }

        ParseTreeRemoveAllChildren(parse_tree);
        *tokenizer = start;

        return true;
}

/*
  direct-declarator:
  identifier
  ( declarator )
  direct-declarator [ constant-expression(opt) ]
  direct-declarator ( parameter-type-list )
  direct-declarator ( identifier-list(opt) )

  A  → B | (C) | A[D] | A(E) | A(F)
  ---------------------------------
  A  → BA'
  A  → (C)A'
  A' → [D]A' | ε
  A' → (E)A' | ε
  A' → (F)A' | ε
*/
bool ParseDirectDeclarator(Tokenizer *tokenizer, ParseTreeNode *parse_tree) {
        Tokenizer start = *tokenizer;
        Token tokens[2];
        ParseTreeNode *child1, *child2, *child3, *child4;

        parse_tree->type = ParseTreeNode_DirectDeclarator;
        child1 = ParseTreeAddChild(parse_tree);
        child2 = ParseTreeAddChild(parse_tree);
        child3 = ParseTreeAddChild(parse_tree);
        child4 = ParseTreeAddChild(parse_tree);

        if (ParseIdentifier(tokenizer, child1) &&
            ParseDirectDeclaratorI(tokenizer, child2)) {
                return true;
        }

        ParseTreeRemoveAllChildren(parse_tree);
        child1 = ParseTreeAddChild(parse_tree);
        child2 = ParseTreeAddChild(parse_tree);
        child3 = ParseTreeAddChild(parse_tree);
        child4 = ParseTreeAddChild(parse_tree);
        *tokenizer = start;

        if (Token_OpenParen == (tokens[0] = GetToken(tokenizer)).type &&
            ParseDeclarator(tokenizer, child2) &&
            Token_CloseParen == (tokens[1] = GetToken(tokenizer)).type &&
            ParseDirectDeclaratorI(tokenizer, child4)) {
                ParseTreeSet(child1, ParseTreeNode_Symbol, tokens[0]);
                ParseTreeSet(child3, ParseTreeNode_Symbol, tokens[1]);
                return true;
        }

        ParseTreeRemoveAllChildren(parse_tree);
        *tokenizer = start;

        return false;
}

/*
  declarator:
  pointer(opt) direct-declarator
*/
bool ParseDeclarator(Tokenizer *tokenizer, ParseTreeNode *parse_tree) {
        Tokenizer start = *tokenizer;
        ParseTreeNode *child1, *child2;

        parse_tree->type = ParseTreeNode_Declarator;
        child1 = ParseTreeAddChild(parse_tree);
        child2 = ParseTreeAddChild(parse_tree);

        if (ParsePointer(tokenizer, child1) &&
            ParseDirectDeclarator(tokenizer, child2)) {
                return true;
        }

        ParseTreeRemoveAllChildren(parse_tree);
        child1 = ParseTreeAddChild(parse_tree);
        *tokenizer = start;

        if (ParseDirectDeclarator(tokenizer, child1)) {
                return true;
        }

        ParseTreeRemoveAllChildren(parse_tree);
        *tokenizer = start;

        return false;
}

/*
  enumerator:
  identifier
  identifier = constant-expression
*/
bool ParseEnumerator(Tokenizer *tokenizer, ParseTreeNode *parse_tree) {
        Tokenizer start = *tokenizer;
        Token token;
        ParseTreeNode *child1, *child2, *child3;

        parse_tree->type = ParseTreeNode_Enumerator;
        child1 = ParseTreeAddChild(parse_tree);
        child2 = ParseTreeAddChild(parse_tree);
        child3 = ParseTreeAddChild(parse_tree);

        if (ParseIdentifier(tokenizer, child1) &&
            Token_EqualSign == (token = GetToken(tokenizer)).type &&
            ParseConstantExpression(tokenizer, child3)) {
                ParseTreeSet(child2, ParseTreeNode_Symbol, token);
                return true;
        }

        ParseTreeRemoveAllChildren(parse_tree);
        child1 = ParseTreeAddChild(parse_tree);
        *tokenizer = start;

        if (ParseIdentifier(tokenizer, child1)) {
                return true;
        }

        ParseTreeRemoveAllChildren(parse_tree);
        *tokenizer = start;

        return false;
}

bool ParseEnumeratorListI(Tokenizer *tokenizer, ParseTreeNode *parse_tree) {
        Tokenizer start = *tokenizer;
        Token token;
        ParseTreeNode *child1, *child2, *child3;

        parse_tree->type = ParseTreeNode_EnumeratorListI;
        child1 = ParseTreeAddChild(parse_tree);
        child2 = ParseTreeAddChild(parse_tree);
        child3 = ParseTreeAddChild(parse_tree);

        if (Token_Comma == (token = GetToken(tokenizer)).type &&
            ParseEnumerator(tokenizer, child2) &&
            ParseEnumeratorListI(tokenizer, child3)) {
                ParseTreeSet(child1, ParseTreeNode_Symbol, token);
                return true;
        }

        ParseTreeRemoveAllChildren(parse_tree);
        *tokenizer = start;

        return true;
}

/*
  enumerator-list:
  enumerator
  enumerator-list , enumerator

  A  → B | A,B
  ------------
  A  → BA'
  A' → ,BA' | ε
*/
bool ParseEnumeratorList(Tokenizer *tokenizer, ParseTreeNode *parse_tree) {
        Tokenizer start = *tokenizer;
        ParseTreeNode *child1, *child2;

        parse_tree->type = ParseTreeNode_EnumeratorList;
        child1 = ParseTreeAddChild(parse_tree);
        child2 = ParseTreeAddChild(parse_tree);

        if (ParseEnumerator(tokenizer, child1) &&
            ParseEnumeratorListI(tokenizer, child2)) {
                return true;
        }

        ParseTreeRemoveAllChildren(parse_tree);
        *tokenizer = start;

        return true;
}

/*
  enum-specifier:
  enum identifier(opt) { enumerator-list }
  enum identifier
*/
bool ParseEnumSpecifier(Tokenizer *tokenizer, ParseTreeNode *parse_tree) {
        Tokenizer start = *tokenizer;
        Token token = GetToken(tokenizer);
        Tokenizer at_token = *tokenizer;
        Token tokens[2];
        ParseTreeNode *child1, *child2, *child3, *child4, *child5;

        parse_tree->type = ParseTreeNode_EnumSpecifier;
        child1 = ParseTreeAddChild(parse_tree);
        child2 = ParseTreeAddChild(parse_tree);
        child3 = ParseTreeAddChild(parse_tree);
        child4 = ParseTreeAddChild(parse_tree);
        child5 = ParseTreeAddChild(parse_tree);

        if (!(Token_Keyword == token.type &&
              gs_StringIsEqual("enum", token.text, token.text_length))) {
                *tokenizer = start;
                return false;
        }

        ParseTreeSet(child1, ParseTreeNode_Keyword, token);

        if (ParseIdentifier(tokenizer, child2) &&
            Token_OpenBrace == (tokens[0] = GetToken(tokenizer)).type &&
            ParseEnumeratorList(tokenizer, child4) &&
            Token_CloseBrace == (tokens[1] = GetToken(tokenizer)).type) {
                ParseTreeSet(child3, ParseTreeNode_Symbol, tokens[0]);
                ParseTreeSet(child5, ParseTreeNode_Symbol, tokens[1]);
                return true;
        }


        ParseTreeRemoveAllChildren(parse_tree);
        child1 = ParseTreeAddChild(parse_tree);
        child2 = ParseTreeAddChild(parse_tree);
        child3 = ParseTreeAddChild(parse_tree);
        child4 = ParseTreeAddChild(parse_tree);
        *tokenizer = at_token;

        if (Token_OpenBrace == (tokens[0] = GetToken(tokenizer)).type &&
            ParseEnumeratorList(tokenizer, child3) &&
            Token_CloseBrace == (tokens[1] = GetToken(tokenizer)).type) {
                ParseTreeSet(child2, ParseTreeNode_Symbol, tokens[0]);
                ParseTreeSet(child4, ParseTreeNode_Symbol, tokens[1]);
                return true;
        }

        ParseTreeRemoveAllChildren(parse_tree);
        child1 = ParseTreeAddChild(parse_tree);
        *tokenizer = at_token;

        if (ParseIdentifier(tokenizer, child1)) {
                return true;
        }

        ParseTreeRemoveAllChildren(parse_tree);
        *tokenizer = start;

        return false;
}

/*
  struct-declarator:
  declarator
  declarator(opt) : constant-expression
*/
bool ParseStructDeclarator(Tokenizer *tokenizer, ParseTreeNode *parse_tree) {
        Tokenizer start = *tokenizer;
        Token token;
        ParseTreeNode *child1, *child2, *child3;

        parse_tree->type = ParseTreeNode_StructDeclarator;
        child1 = ParseTreeAddChild(parse_tree);
        child2 = ParseTreeAddChild(parse_tree);
        child3 = ParseTreeAddChild(parse_tree);

        if (ParseDeclarator(tokenizer, child1) &&
            Token_Colon == (token = GetToken(tokenizer)).type &&
            ParseConstantExpression(tokenizer, child3)) {
                ParseTreeSet(child2, ParseTreeNode_Symbol, token);
                return true;
        }

        ParseTreeRemoveAllChildren(parse_tree);
        child1 = ParseTreeAddChild(parse_tree);
        child2 = ParseTreeAddChild(parse_tree);
        *tokenizer = start;

        if (Token_Colon == (token = GetToken(tokenizer)).type &&
            ParseConstantExpression(tokenizer, child2)) {
                ParseTreeSet(child1, ParseTreeNode_Symbol, token);
                return true;
        }

        ParseTreeRemoveAllChildren(parse_tree);
        child1 = ParseTreeAddChild(parse_tree);
        *tokenizer = start;

        if (ParseDeclarator(tokenizer, child1)) {
                return true;
        }

        ParseTreeRemoveAllChildren(parse_tree);
        *tokenizer = start;

        return false;
}

bool ParseStructDeclaratorListI(Tokenizer *tokenizer, ParseTreeNode *parse_tree) {
        Tokenizer start = *tokenizer;
        Token token;
        ParseTreeNode *child1, *child2, *child3;

        parse_tree->type = ParseTreeNode_StructDeclaratorListI;
        child1 = ParseTreeAddChild(parse_tree);
        child2 = ParseTreeAddChild(parse_tree);
        child3 = ParseTreeAddChild(parse_tree);

        if (Token_Comma == (token = GetToken(tokenizer)).type &&
            ParseStructDeclarator(tokenizer, child2) &&
            ParseStructDeclaratorListI(tokenizer, child3)) {
                ParseTreeSet(child1, ParseTreeNode_Symbol, token);
                return true;
        }

        ParseTreeRemoveAllChildren(parse_tree);
        *tokenizer = start;

        return true;
}

/*
  struct-declarator-list:
  struct-declarator
  struct-declarator-list , struct-declarator

  Sdl  → Sd | Sdl , Sd
  ------------
  Sdl  → Sd Sdl'
  Sdl' → , Sd Sdl' | ε
*/
bool ParseStructDeclaratorList(Tokenizer *tokenizer, ParseTreeNode *parse_tree) {
        Tokenizer start = *tokenizer;
        ParseTreeNode *child1, *child2;

        parse_tree->type = ParseTreeNode_StructDeclaratorList;
        child1 = ParseTreeAddChild(parse_tree);
        child2 = ParseTreeAddChild(parse_tree);

        if (ParseStructDeclarator(tokenizer, child1) &&
            ParseStructDeclaratorListI(tokenizer, child2)) {
                return true;
        }

        ParseTreeRemoveAllChildren(parse_tree);
        *tokenizer = start;

        return false;
}

/*
  specifier-qualifier-list:
  type-specifier specifier-qualifier-list(opt)
  type-qualifier specifier-qualifier-list(opt)
*/
bool ParseSpecifierQualifierList(Tokenizer *tokenizer, ParseTreeNode *parse_tree) {
        Tokenizer start = *tokenizer;
        ParseTreeNode *child1, *child2;

        parse_tree->type = ParseTreeNode_SpecifierQualifierList;
        child1 = ParseTreeAddChild(parse_tree);
        child2 = ParseTreeAddChild(parse_tree);

        if (ParseTypeSpecifier(tokenizer, child1) &&
            ParseSpecifierQualifierList(tokenizer, child2)) {
                return true;
        }

        ParseTreeRemoveAllChildren(parse_tree);
        child1 = ParseTreeAddChild(parse_tree);
        child2 = ParseTreeAddChild(parse_tree);
        *tokenizer = start;

        if (ParseTypeSpecifier(tokenizer, child1)) return true;

        ParseTreeRemoveAllChildren(parse_tree);
        child1 = ParseTreeAddChild(parse_tree);
        child2 = ParseTreeAddChild(parse_tree);
        *tokenizer = start;

        if (ParseTypeQualifier(tokenizer, child1) &&
            ParseSpecifierQualifierList(tokenizer, child2)) {
                return true;
        }

        ParseTreeRemoveAllChildren(parse_tree);
        child1 = ParseTreeAddChild(parse_tree);
        *tokenizer = start;

        if (ParseTypeQualifier(tokenizer, child1)) return true;

        ParseTreeRemoveAllChildren(parse_tree);
        *tokenizer = start;

        return false;
}

/*
  struct-declaration:
  specifier-qualifier-list struct-declarator-list ;
*/
bool ParseStructDeclaration(Tokenizer *tokenizer, ParseTreeNode *parse_tree) {
        Tokenizer start = *tokenizer;
        Token token;
        ParseTreeNode *child1, *child2, *child3;

        parse_tree->type = ParseTreeNode_StructDeclaration;
        child1 = ParseTreeAddChild(parse_tree);
        child2 = ParseTreeAddChild(parse_tree);
        child3 = ParseTreeAddChild(parse_tree);

        if (ParseSpecifierQualifierList(tokenizer, child1) &&
            ParseStructDeclaratorList(tokenizer, child2) &&
            Token_SemiColon == (token = GetToken(tokenizer)).type) {
                ParseTreeSet(child3, ParseTreeNode_StructDeclaration, token);
                return true;
        }

        ParseTreeRemoveAllChildren(parse_tree);
        *tokenizer = start;

        return false;
}

/*
  init-declarator:
  declarator
  declarator = initializer
*/
bool ParseInitDeclarator(Tokenizer *tokenizer, ParseTreeNode *parse_tree) {
        Tokenizer start = *tokenizer;
        Token token;
        ParseTreeNode *child1, *child2, *child3;

        parse_tree->type = ParseTreeNode_InitDeclarator;
        child1 = ParseTreeAddChild(parse_tree);
        child2 = ParseTreeAddChild(parse_tree);
        child3 = ParseTreeAddChild(parse_tree);

        if (ParseDeclarator(tokenizer, child1) &&
            Token_EqualSign == (token = GetToken(tokenizer)).type &&
            ParseInitializer(tokenizer, child3)) {
                ParseTreeSet(child2, ParseTreeNode_Symbol, token);
                return true;
        }

        ParseTreeRemoveAllChildren(parse_tree);
        child1 = ParseTreeAddChild(parse_tree);
        *tokenizer = start;

        if (ParseDeclarator(tokenizer, child1)) return true;

        ParseTreeRemoveAllChildren(parse_tree);
        *tokenizer = start;

        return false;
}

bool ParseInitDeclaratorListI(Tokenizer *tokenizer, ParseTreeNode *parse_tree) {
        Tokenizer start = *tokenizer;
        Token token;
        ParseTreeNode *child1, *child2, *child3;

        parse_tree->type = ParseTreeNode_InitDeclaratorListI;
        child1 = ParseTreeAddChild(parse_tree);
        child2 = ParseTreeAddChild(parse_tree);
        child3 = ParseTreeAddChild(parse_tree);

        if (Token_Comma == (token = GetToken(tokenizer)).type &&
            ParseInitDeclarator(tokenizer, child2) &&
            ParseInitDeclaratorListI(tokenizer, child3)) {
                ParseTreeSet(child1, ParseTreeNode_Symbol, token);
                return true;
        }

        ParseTreeRemoveAllChildren(parse_tree);
        *tokenizer = start;

        return true;
}

/*
  init-declarator-list:
  init-declarator
  init-declarator-list , init-declarator

  Idl  → Id | Idl , Id
  ------------
  Idl  → Id Idl'
  Idl' → , Id Idl' | ε
*/
bool ParseInitDeclaratorList(Tokenizer *tokenizer, ParseTreeNode *parse_tree) {
        Tokenizer start = *tokenizer;
        ParseTreeNode *child1, *child2;

        parse_tree->type = ParseTreeNode_InitDeclarationList;
        child1 = ParseTreeAddChild(parse_tree);
        child2 = ParseTreeAddChild(parse_tree);

        if (ParseInitDeclarator(tokenizer, child1) &&
            ParseInitDeclaratorListI(tokenizer, child2)) {
                return true;
        }

        ParseTreeRemoveAllChildren(parse_tree);
        *tokenizer = start;

        return true;
}

bool ParseStructDeclarationListI(Tokenizer *tokenizer, ParseTreeNode *parse_tree) {
        Tokenizer start = *tokenizer;
        ParseTreeNode *child1, *child2;

        parse_tree->type = ParseTreeNode_StructDeclarationListI;
        child1 = ParseTreeAddChild(parse_tree);
        child2 = ParseTreeAddChild(parse_tree);

        if (ParseStructDeclaration(tokenizer, child1) &&
            ParseStructDeclarationListI(tokenizer, child2)) {
                return true;
        }

        ParseTreeRemoveAllChildren(parse_tree);
        *tokenizer = start;

        return true;
}

/*
  struct-declaration-list:
  struct-declaration
  struct-declaration-list struct-declaration

  Sdl  → Sd | Sdl Sd
  -----------
  Sdl  → Sd Sdl'
  Sdl' → Sd Sdl' | ε
*/
bool ParseStructDeclarationList(Tokenizer *tokenizer, ParseTreeNode *parse_tree) {
        Tokenizer start = *tokenizer;
        ParseTreeNode *child1, *child2;

        parse_tree->type = ParseTreeNode_StructDeclarationList;
        child1 = ParseTreeAddChild(parse_tree);
        child2 = ParseTreeAddChild(parse_tree);

        if (ParseStructDeclaration(tokenizer, child1) &&
            ParseStructDeclarationListI(tokenizer, child2)) {
                return true;
        }

        ParseTreeRemoveAllChildren(parse_tree);
        *tokenizer = start;

        return false;
}

/*
  struct-or-union:
  One of: struct union
*/
bool ParseStructOrUnion(Tokenizer *tokenizer, ParseTreeNode *parse_tree) {
        Tokenizer start = *tokenizer;
        Token token = GetToken(tokenizer);

        if (Token_Keyword == token.type) {
                if (gs_StringIsEqual(token.text, "struct", token.text_length) ||
                    gs_StringIsEqual(token.text, "union", token.text_length)) {
                        ParseTreeSet(parse_tree, ParseTreeNode_StructOrUnion, token);
                        return true;
                }
        }

        *tokenizer = start;

        return false;
}

/*
  struct-or-union-specifier:
  struct-or-union identifier(opt) { struct-declaration-list }
  struct-or-union identifier
*/
bool ParseStructOrUnionSpecifier(Tokenizer *tokenizer, ParseTreeNode *parse_tree) {
        Tokenizer start = *tokenizer;
        ParseTreeNode *child1, *child2, *child3, *child4, *child5;

        parse_tree->type = ParseTreeNode_StructOrUnionSpecifier;
        child1 = ParseTreeAddChild(parse_tree);
        child2 = ParseTreeAddChild(parse_tree);
        child3 = ParseTreeAddChild(parse_tree);
        child4 = ParseTreeAddChild(parse_tree);
        child5 = ParseTreeAddChild(parse_tree);

        Token tokens[2];

        if (ParseStructOrUnion(tokenizer, child1) &&
            ParseIdentifier(tokenizer, child2) &&
            Token_OpenBrace == (tokens[0] = GetToken(tokenizer)).type &&
            ParseStructDeclarationList(tokenizer, child4) &&
            Token_CloseBrace == (tokens[1] = GetToken(tokenizer)).type) {
                ParseTreeSet(child3, ParseTreeNode_Symbol, tokens[0]);
                ParseTreeSet(child5, ParseTreeNode_Symbol, tokens[1]);
                return true;
        }

        ParseTreeRemoveAllChildren(parse_tree);
        child1 = ParseTreeAddChild(parse_tree);
        child2 = ParseTreeAddChild(parse_tree);
        child3 = ParseTreeAddChild(parse_tree);
        child4 = ParseTreeAddChild(parse_tree);
        *tokenizer = start;

        if (ParseStructOrUnion(tokenizer, child1) &&
            Token_OpenBrace == (tokens[0] = GetToken(tokenizer)).type &&
            ParseStructDeclarationList(tokenizer, child3) &&
            Token_CloseBrace == (tokens[1] = GetToken(tokenizer)).type) {
                ParseTreeSet(child2, ParseTreeNode_Symbol, tokens[0]);
                ParseTreeSet(child4, ParseTreeNode_Symbol, tokens[1]);
                return true;
        }

        ParseTreeRemoveAllChildren(parse_tree);
        child1 = ParseTreeAddChild(parse_tree);
        child2 = ParseTreeAddChild(parse_tree);
        *tokenizer = start;

        if (ParseStructOrUnion(tokenizer, child1) &&
            ParseIdentifier(tokenizer, child2)) {
                return true;
        }

        ParseTreeRemoveAllChildren(parse_tree);
        *tokenizer = start;

        return false;
}

/*
  type-qualifier:
  One of: const volatile
*/
bool ParseTypeQualifier(Tokenizer *tokenizer, ParseTreeNode *parse_tree) {
        Tokenizer start = *tokenizer;
        Token token = GetToken(tokenizer);

        if (token.type == Token_Keyword) {
                if (gs_StringIsEqual(token.text, "const", token.text_length) ||
                    gs_StringIsEqual(token.text, "volatile", token.text_length)) {
                        ParseTreeSet(parse_tree, ParseTreeNode_TypeQualifier, token);
                        return true;
                }
        }

        *tokenizer = start;

        return false;
}

/*
  type-specifier:
  One of: void char short int long float double signed unsigned
  struct-or-union-specifier enum-specifier typedef-name
*/
bool ParseTypeSpecifier(Tokenizer *tokenizer, ParseTreeNode *parse_tree) {
        Tokenizer start = *tokenizer;
        char *keywords[] = { "void", "char", "short", "int", "long", "float",
                             "double", "signed", "unsigned" };

        Token token = GetToken(tokenizer);
        if (token.type == Token_Keyword) {
                for (int i = 0; i < gs_ArraySize(keywords); i++) {
                        if (gs_StringIsEqual(token.text, keywords[i], token.text_length)) {
                                ParseTreeSet(parse_tree, ParseTreeNode_TypeSpecifier, token);
                                return true;
                        }
                }
        }

        ParseTreeNode *child1;

        parse_tree->type = ParseTreeNode_TypeSpecifier;
        child1 = ParseTreeAddChild(parse_tree);
        *tokenizer = start;

        if (ParseStructOrUnionSpecifier(tokenizer, child1)) return true;

        ParseTreeRemoveAllChildren(parse_tree);
        child1 = ParseTreeAddChild(parse_tree);
        *tokenizer = start;

        if (ParseEnumSpecifier(tokenizer, child1)) return true;

        ParseTreeRemoveAllChildren(parse_tree);
        child1 = ParseTreeAddChild(parse_tree);
        *tokenizer = start;

        if (ParseTypedefName(tokenizer, child1)) return true;

        ParseTreeRemoveAllChildren(parse_tree);
        *tokenizer = start;

        return false;
}

/*
  storage-class-specifier:
  One of: auto register static extern typedef
*/
bool ParseStorageClassSpecifier(Tokenizer *tokenizer, ParseTreeNode *parse_tree) {
        Tokenizer start = *tokenizer;
        char *keywords[] = { "auto", "register", "static", "extern", "typedef" };

        Token token = GetToken(tokenizer);
        if (token.type == Token_Keyword) {
                for (int i = 0; i < gs_ArraySize(keywords); i++) {
                        if (gs_StringIsEqual(token.text, keywords[i], token.text_length)) {
                                ParseTreeSet(parse_tree, ParseTreeNode_StorageClassSpecifier, token);
                                return true;
                        }
                }
        }

        *tokenizer = start;
        return false;
}


/*
  declaration-specifiers:
  storage-class-specifier declaration-specifiers(opt)
  type-specifier declaration-specifiers(opt)
  type-qualifier declaration-specifiers(opt)
*/
bool ParseDeclarationSpecifiers(Tokenizer *tokenizer, ParseTreeNode *parse_tree) {
        Tokenizer start = *tokenizer;
        ParseTreeNode *child1, *child2;

        parse_tree->type = ParseTreeNode_DeclarationSpecifiers;
        child1 = ParseTreeAddChild(parse_tree);
        child2 = ParseTreeAddChild(parse_tree);

        if (ParseStorageClassSpecifier(tokenizer, child1) &&
            ParseDeclarationSpecifiers(tokenizer, child2)) {
                return true;
        }

        ParseTreeRemoveAllChildren(parse_tree);
        child1 = ParseTreeAddChild(parse_tree);
        child2 = ParseTreeAddChild(parse_tree);
        *tokenizer = start;

        if (ParseTypeSpecifier(tokenizer, child1) &&
            ParseDeclarationSpecifiers(tokenizer, child2)) {
                return true;
        }

        ParseTreeRemoveAllChildren(parse_tree);
        child1 = ParseTreeAddChild(parse_tree);
        child2 = ParseTreeAddChild(parse_tree);
        *tokenizer = start;

        if (ParseTypeQualifier(tokenizer, child1) &&
            ParseDeclarationSpecifiers(tokenizer, child2)) {
                return true;
        }

        ParseTreeRemoveAllChildren(parse_tree);
        child1 = ParseTreeAddChild(parse_tree);
        *tokenizer = start;

        if (ParseStorageClassSpecifier(tokenizer, child1)) {
                return true;
        }

        ParseTreeRemoveAllChildren(parse_tree);
        child1 = ParseTreeAddChild(parse_tree);
        *tokenizer = start;

        if (ParseTypeSpecifier(tokenizer, child1)) {
                return true;
        }

        ParseTreeRemoveAllChildren(parse_tree);
        child1 = ParseTreeAddChild(parse_tree);
        *tokenizer = start;

        if (ParseTypeQualifier(tokenizer, child1)) {
                return true;
        }

        ParseTreeRemoveAllChildren(parse_tree);
        *tokenizer = start;

        return false;
}

bool ParseDeclarationListI(Tokenizer *tokenizer, ParseTreeNode *parse_tree) {
        Tokenizer start = *tokenizer;
        ParseTreeNode *child1, *child2;

        parse_tree->type = ParseTreeNode_DeclarationListI;
        child1 = ParseTreeAddChild(parse_tree);
        child2 = ParseTreeAddChild(parse_tree);

        if (ParseDeclaration(tokenizer, child1) &&
            ParseDeclarationListI(tokenizer, child2)) {
                return true;
        }

        ParseTreeRemoveAllChildren(parse_tree);
        *tokenizer = start;

        return true;
}

/*
  declaration-list:
  declaration
  declaration-list declaration

  Dl  → D | Dl D
  -----------
  Dl  → D Dl'
  Dl' → D Dl' | ε
*/
bool ParseDeclarationList(Tokenizer *tokenizer, ParseTreeNode *parse_tree) {
        Tokenizer start = *tokenizer;
        ParseTreeNode *child1, *child2;

        parse_tree->type = ParseTreeNode_DeclarationList;
        child1 = ParseTreeAddChild(parse_tree);
        child2 = ParseTreeAddChild(parse_tree);

        if (ParseDeclaration(tokenizer, child1) &&
            ParseDeclarationListI(tokenizer, child2)) {
                return true;
        }

        ParseTreeRemoveAllChildren(parse_tree);
        *tokenizer = start;

        return false;
}

/*
  declaration:
  declaration-specifiers init-declarator-list(opt) ;
*/
bool ParseDeclaration(Tokenizer *tokenizer, ParseTreeNode *parse_tree) {
        Tokenizer start = *tokenizer;
        Token token;
        ParseTreeNode *child1, *child2, *child3;

        parse_tree->type = ParseTreeNode_Declaration;
        child1 = ParseTreeAddChild(parse_tree);
        child2 = ParseTreeAddChild(parse_tree);
        child3 = ParseTreeAddChild(parse_tree);

        if (ParseDeclarationSpecifiers(tokenizer, child1) &&
            ParseInitDeclaratorList(tokenizer, child2) &&
            Token_SemiColon == (token = GetToken(tokenizer)).type) {
                ParseTreeSet(child3, ParseTreeNode_Symbol, token);
                return true;
        }

        ParseTreeRemoveAllChildren(parse_tree);
        child1 = ParseTreeAddChild(parse_tree);
        child2 = ParseTreeAddChild(parse_tree);
        child3 = ParseTreeAddChild(parse_tree);
        *tokenizer = start;

        if (ParseDeclarationSpecifiers(tokenizer, child1) &&
            Token_SemiColon == (token = GetToken(tokenizer)).type) {
                ParseTreeSet(child2, ParseTreeNode_Symbol, token);
                return true;
        }

        ParseTreeRemoveAllChildren(parse_tree);
        *tokenizer = start;

        return false;
}

/*
  function-definition:
  declaration-specifiers(opt) declarator declaration-list(opt) compound-statement
*/
bool ParseFunctionDefinition(Tokenizer *tokenizer, ParseTreeNode *parse_tree) {
        Tokenizer start = *tokenizer;
        ParseTreeNode *child1, *child2, *child3, *child4;

        parse_tree->type = ParseTreeNode_FunctionDefinition;

        child1 = ParseTreeAddChild(parse_tree);
        child2 = ParseTreeAddChild(parse_tree);
        child3 = ParseTreeAddChild(parse_tree);
        child4 = ParseTreeAddChild(parse_tree);

        if (ParseDeclarationSpecifiers(tokenizer, child1) &&
            ParseDeclarator(tokenizer, child2) &&
            ParseDeclarationList(tokenizer, child3) &&
            ParseCompoundStatement(tokenizer, child4)) {
                return true;
        }

        ParseTreeRemoveAllChildren(parse_tree);
        child1 = ParseTreeAddChild(parse_tree);
        child2 = ParseTreeAddChild(parse_tree);
        child3 = ParseTreeAddChild(parse_tree);

        *tokenizer = start;
        if (ParseDeclarationSpecifiers(tokenizer, child1) &&
            ParseDeclarator(tokenizer, child2) &&
            ParseCompoundStatement(tokenizer, child3)) {
                return true;
        }

        ParseTreeRemoveAllChildren(parse_tree);
        child1 = ParseTreeAddChild(parse_tree);
        child2 = ParseTreeAddChild(parse_tree);
        child3 = ParseTreeAddChild(parse_tree);

        *tokenizer = start;
        if (ParseDeclarator(tokenizer, child1) &&
            ParseDeclarationList(tokenizer, child2) &&
            ParseCompoundStatement(tokenizer, child3)) {
                return true;
        }

        ParseTreeRemoveAllChildren(parse_tree);
        child1 = ParseTreeAddChild(parse_tree);
        child2 = ParseTreeAddChild(parse_tree);

        *tokenizer = start;
        if (ParseDeclarator(tokenizer, child1) &&
            ParseCompoundStatement(tokenizer, child2)) {
                return true;
        }

        ParseTreeRemoveAllChildren(parse_tree);
        *tokenizer = start;

        return false;
}

/*
  external-declaration:
  function-definition
  declaration
*/
bool ParseExternalDeclaration(Tokenizer *tokenizer, ParseTreeNode *parse_tree) {
        Tokenizer start = *tokenizer;
        ParseTreeNode *child1;

        parse_tree->type = ParseTreeNode_ExternalDeclaration;
        child1 = ParseTreeAddChild(parse_tree);

        if (ParseFunctionDefinition(tokenizer, child1)) return true;

        ParseTreeRemoveAllChildren(parse_tree);
        child1 = ParseTreeAddChild(parse_tree);
        *tokenizer = start;

        if (ParseDeclaration(tokenizer, child1)) return true;

        ParseTreeRemoveAllChildren(parse_tree);
        *tokenizer = start;

        return false;
}

bool ParseTranslationUnitI(Tokenizer *tokenizer, ParseTreeNode *parse_tree) {
        Tokenizer start = *tokenizer;
        ParseTreeNode *child1, *child2;

        parse_tree->type = ParseTreeNode_TranslationUnitI;
        child1 = ParseTreeAddChild(parse_tree);
        child2 = ParseTreeAddChild(parse_tree);

        if (ParseExternalDeclaration(tokenizer, child1) &&
            ParseTranslationUnitI(tokenizer, child2)) {
                return true;
        }

        ParseTreeRemoveAllChildren(parse_tree);
        *tokenizer = start;

        return true;
}

/*
  translation-unit:
  external-declaration
  translation-unit external-declaration

  Tu  → Ed | Tu Ed
  -----------
  Tu  → Ed Tu'
  Tu' → Ed Tu' | ε
*/
bool ParseTranslationUnit(Tokenizer *tokenizer, ParseTreeNode *parse_tree) {
        Tokenizer start = *tokenizer;
        ParseTreeNode *child1, *child2;

        parse_tree->type = ParseTreeNode_TranslationUnit;
        child1 = ParseTreeAddChild(parse_tree);
        child2 = ParseTreeAddChild(parse_tree);

        if (ParseExternalDeclaration(tokenizer, child1) &&
            ParseTranslationUnitI(tokenizer, child2)) {
                return true;
        }

        ParseTreeRemoveAllChildren(parse_tree);
        *tokenizer = start;

        return false;
}

bool Parse(gs_Allocator allocator, gs_Buffer *stream, ParseTreeNode **out_tree, Tokenizer *out_tokenizer) {
        __parser_allocator = allocator;
        ParseTreeNode *parse_tree = ParseTreeInit(allocator);

        Tokenizer tokenizer;
        tokenizer.beginning = tokenizer.at = stream->start;
        tokenizer.line = tokenizer.column = 1;

        TypedefInit(__parser_typedef_names);

        bool result = ParseTranslationUnit(&tokenizer, parse_tree);
        *out_tokenizer = tokenizer;
        *out_tree = parse_tree;

        return result;
}

#endif /* PARSER_C */
