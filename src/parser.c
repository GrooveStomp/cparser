/******************************************************************************
 * File: parser.c
 * Created: 2016-07-06
 * Updated: 2020-11-16
 * Package: C-Parser
 * Creator: Aaron Oman (GrooveStomp)
 * Copyright 2016 - 2020, Aaron Oman and the C-Parser contributors
 * SPDX-License-Identifier: LGPL-3.0-only
 ******************************************************************************/
#ifndef PARSER_C
#define PARSER_C

#include "gs.h"
#include "lexer.c"
#include "parse_tree.c"

gs_Allocator __parser_allocator;

void __parser_ParseTreeUpdate(ParseTreeNode *node, ParseTreeNodeType type, u32 num_children) {
        node->type = type;
        if (num_children > 0) {
                ParseTreeNewChildren(node, num_children);
        }
}

void __parser_ParseTreeClearChildren(ParseTreeNode *node) {
        for (int i = 0; i < node->num_children; i++) {
                node->children[i].type = ParseTreeNode_Unknown;
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

        __parser_ParseTreeUpdate(parse_tree, ParseTreeNode_ArgumentExpressionListI, 3);

        if (Token_Comma == (token = GetToken(tokenizer)).type &&
            ParseAssignmentExpression(tokenizer, &parse_tree->children[1]) &&
            ParseArgumentExpressionListI(tokenizer, &parse_tree->children[2])) {
                ParseTreeSet(&parse_tree->children[0], ParseTreeNode_Symbol, token);
                return true;
        }

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

        __parser_ParseTreeUpdate(parse_tree, ParseTreeNode_ArgumentExpressionList, 2);

        if (ParseAssignmentExpression(tokenizer, &parse_tree->children[0]) &&
            ParseArgumentExpressionListI(tokenizer, &parse_tree->children[1])) {
                return true;
        }

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

        __parser_ParseTreeUpdate(parse_tree, ParseTreeNode_PrimaryExpression, 3);

        if (Token_Identifier == (tokens[0] = GetToken(tokenizer)).type) {
                ParseTreeSet(&parse_tree->children[0], ParseTreeNode_Identifier, tokens[0]);
                return true;
        }

        *tokenizer = start;
        if (ParseConstant(tokenizer, &parse_tree->children[0])) return true;

        *tokenizer = start;
        if (Token_String == (tokens[0] = GetToken(tokenizer)).type) {
                ParseTreeSet(&parse_tree->children[0], ParseTreeNode_String, tokens[0]);
                return true;
        }

        *tokenizer = start;
        if (Token_OpenParen == (tokens[0] = GetToken(tokenizer)).type &&
            ParseExpression(tokenizer, &parse_tree->children[1]) &&
            Token_CloseParen == (tokens[1] = GetToken(tokenizer)).type) {
                ParseTreeSet(&parse_tree->children[0], ParseTreeNode_Symbol, tokens[0]);
                ParseTreeSet(&parse_tree->children[2], ParseTreeNode_Symbol, tokens[1]);
                return true;
        }

        *tokenizer = start;
        return false;
}

bool ParsePostfixExpressionI(Tokenizer *tokenizer, ParseTreeNode *parse_tree) {
        Tokenizer start = *tokenizer;
        Token tokens[2];

        __parser_ParseTreeUpdate(parse_tree, ParseTreeNode_PostfixExpressionI, 4);

        if (Token_OpenBracket == (tokens[0] = GetToken(tokenizer)).type &&
            ParseExpression(tokenizer, &parse_tree->children[1]) &&
            Token_CloseBracket == (tokens[0] = GetToken(tokenizer)).type &&
            ParsePostfixExpressionI(tokenizer, &parse_tree->children[3])) {
                ParseTreeSet(&parse_tree->children[0], ParseTreeNode_Symbol, tokens[0]);
                ParseTreeSet(&parse_tree->children[2], ParseTreeNode_Symbol, tokens[1]);
                return true;
        }

        *tokenizer = start;
        if (Token_OpenParen == (tokens[0] = GetToken(tokenizer)).type &&
            ParseArgumentExpressionList(tokenizer, &parse_tree->children[1]) &&
            Token_CloseParen == (tokens[1] = GetToken(tokenizer)).type &&
            ParsePostfixExpressionI(tokenizer, &parse_tree->children[3])) {
                ParseTreeSet(&parse_tree->children[0], ParseTreeNode_Symbol, tokens[0]);
                ParseTreeSet(&parse_tree->children[2], ParseTreeNode_Symbol, tokens[1]);
                return true;
        }

        __parser_ParseTreeClearChildren(parse_tree);

        *tokenizer = start;
        if (Token_OpenParen == (tokens[0] = GetToken(tokenizer)).type &&
            Token_CloseParen == (tokens[1] = GetToken(tokenizer)).type &&
            ParsePostfixExpressionI(tokenizer, &parse_tree->children[2])) {
                ParseTreeSet(&parse_tree->children[0], ParseTreeNode_Symbol, tokens[0]);
                ParseTreeSet(&parse_tree->children[1], ParseTreeNode_Symbol, tokens[1]);
                return true;
        }

        *tokenizer = start;
        if (Token_Dot == (tokens[0] = GetToken(tokenizer)).type &&
            Token_Identifier == (tokens[1] = GetToken(tokenizer)).type &&
            ParsePostfixExpressionI(tokenizer, &parse_tree->children[2])) {
                ParseTreeSet(&parse_tree->children[0], ParseTreeNode_Symbol, tokens[0]);
                ParseTreeSet(&parse_tree->children[1], ParseTreeNode_Identifier, tokens[1]);
                return true;
        }

        *tokenizer = start;
        if (Token_Arrow == (tokens[0] = GetToken(tokenizer)).type &&
            Token_Identifier == (tokens[1] = GetToken(tokenizer)).type &&
            ParsePostfixExpressionI(tokenizer, &parse_tree->children[2])) {
                ParseTreeSet(&parse_tree->children[0], ParseTreeNode_Symbol, tokens[0]);
                ParseTreeSet(&parse_tree->children[1], ParseTreeNode_Identifier, tokens[1]);
                return true;
        }

        __parser_ParseTreeClearChildren(parse_tree);

        *tokenizer = start;
        if (Token_PlusPlus == (tokens[0] = GetToken(tokenizer)).type &&
            ParsePostfixExpressionI(tokenizer, &parse_tree->children[1])) {
                ParseTreeSet(&parse_tree->children[0], ParseTreeNode_Symbol, tokens[0]);
                return true;
        }

        *tokenizer = start;
        if (Token_MinusMinus == (tokens[0] = GetToken(tokenizer)).type &&
            ParsePostfixExpressionI(tokenizer, &parse_tree->children[1])) {
                ParseTreeSet(&parse_tree->children[0], ParseTreeNode_Symbol, tokens[0]);
                return true;
        }

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

        __parser_ParseTreeUpdate(parse_tree, ParseTreeNode_PostfixExpression, 2);

        if (ParsePrimaryExpression(tokenizer, &parse_tree->children[0]) &&
            ParsePostfixExpressionI(tokenizer, &parse_tree->children[1])) {
                return true;
        }

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

        __parser_ParseTreeUpdate(parse_tree, ParseTreeNode_UnaryExpression, 4);

        if (ParsePostfixExpression(tokenizer, &parse_tree->children[0])) {
                return true;
        }

        __parser_ParseTreeClearChildren(parse_tree);

        *tokenizer = start;
        if (Token_PlusPlus == (tokens[0] = GetToken(tokenizer)).type &&
            ParseUnaryExpression(tokenizer, &parse_tree->children[1])) {
                ParseTreeSet(&parse_tree->children[0], ParseTreeNode_Symbol, tokens[0]);
                return true;
        }

        __parser_ParseTreeClearChildren(parse_tree);

        *tokenizer = start;
        if (Token_MinusMinus == (tokens[0] = GetToken(tokenizer)).type &&
            ParseUnaryExpression(tokenizer, &parse_tree->children[1])) {
                ParseTreeSet(&parse_tree->children[0], ParseTreeNode_Symbol, tokens[0]);
                return true;
        }

        __parser_ParseTreeClearChildren(parse_tree);

        *tokenizer = start;
        if (ParseUnaryOperator(tokenizer, &parse_tree->children[0]) &&
            ParseCastExpression(tokenizer, &parse_tree->children[1])) {
                return true;
        }

        __parser_ParseTreeClearChildren(parse_tree);

        *tokenizer = start;
        tokens[0] = GetToken(tokenizer);
        if (Token_Keyword == tokens[0].type &&
            gs_StringIsEqual("sizeof", tokens[0].text, gs_StringLength("sizeof"))) {
                ParseTreeSet(&parse_tree->children[0], ParseTreeNode_Keyword, tokens[0]);

                Tokenizer Previous = *tokenizer;
                if (ParseUnaryExpression(tokenizer, &parse_tree->children[1])) {
                        return true;
                }

                *tokenizer = Previous;
                if (Token_OpenParen == (tokens[0] = GetToken(tokenizer)).type &&
                    ParseTypeName(tokenizer, &parse_tree->children[2]) &&
                    Token_CloseParen == (tokens[1] = GetToken(tokenizer)).type) {
                        ParseTreeSet(&parse_tree->children[1], ParseTreeNode_Symbol, tokens[0]);
                        ParseTreeSet(&parse_tree->children[3], ParseTreeNode_Symbol, tokens[1]);
                        return true;
                }
        }

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

        __parser_ParseTreeUpdate(parse_tree, ParseTreeNode_CastExpression, 4);

        if (ParseUnaryExpression(tokenizer, &parse_tree->children[0])) {
                return true;
        }

        *tokenizer = start;
        if (Token_OpenParen == (tokens[0] = GetToken(tokenizer)).type &&
            ParseTypeName(tokenizer, &parse_tree->children[1]) &&
            Token_CloseParen == (tokens[1] = GetToken(tokenizer)).type &&
            ParseCastExpression(tokenizer, &parse_tree->children[3])) {
                ParseTreeSet(&parse_tree->children[0], ParseTreeNode_Symbol, tokens[0]);
                ParseTreeSet(&parse_tree->children[2], ParseTreeNode_Symbol, tokens[1]);
                return true;
        }

        *tokenizer = start;
        return false;
}

bool ParseMultiplicativeExpressionI(Tokenizer *tokenizer, ParseTreeNode *parse_tree) {
        Tokenizer start = *tokenizer;
        Token token;

        __parser_ParseTreeUpdate(parse_tree, ParseTreeNode_MultiplicativeExpression, 3);

        if (Token_Asterisk == (token = GetToken(tokenizer)).type &&
            ParseCastExpression(tokenizer, &parse_tree->children[1]) &&
            ParseMultiplicativeExpressionI(tokenizer, &parse_tree->children[2])) {
                ParseTreeSet(&parse_tree->children[0], ParseTreeNode_Symbol, token);
                return true;
        }

        *tokenizer = start;
        if (Token_Slash == (token = GetToken(tokenizer)).type &&
            ParseCastExpression(tokenizer, &parse_tree->children[1]) &&
            ParseMultiplicativeExpressionI(tokenizer, &parse_tree->children[2])) {
                ParseTreeSet(&parse_tree->children[0], ParseTreeNode_Symbol, token);
                return true;
        }

        *tokenizer = start;
        if (Token_PercentSign == (token = GetToken(tokenizer)).type &&
            ParseCastExpression(tokenizer, &parse_tree->children[1]) &&
            ParseMultiplicativeExpressionI(tokenizer, &parse_tree->children[2])) {
                ParseTreeSet(&parse_tree->children[0], ParseTreeNode_Symbol, token);
                return true;
        }

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

        __parser_ParseTreeUpdate(parse_tree, ParseTreeNode_MultiplicativeExpression, 2);

        if (ParseCastExpression(tokenizer, &parse_tree->children[0]) &&
            ParseMultiplicativeExpressionI(tokenizer, &parse_tree->children[1])) {
                return true;
        }

        *tokenizer = start;
        return false;
}

bool ParseAdditiveExpressionI(Tokenizer *tokenizer, ParseTreeNode *parse_tree) {
        Tokenizer start = *tokenizer;
        Token token;

        __parser_ParseTreeUpdate(parse_tree, ParseTreeNode_AdditiveExpressionI, 3);

        if (Token_Cross == (token = GetToken(tokenizer)).type &&
            ParseMultiplicativeExpression(tokenizer, &parse_tree->children[1]) &&
            ParseAdditiveExpressionI(tokenizer, &parse_tree->children[2])) {
                ParseTreeSet(&parse_tree->children[0], ParseTreeNode_Symbol, token);
                return true;
        }

        *tokenizer = start;
        if (Token_Dash == (token = GetToken(tokenizer)).type &&
            ParseMultiplicativeExpression(tokenizer, &parse_tree->children[1]) &&
            ParseAdditiveExpressionI(tokenizer, &parse_tree->children[2])) {
                ParseTreeSet(&parse_tree->children[0], ParseTreeNode_Symbol, token);
                return true;
        }

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

        __parser_ParseTreeUpdate(parse_tree, ParseTreeNode_AdditiveExpression, 2);

        if (ParseMultiplicativeExpression(tokenizer, &parse_tree->children[0]) &&
            ParseAdditiveExpressionI(tokenizer, &parse_tree->children[1])) {
                return true;
        }

        *tokenizer = start;
        return false;
}

bool ParseShiftExpressionI(Tokenizer *tokenizer, ParseTreeNode *parse_tree) {
        Tokenizer start = *tokenizer;
        Token token;

        __parser_ParseTreeUpdate(parse_tree, ParseTreeNode_ShiftExpressionI, 3);

        if (Token_BitShiftLeft == (token = GetToken(tokenizer)).type &&
            ParseAdditiveExpression(tokenizer, &parse_tree->children[1]) &&
            ParseShiftExpressionI(tokenizer, &parse_tree->children[2])) {
                ParseTreeSet(&parse_tree->children[0], ParseTreeNode_Symbol, token);
                return true;
        }

        *tokenizer = start;
        if (Token_BitShiftRight == (token = GetToken(tokenizer)).type &&
            ParseAdditiveExpression(tokenizer, &parse_tree->children[1]) &&
            ParseShiftExpressionI(tokenizer, &parse_tree->children[2])) {
                ParseTreeSet(&parse_tree->children[0], ParseTreeNode_Symbol, token);
                return true;
        }

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

        __parser_ParseTreeUpdate(parse_tree, ParseTreeNode_ShiftExpression, 2);

        if (ParseAdditiveExpression(tokenizer, &parse_tree->children[0]) &&
            ParseShiftExpressionI(tokenizer, &parse_tree->children[1])) {
                return true;
        }

        *tokenizer = start;
        return false;
}

bool ParseRelationalExpressionI(Tokenizer *tokenizer, ParseTreeNode *parse_tree) {
        Tokenizer start = *tokenizer;
        Token token;

        __parser_ParseTreeUpdate(parse_tree, ParseTreeNode_RelationalExpressionI, 3);

        if (Token_LessThan == (token = GetToken(tokenizer)).type &&
            ParseShiftExpression(tokenizer, &parse_tree->children[1]) &&
            ParseRelationalExpressionI(tokenizer, &parse_tree->children[2])) {
                ParseTreeSet(&parse_tree->children[0], ParseTreeNode_Symbol, token);
                return true;
        }

        *tokenizer = start;
        if (Token_GreaterThan == (token = GetToken(tokenizer)).type &&
            ParseShiftExpression(tokenizer, &parse_tree->children[1]) &&
            ParseRelationalExpressionI(tokenizer, &parse_tree->children[2])) {
                ParseTreeSet(&parse_tree->children[0], ParseTreeNode_Symbol, token);
                return true;
        }

        *tokenizer = start;
        if (Token_LessThanEqual == (token = GetToken(tokenizer)).type &&
            ParseShiftExpression(tokenizer, &parse_tree->children[1]) &&
            ParseRelationalExpressionI(tokenizer, &parse_tree->children[2])) {
                ParseTreeSet(&parse_tree->children[0], ParseTreeNode_Symbol, token);
                return true;
        }

        *tokenizer = start;
        if (Token_GreaterThanEqual == (token = GetToken(tokenizer)).type &&
            ParseShiftExpression(tokenizer, &parse_tree->children[1]) &&
            ParseRelationalExpressionI(tokenizer, &parse_tree->children[2])) {
                ParseTreeSet(&parse_tree->children[0], ParseTreeNode_Symbol, token);
                return true;
        }

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

        __parser_ParseTreeUpdate(parse_tree, ParseTreeNode_RelationalExpression, 2);

        if (ParseShiftExpression(tokenizer, &parse_tree->children[0]) &&
            ParseRelationalExpressionI(tokenizer, &parse_tree->children[1])) {
                return true;
        }

        *tokenizer = start;
        return false;
}

bool ParseEqualityExpressionI(Tokenizer *tokenizer, ParseTreeNode *parse_tree) {
        Tokenizer start = *tokenizer;
        Token token;

        __parser_ParseTreeUpdate(parse_tree, ParseTreeNode_EqualityExpressionI, 3);

        if (Token_LogicalEqual == (token = GetToken(tokenizer)).type &&
            ParseRelationalExpression(tokenizer, &parse_tree->children[1]) &&
            ParseEqualityExpressionI(tokenizer, &parse_tree->children[2])) {
                ParseTreeSet(&parse_tree->children[0], ParseTreeNode_Symbol, token);
                return true;
        }

        *tokenizer = start;
        if (Token_NotEqual == (token = GetToken(tokenizer)).type &&
            ParseRelationalExpression(tokenizer, &parse_tree->children[1]) &&
            ParseEqualityExpressionI(tokenizer, &parse_tree->children[2])) {
                ParseTreeSet(&parse_tree->children[0], ParseTreeNode_Symbol, token);
                return true;
        }

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

        __parser_ParseTreeUpdate(parse_tree, ParseTreeNode_EqualityExpression, 2);

        if (ParseRelationalExpression(tokenizer, &parse_tree->children[0]) &&
            ParseEqualityExpressionI(tokenizer, &parse_tree->children[1])) {
                return true;
        }

        *tokenizer = start;
        return false;
}

bool ParseAndExpressionI(Tokenizer *tokenizer, ParseTreeNode *parse_tree) {
        Tokenizer start = *tokenizer;
        Token token;

        __parser_ParseTreeUpdate(parse_tree, ParseTreeNode_AndExpressionI, 3);

        if (Token_Ampersand == (token = GetToken(tokenizer)).type &&
            ParseEqualityExpression(tokenizer, &parse_tree->children[1]) &&
            ParseAndExpressionI(tokenizer, &parse_tree->children[2])) {
                ParseTreeSet(&parse_tree->children[0], ParseTreeNode_Symbol, token);
                return true;
        }

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

        __parser_ParseTreeUpdate(parse_tree, ParseTreeNode_AndExpression, 2);

        if (ParseEqualityExpression(tokenizer, &parse_tree->children[0]) &&
            ParseAndExpressionI(tokenizer, &parse_tree->children[1])) {
                return true;
        }

        *tokenizer = start;
        return false;
}

bool ParseExclusiveOrExpressionI(Tokenizer *tokenizer, ParseTreeNode *parse_tree) {
        Tokenizer start = *tokenizer;
        Token token;

        __parser_ParseTreeUpdate(parse_tree, ParseTreeNode_ExclusiveOrExpressionI, 3);

        if (Token_Carat == (token = GetToken(tokenizer)).type &&
            ParseAndExpression(tokenizer, &parse_tree->children[1]) &&
            ParseExclusiveOrExpressionI(tokenizer, &parse_tree->children[2])) {
                ParseTreeSet(&parse_tree->children[0], ParseTreeNode_Symbol, token);
                return true;
        }

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

        __parser_ParseTreeUpdate(parse_tree, ParseTreeNode_ExclusiveOrExpression, 2);

        if (ParseAndExpression(tokenizer, &parse_tree->children[0]) &&
            ParseExclusiveOrExpressionI(tokenizer, &parse_tree->children[1])) {
                return true;
        }

        *tokenizer = start;
        return false;
}

bool ParseInclusiveOrExpressionI(Tokenizer *tokenizer, ParseTreeNode *parse_tree) {
        Tokenizer start = *tokenizer;
        Token token;

        __parser_ParseTreeUpdate(parse_tree, ParseTreeNode_InclusiveOrExpressionI, 3);

        if (Token_Pipe == (token = GetToken(tokenizer)).type &&
            ParseExclusiveOrExpression(tokenizer, &parse_tree->children[1]) &&
            ParseInclusiveOrExpressionI(tokenizer, &parse_tree->children[2])) {
                ParseTreeSet(&parse_tree->children[0], ParseTreeNode_Symbol, token);
                return true;
        }

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

        __parser_ParseTreeUpdate(parse_tree, ParseTreeNode_InclusiveOrExpression, 2);

        if (ParseExclusiveOrExpression(tokenizer, &parse_tree->children[0]) &&
            ParseInclusiveOrExpressionI(tokenizer, &parse_tree->children[1])) {
                return true;
        }

        *tokenizer = start;
        return false;
}

bool ParseLogicalAndExpressionI(Tokenizer *tokenizer, ParseTreeNode *parse_tree) {
        Tokenizer start = *tokenizer;
        Token token;

        __parser_ParseTreeUpdate(parse_tree, ParseTreeNode_LogicalAndExpressionI, 3);

        if (Token_LogicalAnd == (token = GetToken(tokenizer)).type &&
            ParseInclusiveOrExpression(tokenizer, &parse_tree->children[1]) &&
            ParseLogicalAndExpressionI(tokenizer, &parse_tree->children[2])) {
                ParseTreeSet(&parse_tree->children[0], ParseTreeNode_Symbol, token);
                return true;
        }

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

        __parser_ParseTreeUpdate(parse_tree, ParseTreeNode_LogicalAndExpression, 2);

        if (ParseInclusiveOrExpression(tokenizer, &parse_tree->children[0]) &&
            ParseLogicalAndExpressionI(tokenizer, &parse_tree->children[1])) {
                return true;
        }

        *tokenizer = start;
        return false;
}

bool ParseLogicalOrExpressionI(Tokenizer *tokenizer, ParseTreeNode *parse_tree) {
        Tokenizer start = *tokenizer;
        Token token;

        __parser_ParseTreeUpdate(parse_tree, ParseTreeNode_LogicalOrExpressionI, 3);

        if (Token_LogicalOr == (token = GetToken(tokenizer)).type &&
            ParseLogicalAndExpression(tokenizer, &parse_tree->children[1]) &&
            ParseLogicalOrExpressionI(tokenizer, &parse_tree->children[2])) {
                ParseTreeSet(&parse_tree->children[0], ParseTreeNode_Symbol, token);
                return true;
        }

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

        __parser_ParseTreeUpdate(parse_tree, ParseTreeNode_LogicalOrExpression, 2);

        if (ParseLogicalAndExpression(tokenizer, &parse_tree->children[0]) &&
            ParseLogicalOrExpressionI(tokenizer, &parse_tree->children[1])) {
                return true;
        }

        *tokenizer = start;
        return false;
}

/*
  constant-expression:
  conditional-expression
*/
bool ParseConstantExpression(Tokenizer *tokenizer, ParseTreeNode *parse_tree) {
        Tokenizer start = *tokenizer;

        __parser_ParseTreeUpdate(parse_tree, ParseTreeNode_ConstantExpression, 1);

        if (ParseConditionalExpression(tokenizer, &parse_tree->children[0])) {
                return true;
        }

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

        __parser_ParseTreeUpdate(parse_tree, ParseTreeNode_ConditionalExpression, 5);

        if (ParseLogicalOrExpression(tokenizer, &parse_tree->children[0]) &&
            Token_QuestionMark == (tokens[0] = GetToken(tokenizer)).type &&
            ParseExpression(tokenizer, &parse_tree->children[2]) &&
            Token_Colon == (tokens[1] = GetToken(tokenizer)).type &&
            ParseConditionalExpression(tokenizer, &parse_tree->children[4])) {
                ParseTreeSet(&parse_tree->children[1], ParseTreeNode_Symbol, tokens[0]);
                ParseTreeSet(&parse_tree->children[3], ParseTreeNode_Symbol, tokens[1]);
                return true;
        }

        __parser_ParseTreeClearChildren(parse_tree);

        *tokenizer = start;
        if (ParseLogicalOrExpression(tokenizer, &parse_tree->children[0])) {
                return true;
        }

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

        __parser_ParseTreeUpdate(parse_tree, ParseTreeNode_AssignmentExpression, 3);

        if (ParseUnaryExpression(tokenizer, &parse_tree->children[0]) &&
            ParseAssignmentOperator(tokenizer, &parse_tree->children[1]) &&
            ParseAssignmentExpression(tokenizer, &parse_tree->children[2])) {
                return true;
        }

        __parser_ParseTreeClearChildren(parse_tree);

        *tokenizer = start;
        if (ParseConditionalExpression(tokenizer, &parse_tree->children[0])) {
                return true;
        }

        *tokenizer = start;
        return false;
}

bool ParseExpressionI(Tokenizer *tokenizer, ParseTreeNode *parse_tree) {
        Tokenizer start = *tokenizer;
        Token token;

        __parser_ParseTreeUpdate(parse_tree, ParseTreeNode_ExpressionI, 3);

        if (Token_Comma == (token = GetToken(tokenizer)).type &&
            ParseAssignmentExpression(tokenizer, &parse_tree->children[1]) &&
            ParseExpressionI(tokenizer, &parse_tree->children[2])) {
                ParseTreeSet(&parse_tree->children[0], ParseTreeNode_Symbol, token);
                return true;
        }

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

        __parser_ParseTreeUpdate(parse_tree, ParseTreeNode_Expression, 2);

        if (ParseAssignmentExpression(tokenizer, &parse_tree->children[0]) &&
            ParseExpressionI(tokenizer, &parse_tree->children[1])) {
                return true;
        }

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

        __parser_ParseTreeUpdate(parse_tree, ParseTreeNode_JumpStatement, 3);

        if (Token_Keyword == tokens[0].type &&
            gs_StringIsEqual("goto", tokens[0].text, tokens[0].text_length) &&
            ParseIdentifier(tokenizer, &parse_tree->children[1]) &&
            Token_SemiColon == (tokens[1] = GetToken(tokenizer)).type) {
                ParseTreeSet(&parse_tree->children[0], ParseTreeNode_Keyword, tokens[0]);
                ParseTreeSet(&parse_tree->children[2], ParseTreeNode_Symbol, tokens[1]);
                return true;
        }

        __parser_ParseTreeClearChildren(parse_tree);

        *tokenizer = at_token;
        if (Token_Keyword == tokens[0].type &&
            gs_StringIsEqual("continue", tokens[0].text, tokens[0].text_length) &&
            Token_SemiColon == (tokens[1] = GetToken(tokenizer)).type) {
                ParseTreeSet(&parse_tree->children[0], ParseTreeNode_Keyword, tokens[0]);
                ParseTreeSet(&parse_tree->children[1], ParseTreeNode_Symbol, tokens[1]);
                return true;
        }

        __parser_ParseTreeClearChildren(parse_tree);

        *tokenizer = at_token;
        if (Token_Keyword == tokens[0].type &&
            gs_StringIsEqual("break", tokens[0].text, tokens[0].text_length) &&
            Token_SemiColon == (tokens[1] = GetToken(tokenizer)).type) {
                ParseTreeSet(&parse_tree->children[0], ParseTreeNode_Keyword, tokens[0]);
                ParseTreeSet(&parse_tree->children[1], ParseTreeNode_Symbol, tokens[1]);
                return true;
        }

        __parser_ParseTreeClearChildren(parse_tree);

        *tokenizer = at_token;
        if (Token_Keyword == tokens[0].type &&
            gs_StringIsEqual("return", tokens[0].text, tokens[0].text_length)) {
                ParseTreeSet(&parse_tree->children[0], ParseTreeNode_Keyword, tokens[0]);
                int ChildIndex = 1;
                Tokenizer Previous = *tokenizer;

                if (!ParseExpression(tokenizer, &parse_tree->children[ChildIndex++])) {
                        --ChildIndex;
                        *tokenizer = Previous;
                }

                if (Token_SemiColon == (tokens[1] = GetToken(tokenizer)).type) {
                        ParseTreeSet(&parse_tree->children[ChildIndex], ParseTreeNode_Symbol, tokens[1]);
                        return true;
                }
        }

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

        __parser_ParseTreeUpdate(parse_tree, ParseTreeNode_IterationStatement, 10); /* TODO(AARON): Magic Number! */

        if (Token_Keyword == tokens[0].type &&
            gs_StringIsEqual("while", tokens[0].text, tokens[0].text_length) &&
            Token_OpenParen == (tokens[1] = GetToken(tokenizer)).type &&
            ParseExpression(tokenizer, &parse_tree->children[2]) &&
            Token_CloseParen == (tokens[2] = GetToken(tokenizer)).type &&
            ParseStatement(tokenizer, &parse_tree->children[4])) {
                ParseTreeSet(&parse_tree->children[0], ParseTreeNode_Keyword, tokens[0]);
                ParseTreeSet(&parse_tree->children[1], ParseTreeNode_Symbol, tokens[1]);
                ParseTreeSet(&parse_tree->children[3], ParseTreeNode_Symbol, tokens[2]);
                return true;
        }

        __parser_ParseTreeClearChildren(parse_tree);

        *tokenizer = at_token;
        if (Token_Keyword == tokens[0].type &&
            gs_StringIsEqual("do", tokens[0].text, tokens[0].text_length) &&
            ParseStatement(tokenizer, &parse_tree->children[1])) {
                ParseTreeNode *child_node = &parse_tree->children[0];
                ParseTreeSet(child_node, ParseTreeNode_Keyword, tokens[0]);

                tokens[1] = GetToken(tokenizer);
                if (Token_Keyword == tokens[1].type &&
                    gs_StringIsEqual("while", tokens[1].text, tokens[1].text_length) &&
                    Token_OpenParen == (tokens[2] = GetToken(tokenizer)).type &&
                    ParseExpression(tokenizer, &parse_tree->children[4]) &&
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

        __parser_ParseTreeClearChildren(parse_tree);

        *tokenizer = at_token;
        if (Token_Keyword == tokens[0].type &&
            gs_StringIsEqual("for", tokens[0].text, tokens[0].text_length) &&
            Token_OpenParen == (tokens[1] = GetToken(tokenizer)).type) {
                ParseTreeSet(&parse_tree->children[0], ParseTreeNode_Keyword, tokens[0]);
                ParseTreeSet(&parse_tree->children[1], ParseTreeNode_Symbol, tokens[1]);

                int i = 2;

                Tokenizer Previous = *tokenizer;
                if (!ParseExpression(tokenizer, &parse_tree->children[i])) {
                        *tokenizer = Previous;
                } else {
                        i++;
                }

                if (Token_SemiColon != (tokens[2] = GetToken(tokenizer)).type) {
                        *tokenizer = start;
                        return false;
                }
                ParseTreeSet(&parse_tree->children[i++], ParseTreeNode_Symbol, tokens[2]);

                Previous = *tokenizer;
                if (!ParseExpression(tokenizer, &parse_tree->children[i++])) {
                        --i;
                        *tokenizer = Previous;
                }

                if (Token_SemiColon != (tokens[3] = GetToken(tokenizer)).type) {
                        *tokenizer = start;
                        return false;
                }
                ParseTreeSet(&parse_tree->children[i++], ParseTreeNode_Symbol, tokens[3]);

                Previous = *tokenizer;
                if (!ParseExpression(tokenizer, &parse_tree->children[i++])) {
                        --i;
                        *tokenizer = Previous;
                }

                if (Token_CloseParen == (tokens[4] = GetToken(tokenizer)).type &&
                    ParseStatement(tokenizer, &parse_tree->children[i + 1])) {
                        ParseTreeSet(&parse_tree->children[i], ParseTreeNode_Symbol, tokens[4]);
                        return true;
                }
        }

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

        __parser_ParseTreeUpdate(parse_tree, ParseTreeNode_SelectionStatement, 6);

        if (Token_Keyword == tokens[0].type &&
            gs_StringIsEqual("if", tokens[0].text, tokens[0].text_length) &&
            Token_OpenParen == (tokens[1] = GetToken(tokenizer)).type &&
            ParseExpression(tokenizer, &parse_tree->children[2]) &&
            Token_CloseParen == (tokens[2] = GetToken(tokenizer)).type &&
            ParseStatement(tokenizer, &parse_tree->children[4])) {
                Tokenizer at_else = *tokenizer;
                Token token = GetToken(tokenizer);

                ParseTreeSet(&parse_tree->children[0], ParseTreeNode_Keyword, tokens[0]);
                ParseTreeSet(&parse_tree->children[1], ParseTreeNode_Symbol, tokens[1]);
                ParseTreeSet(&parse_tree->children[3], ParseTreeNode_Symbol, tokens[2]);

                if (Token_Keyword == token.type &&
                    gs_StringIsEqual("else", token.text, token.text_length) &&
                    ParseStatement(tokenizer, &parse_tree->children[5])) {
                        ParseTreeSet(&parse_tree->children[4], ParseTreeNode_Keyword, token);
                        return true;
                }

                *tokenizer = at_else;
                return true;
        }

        __parser_ParseTreeClearChildren(parse_tree);

        *tokenizer = at_token;
        if (Token_Keyword == tokens[0].type &&
            gs_StringIsEqual("switch", tokens[0].text, tokens[0].text_length) &&
            Token_OpenParen == (tokens[1] = GetToken(tokenizer)).type &&
            ParseExpression(tokenizer, &parse_tree->children[2]) &&
            Token_CloseParen == (tokens[2] = GetToken(tokenizer)).type &&
            ParseStatement(tokenizer, &parse_tree->children[4])) {
                ParseTreeSet(&parse_tree->children[0], ParseTreeNode_Keyword, tokens[0]);
                ParseTreeSet(&parse_tree->children[1], ParseTreeNode_Symbol, tokens[1]);
                ParseTreeSet(&parse_tree->children[3], ParseTreeNode_Symbol, tokens[2]);
                return true;
        }

        *tokenizer = start;
        return false;
}

bool ParseStatementListI(Tokenizer *tokenizer, ParseTreeNode *parse_tree) {
        Tokenizer start = *tokenizer;

        __parser_ParseTreeUpdate(parse_tree, ParseTreeNode_StatementListI, 2);

        if (ParseStatement(tokenizer, &parse_tree->children[0]) &&
            ParseStatementListI(tokenizer, &parse_tree->children[1])) {
                return true;
        }

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

        __parser_ParseTreeUpdate(parse_tree, ParseTreeNode_StatementList, 2);

        if (ParseStatement(tokenizer, &parse_tree->children[0]) &&
            ParseStatementListI(tokenizer, &parse_tree->children[1])) {
                return true;
        }

        *tokenizer = start;
        return false;
}

/*
  compound-statement:
  { declaration-list(opt) statement-list(opt) }
*/
bool ParseCompoundStatement(Tokenizer *tokenizer, ParseTreeNode *parse_tree) {
        Tokenizer start = *tokenizer;
        Token token;

        __parser_ParseTreeUpdate(parse_tree, ParseTreeNode_CompoundStatement, 3);

        if (Token_OpenBrace == (token = GetToken(tokenizer)).type) {
                ParseTreeSet(&parse_tree->children[0], ParseTreeNode_Symbol, token);
                int i = 0;

                Tokenizer Previous = *tokenizer;
                if (!ParseDeclarationList(tokenizer, &parse_tree->children[i++])) {
                        --i;
                        *tokenizer = Previous;
                }

                Previous = *tokenizer;
                if (!ParseStatementList(tokenizer, &parse_tree->children[i++])) {
                        --i;
                        *tokenizer = Previous;
                }

                if (Token_CloseBrace == (token = GetToken(tokenizer)).type) {
                        ParseTreeSet(&parse_tree->children[i], ParseTreeNode_Symbol, token);
                        return true;
                }
        }

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

        __parser_ParseTreeUpdate(parse_tree, ParseTreeNode_ExpressionStatement, 2);

        if (ParseExpression(tokenizer, &parse_tree->children[0]) &&
            Token_SemiColon == (token = GetToken(tokenizer)).type) {
                ParseTreeSet(&parse_tree->children[1], ParseTreeNode_Symbol, token);
                return true;
        }

        __parser_ParseTreeClearChildren(parse_tree);

        *tokenizer = start;
        if (Token_SemiColon == (token = GetToken(tokenizer)).type) {
                ParseTreeSet(&parse_tree->children[0], ParseTreeNode_Symbol, token);
                return true;
        }

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

        __parser_ParseTreeUpdate(parse_tree, ParseTreeNode_LabeledStatement, 4);

        if (ParseIdentifier(tokenizer, &parse_tree->children[0]) &&
            Token_Colon == (tokens[0] = GetToken(tokenizer)).type &&
            ParseStatement(tokenizer, &parse_tree->children[2])) {
                ParseTreeSet(&parse_tree->children[1], ParseTreeNode_Symbol, tokens[0]);
                return true;
        }

        *tokenizer = start;
        tokens[0] = GetToken(tokenizer);
        Tokenizer at_token = *tokenizer;

        if (Token_Keyword == tokens[0].type &&
            gs_StringIsEqual("case", tokens[0].text, tokens[0].text_length) &&
            ParseConstantExpression(tokenizer, &parse_tree->children[1]) &&
            Token_Colon == (tokens[1] = GetToken(tokenizer)).type &&
            ParseStatement(tokenizer, &parse_tree->children[3])) {
                ParseTreeSet(&parse_tree->children[0], ParseTreeNode_Keyword, tokens[0]);
                ParseTreeSet(&parse_tree->children[2], ParseTreeNode_Symbol, tokens[1]);
                return true;
        }

        __parser_ParseTreeClearChildren(parse_tree);

        *tokenizer = at_token;
        if (Token_Keyword == tokens[0].type &&
            gs_StringIsEqual("default", tokens[0].text, tokens[0].text_length) &&
            Token_Colon == (tokens[1] = GetToken(tokenizer)).type &&
            ParseStatement(tokenizer, &parse_tree->children[2])) {
                ParseTreeSet(&parse_tree->children[0], ParseTreeNode_Keyword, tokens[0]);
                ParseTreeSet(&parse_tree->children[2], ParseTreeNode_Symbol, tokens[1]);
                return true;
        }

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

        __parser_ParseTreeUpdate(parse_tree, ParseTreeNode_Statement, 1);
        ParseTreeNode *child = &parse_tree->children[0];

        if (ParseLabeledStatement(tokenizer, child)) return true;

        *tokenizer = start;
        if (ParseExpressionStatement(tokenizer, child)) return true;

        *tokenizer = start;
        if (ParseCompoundStatement(tokenizer, child)) return true;

        *tokenizer = start;
        if (ParseSelectionStatement(tokenizer, child)) return true;

        *tokenizer = start;
        if (ParseIterationStatement(tokenizer, child)) return true;

        *tokenizer = start;
        if (ParseJumpStatement(tokenizer, child)) return true;

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
        *tokenizer = start;

        __parser_ParseTreeUpdate(parse_tree, ParseTreeNode_TypedefName, 1);

        if (ParseIdentifier(tokenizer, &parse_tree->children[0]) && TypedefIsName(token)) {
                ParseTreeSet(parse_tree, ParseTreeNode_TypedefName, token);
                return true;
        }

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

        __parser_ParseTreeUpdate(parse_tree, ParseTreeNode_DirectAbstractDeclaratorI, 4);

        if (Token_OpenBracket == (tokens[0] = GetToken(tokenizer)).type &&
            ParseConstantExpression(tokenizer, &parse_tree->children[1]) &&
            Token_CloseBracket == (tokens[1] = GetToken(tokenizer)).type &&
            ParseDirectAbstractDeclaratorI(tokenizer, &parse_tree->children[2])) {
                ParseTreeSet(&parse_tree->children[0], ParseTreeNode_Symbol, tokens[0]);
                ParseTreeSet(&parse_tree->children[2], ParseTreeNode_Symbol, tokens[1]);
                return true;
        }

        __parser_ParseTreeClearChildren(parse_tree);

        *tokenizer = start;
        if (Token_OpenBracket == (tokens[0] = GetToken(tokenizer)).type &&
            Token_CloseBracket == (tokens[1] = GetToken(tokenizer)).type &&
            ParseDirectAbstractDeclaratorI(tokenizer, &parse_tree->children[2])) {
                ParseTreeSet(&parse_tree->children[0], ParseTreeNode_Symbol, tokens[0]);
                ParseTreeSet(&parse_tree->children[1], ParseTreeNode_Symbol, tokens[1]);
                return true;
        }

        *tokenizer = start;
        if (Token_OpenParen == (tokens[0] = GetToken(tokenizer)).type &&
            ParseParameterTypeList(tokenizer, &parse_tree->children[1]) &&
            Token_CloseParen == (tokens[1] = GetToken(tokenizer)).type &&
            ParseDirectAbstractDeclaratorI(tokenizer, &parse_tree->children[3])) {
                ParseTreeSet(&parse_tree->children[0], ParseTreeNode_Symbol, tokens[0]);
                ParseTreeSet(&parse_tree->children[2], ParseTreeNode_Symbol, tokens[1]);
                return true;
        }

        __parser_ParseTreeClearChildren(parse_tree);

        *tokenizer = start;
        if (Token_OpenParen == (tokens[0] = GetToken(tokenizer)).type &&
            Token_CloseParen == (tokens[1] = GetToken(tokenizer)).type &&
            ParseDirectAbstractDeclaratorI(tokenizer, &parse_tree->children[2])) {
                ParseTreeSet(&parse_tree->children[0], ParseTreeNode_Symbol, tokens[0]);
                ParseTreeSet(&parse_tree->children[1], ParseTreeNode_Symbol, tokens[1]);
                return true;
        }

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

        __parser_ParseTreeUpdate(parse_tree, ParseTreeNode_DirectAbstractDeclarator, 4);

        if (Token_OpenParen == (tokens[0] = GetToken(tokenizer)).type &&
            ParseAbstractDeclarator(tokenizer, &parse_tree->children[1]) &&
            Token_CloseParen == (tokens[1] = GetToken(tokenizer)).type &&
            ParseDirectAbstractDeclaratorI(tokenizer, &parse_tree->children[3])) {
                ParseTreeSet(&parse_tree->children[0], ParseTreeNode_Symbol, tokens[0]);
                ParseTreeSet(&parse_tree->children[2], ParseTreeNode_Symbol, tokens[1]);
                return true;
        }

        *tokenizer = start;
        if (Token_OpenBracket == (tokens[0] = GetToken(tokenizer)).type &&
            ParseConstantExpression(tokenizer, &parse_tree->children[1]) &&
            Token_CloseBracket == (tokens[1] = GetToken(tokenizer)).type &&
            ParseDirectAbstractDeclaratorI(tokenizer, &parse_tree->children[3])) {
                ParseTreeSet(&parse_tree->children[0], ParseTreeNode_Symbol, tokens[0]);
                ParseTreeSet(&parse_tree->children[2], ParseTreeNode_Symbol, tokens[1]);
                return true;
        }

        __parser_ParseTreeClearChildren(parse_tree);

        *tokenizer = start;
        if (Token_OpenBracket == (tokens[0] = GetToken(tokenizer)).type &&
            Token_CloseBracket == (tokens[1] = GetToken(tokenizer)).type &&
            ParseDirectAbstractDeclaratorI(tokenizer, &parse_tree->children[2])) {
                ParseTreeSet(&parse_tree->children[0], ParseTreeNode_Symbol, tokens[0]);
                ParseTreeSet(&parse_tree->children[1], ParseTreeNode_Symbol, tokens[1]);
                return true;
        }

        *tokenizer = start;
        if (Token_OpenParen == (tokens[0] = GetToken(tokenizer)).type &&
            ParseParameterTypeList(tokenizer, &parse_tree->children[1]) &&
            Token_CloseParen == (tokens[1] = GetToken(tokenizer)).type &&
            ParseDirectAbstractDeclaratorI(tokenizer, &parse_tree->children[2])) {
                ParseTreeSet(&parse_tree->children[0], ParseTreeNode_Symbol, tokens[0]);
                ParseTreeSet(&parse_tree->children[2], ParseTreeNode_Symbol, tokens[1]);
                return true;
        }

        *tokenizer = start;
        if (Token_OpenParen == (tokens[0] = GetToken(tokenizer)).type &&
            Token_CloseParen == (tokens[1] = GetToken(tokenizer)).type &&
            ParseDirectAbstractDeclaratorI(tokenizer, &parse_tree->children[2])) {
                ParseTreeSet(&parse_tree->children[0], ParseTreeNode_Symbol, tokens[0]);
                ParseTreeSet(&parse_tree->children[1], ParseTreeNode_Symbol, tokens[1]);
                return true;
        }

        *tokenizer = start;
        return false;
}

/*
  abstract-declarator:
  pointer
  pointer(opt) direct-abstract-declarator
*/
bool ParseAbstractDeclarator(Tokenizer *tokenizer, ParseTreeNode *parse_tree) {
        Tokenizer start = *tokenizer;

        __parser_ParseTreeUpdate(parse_tree, ParseTreeNode_AbstractDeclarator, 2);

        if (ParsePointer(tokenizer, &parse_tree->children[0]) &&
            ParseDirectAbstractDeclarator(tokenizer, &parse_tree->children[1])) {
                return true;
        }

        __parser_ParseTreeClearChildren(parse_tree);

        *tokenizer = start;
        if (ParseDirectAbstractDeclarator(tokenizer, &parse_tree->children[0])) {
                return true;
        }

        *tokenizer = start;
        if (ParsePointer(tokenizer, &parse_tree->children[0])) {
                return true;
        }

        *tokenizer = start;
        return false;
}

/*
  type-name:
  specifier-qualifier-list abstract-declarator(opt)
*/
bool ParseTypeName(Tokenizer *tokenizer, ParseTreeNode *parse_tree) {
        Tokenizer start = *tokenizer;

        __parser_ParseTreeUpdate(parse_tree, ParseTreeNode_TypeName, 2);

        if (ParseSpecifierQualifierList(tokenizer, &parse_tree->children[0]) &&
            ParseAbstractDeclarator(tokenizer, &parse_tree->children[1])) {
                return true;
        }

        __parser_ParseTreeClearChildren(parse_tree);

        *tokenizer = start;
        if (ParseSpecifierQualifierList(tokenizer, &parse_tree->children[0])) {
                return true;
        }

        *tokenizer = start;
        return false;
}

bool ParseInitializerListI(Tokenizer *tokenizer, ParseTreeNode *parse_tree) {
        Tokenizer start = *tokenizer;
        Token token;

        __parser_ParseTreeUpdate(parse_tree, ParseTreeNode_InitializerListI, 3);

        if (Token_Comma == (token = GetToken(tokenizer)).type &&
            ParseInitializer(tokenizer, &parse_tree->children[1]) &&
            ParseInitializerListI(tokenizer, &parse_tree->children[2])) {
                ParseTreeSet(&parse_tree->children[0], ParseTreeNode_Symbol, token);
                return true;
        }

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

        __parser_ParseTreeUpdate(parse_tree, ParseTreeNode_InitializerList, 2);

        if (ParseInitializer(tokenizer, &parse_tree->children[0]) &&
            ParseInitializerListI(tokenizer, &parse_tree->children[1])) {
                return true;
        }

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

        __parser_ParseTreeUpdate(parse_tree, ParseTreeNode_Initializer, 4);

        if (ParseAssignmentExpression(tokenizer, &parse_tree->children[0])) {
                return true;
        }

        *tokenizer = start;
        if (Token_OpenBrace == (tokens[0] = GetToken(tokenizer)).type &&
            ParseInitializerList(tokenizer, &parse_tree->children[1]) &&
            Token_CloseBrace == (tokens[1] = GetToken(tokenizer)).type) {
                ParseTreeSet(&parse_tree->children[0], ParseTreeNode_Symbol, tokens[0]);
                ParseTreeSet(&parse_tree->children[2], ParseTreeNode_Symbol, tokens[1]);
                return true;
        }

        *tokenizer = start;
        if (Token_OpenBrace == (tokens[0] = GetToken(tokenizer)).type &&
            ParseInitializerList(tokenizer, &parse_tree->children[1]) &&
            Token_Comma == (tokens[1] = GetToken(tokenizer)).type &&
            Token_CloseBrace == (tokens[2] = GetToken(tokenizer)).type) {
                ParseTreeSet(&parse_tree->children[0], ParseTreeNode_Symbol, tokens[0]);
                ParseTreeSet(&parse_tree->children[2], ParseTreeNode_Symbol, tokens[1]);
                ParseTreeSet(&parse_tree->children[3], ParseTreeNode_Symbol, tokens[2]);
                return true;
        }

        *tokenizer = start;
        return false;
}

bool ParseIdentifierListI(Tokenizer *tokenizer, ParseTreeNode *parse_tree) {
        Tokenizer start = *tokenizer;
        Token token;

        __parser_ParseTreeUpdate(parse_tree, ParseTreeNode_IdentifierListI, 3);

        if (Token_Comma == (token = GetToken(tokenizer)).type &&
            ParseIdentifier(tokenizer, &parse_tree->children[1]) &&
            ParseIdentifierListI(tokenizer, &parse_tree->children[2])) {
                ParseTreeSet(&parse_tree->children[0], ParseTreeNode_Symbol, token);
                return true;
        }

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

        __parser_ParseTreeUpdate(parse_tree, ParseTreeNode_IdentifierList, 2);

        if (ParseIdentifier(tokenizer, &parse_tree->children[0]) &&
            ParseIdentifierListI(tokenizer, &parse_tree->children[1])) {
                return true;
        }

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

        __parser_ParseTreeUpdate(parse_tree, ParseTreeNode_ParameterDeclaration, 2);

        if (ParseDeclarationSpecifiers(tokenizer, &parse_tree->children[0]) &&
            ParseDeclarator(tokenizer, &parse_tree->children[1])) {
                return true;
        }

        *tokenizer = start;
        if (ParseDeclarationSpecifiers(tokenizer, &parse_tree->children[0]) &&
            ParseAbstractDeclarator(tokenizer, &parse_tree->children[1])) {
                return true;
        }

        __parser_ParseTreeClearChildren(parse_tree);

        *tokenizer = start;
        if (ParseDeclarationSpecifiers(tokenizer, &parse_tree->children[0])) {
                return true;
        }

        *tokenizer = start;
        return false;
}

bool ParseParameterListI(Tokenizer *tokenizer, ParseTreeNode *parse_tree) {
        Tokenizer start = *tokenizer;
        Token token;

        __parser_ParseTreeUpdate(parse_tree, ParseTreeNode_ParameterListI, 3);

        if (Token_Comma == (token = GetToken(tokenizer)).type &&
            ParseParameterDeclaration(tokenizer, &parse_tree->children[1]) &&
            ParseParameterListI(tokenizer, &parse_tree->children[2])) {
                ParseTreeSet(&parse_tree->children[0], ParseTreeNode_Symbol, token);
                return true;
        }

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

        __parser_ParseTreeUpdate(parse_tree, ParseTreeNode_ParameterList, 2);

        if (ParseParameterDeclaration(tokenizer, &parse_tree->children[0]) &&
            ParseParameterListI(tokenizer, &parse_tree->children[1])) {
                return true;
        }

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

        __parser_ParseTreeUpdate(parse_tree, ParseTreeNode_ParameterTypeList, 3);

        if (ParseParameterList(tokenizer, &parse_tree->children[0])) {
                Tokenizer Previous = *tokenizer;
                if (Token_Comma == (tokens[0] = GetToken(tokenizer)).type &&
                    Token_Ellipsis == (tokens[1] = GetToken(tokenizer)).type) {
                        ParseTreeSet(&parse_tree->children[1], ParseTreeNode_Symbol, tokens[0]);
                        ParseTreeSet(&parse_tree->children[2], ParseTreeNode_Symbol, tokens[1]);
                        return true;
                }

                *tokenizer = Previous;
                return true;
        }

        *tokenizer = start;
        return false;
}

bool ParseTypeQualifierListI(Tokenizer *tokenizer, ParseTreeNode *parse_tree) {
        Tokenizer start = *tokenizer;

        __parser_ParseTreeUpdate(parse_tree, ParseTreeNode_TypeQualifierListI, 2);

        if (ParseTypeQualifier(tokenizer, &parse_tree->children[0]) &&
            ParseTypeQualifierListI(tokenizer, &parse_tree->children[1])) {
                return true;
        }

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

        __parser_ParseTreeUpdate(parse_tree, ParseTreeNode_TypeQualifierList, 2);

        if (ParseTypeQualifier(tokenizer, &parse_tree->children[0]) &&
            ParseTypeQualifierListI(tokenizer, &parse_tree->children[1])) {
                return true;
        }

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

        __parser_ParseTreeUpdate(parse_tree, ParseTreeNode_Pointer, 2);

        if (Token_Asterisk != token.type) {
                *tokenizer = start;
                return false;
        }

        ParseTreeSet(parse_tree, ParseTreeNode_Pointer, token);

        if (ParseTypeQualifierList(tokenizer, &parse_tree->children[0]) &&
            ParsePointer(tokenizer, &parse_tree->children[1])) {
                return true;
        }

        __parser_ParseTreeClearChildren(parse_tree);

        *tokenizer = at_token;
        if (ParsePointer(tokenizer, &parse_tree->children[0])) {
                return true;
        }

        *tokenizer = at_token;
        if (ParseTypeQualifierList(tokenizer, &parse_tree->children[0])) {
                return true;
        }

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
*/
bool ParseDirectDeclaratorI(Tokenizer *tokenizer, ParseTreeNode *parse_tree) {
        Tokenizer start = *tokenizer;
        Token tokens[2];

        __parser_ParseTreeUpdate(parse_tree, ParseTreeNode_DirectDeclaratorI, 4);

        if (Token_OpenBracket == (tokens[0] = GetToken(tokenizer)).type &&
            ParseConstantExpression(tokenizer, &parse_tree->children[1]) &&
            Token_CloseBracket == (tokens[1] = GetToken(tokenizer)).type &&
            ParseDirectDeclaratorI(tokenizer, &parse_tree->children[3])) {
                ParseTreeSet(&parse_tree->children[0], ParseTreeNode_Symbol, tokens[0]);
                ParseTreeSet(&parse_tree->children[2], ParseTreeNode_Symbol, tokens[1]);
                return true;
        }

        __parser_ParseTreeClearChildren(parse_tree);

        *tokenizer = start;
        if (Token_OpenBracket == (tokens[0] = GetToken(tokenizer)).type &&
            Token_CloseBracket == (tokens[1] = GetToken(tokenizer)).type &&
            ParseDirectDeclaratorI(tokenizer, &parse_tree->children[2])) {
                ParseTreeSet(&parse_tree->children[0], ParseTreeNode_Symbol, tokens[0]);
                ParseTreeSet(&parse_tree->children[1], ParseTreeNode_Symbol, tokens[1]);
                return true;
        }

        *tokenizer = start;
        if (Token_OpenParen == (tokens[0] = GetToken(tokenizer)).type &&
            ParseParameterTypeList(tokenizer, &parse_tree->children[1]) &&
            Token_CloseParen == (tokens[1] = GetToken(tokenizer)).type &&
            ParseDirectDeclaratorI(tokenizer, &parse_tree->children[3])) {
                ParseTreeSet(&parse_tree->children[0], ParseTreeNode_Symbol, tokens[0]);
                ParseTreeSet(&parse_tree->children[2], ParseTreeNode_Symbol, tokens[1]);
                return true;
        }

        *tokenizer = start;
        if (Token_OpenParen == (tokens[0] = GetToken(tokenizer)).type &&
            ParseIdentifierList(tokenizer, &parse_tree->children[1]) &&
            Token_CloseParen == (tokens[1] = GetToken(tokenizer)).type &&
            ParseDirectDeclaratorI(tokenizer, &parse_tree->children[3])) {
                ParseTreeSet(&parse_tree->children[0], ParseTreeNode_Symbol, tokens[0]);
                ParseTreeSet(&parse_tree->children[2], ParseTreeNode_Symbol, tokens[1]);
                return true;
        }

        __parser_ParseTreeClearChildren(parse_tree);

        *tokenizer = start;
        if (Token_OpenParen == (tokens[0] = GetToken(tokenizer)).type &&
            Token_CloseParen == (tokens[1] = GetToken(tokenizer)).type &&
            ParseDirectDeclaratorI(tokenizer, &parse_tree->children[2])) {
                ParseTreeSet(&parse_tree->children[0], ParseTreeNode_Symbol, tokens[0]);
                ParseTreeSet(&parse_tree->children[1], ParseTreeNode_Symbol, tokens[1]);
                return true;
        }

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
*/
bool ParseDirectDeclarator(Tokenizer *tokenizer, ParseTreeNode *parse_tree) {
        Tokenizer start = *tokenizer;
        Token tokens[2];

        __parser_ParseTreeUpdate(parse_tree, ParseTreeNode_DirectDeclarator, 4);

        if (ParseIdentifier(tokenizer, &parse_tree->children[0]) &&
            ParseDirectDeclaratorI(tokenizer, &parse_tree->children[1])) {
                return true;
        }

        *tokenizer = start;
        if (Token_OpenParen == (tokens[0] = GetToken(tokenizer)).type &&
            ParseDeclarator(tokenizer, &parse_tree->children[1]) &&
            Token_CloseParen == (tokens[1] = GetToken(tokenizer)).type &&
            ParseDirectDeclaratorI(tokenizer, &parse_tree->children[3])) {
                ParseTreeSet(&parse_tree->children[0], ParseTreeNode_Symbol, tokens[0]);
                ParseTreeSet(&parse_tree->children[2], ParseTreeNode_Symbol, tokens[1]);
                return true;
        }

        *tokenizer = start;
        return false;
}

/*
  declarator:
  pointer(opt) direct-declarator
*/
bool ParseDeclarator(Tokenizer *tokenizer, ParseTreeNode *parse_tree) {
        Tokenizer start = *tokenizer;

        __parser_ParseTreeUpdate(parse_tree, ParseTreeNode_Declarator, 2);

        if (ParsePointer(tokenizer, &parse_tree->children[0]) &&
            ParseDirectDeclarator(tokenizer, &parse_tree->children[1])) {
                return true;
        }

        __parser_ParseTreeClearChildren(parse_tree);

        *tokenizer = start;
        if (ParseDirectDeclarator(tokenizer, &parse_tree->children[0])) {
                return true;
        }

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

        __parser_ParseTreeUpdate(parse_tree, ParseTreeNode_Enumerator, 3);

        if (ParseIdentifier(tokenizer, &parse_tree->children[0]) &&
            Token_EqualSign == (token = GetToken(tokenizer)).type &&
            ParseConstantExpression(tokenizer, &parse_tree->children[2])) {
                ParseTreeSet(&parse_tree->children[1], ParseTreeNode_Symbol, token);
                return true;
        }

        __parser_ParseTreeClearChildren(parse_tree);

        *tokenizer = start;
        if (ParseIdentifier(tokenizer, &parse_tree->children[0])) {
                return true;
        }

        *tokenizer = start;
        return false;
}

bool ParseEnumeratorListI(Tokenizer *tokenizer, ParseTreeNode *parse_tree) {
        Tokenizer start = *tokenizer;
        Token token;

        __parser_ParseTreeUpdate(parse_tree, ParseTreeNode_EnumeratorListI, 3);

        if (Token_Comma == (token = GetToken(tokenizer)).type &&
            ParseEnumerator(tokenizer, &parse_tree->children[1]) &&
            ParseEnumeratorListI(tokenizer, &parse_tree->children[2])) {
                ParseTreeSet(&parse_tree->children[0], ParseTreeNode_Symbol, token);
                return true;
        }

        *tokenizer = start;
        return true;
}

/*
  enumerator-list:
  enumerator
  enumerator-list , enumerator
*/
bool ParseEnumeratorList(Tokenizer *tokenizer, ParseTreeNode *parse_tree) {
        Tokenizer start = *tokenizer;

        __parser_ParseTreeUpdate(parse_tree, ParseTreeNode_EnumeratorList, 2);

        if (ParseEnumerator(tokenizer, &parse_tree->children[0]) &&
            ParseEnumeratorListI(tokenizer, &parse_tree->children[1])) {
                return true;
        }

        *tokenizer = start;
        return false;
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

        __parser_ParseTreeUpdate(parse_tree, ParseTreeNode_EnumSpecifier, 5);

        if (!(Token_Keyword == token.type &&
              gs_StringIsEqual("enum", token.text, token.text_length))) {
                *tokenizer = start;
                return false;
        }

        ParseTreeSet(&parse_tree->children[0], ParseTreeNode_Keyword, token);

        if (ParseIdentifier(tokenizer, &parse_tree->children[1]) &&
            Token_OpenBrace == (tokens[0] = GetToken(tokenizer)).type &&
            ParseEnumeratorList(tokenizer, &parse_tree->children[3]) &&
            Token_CloseBrace == (tokens[1] = GetToken(tokenizer)).type) {
                ParseTreeSet(&parse_tree->children[2], ParseTreeNode_Symbol, tokens[0]);
                ParseTreeSet(&parse_tree->children[4], ParseTreeNode_Symbol, tokens[1]);
                return true;
        }


        __parser_ParseTreeClearChildren(parse_tree);

        *tokenizer = at_token;
        if (Token_OpenBrace == (tokens[0] = GetToken(tokenizer)).type &&
            ParseEnumeratorList(tokenizer, &parse_tree->children[2]) &&
            Token_CloseBrace == (tokens[1] = GetToken(tokenizer)).type) {
                ParseTreeSet(&parse_tree->children[1], ParseTreeNode_Symbol, tokens[0]);
                ParseTreeSet(&parse_tree->children[3], ParseTreeNode_Symbol, tokens[1]);
                return true;
        }

        __parser_ParseTreeClearChildren(parse_tree);

        *tokenizer = at_token;
        if (ParseIdentifier(tokenizer, &parse_tree->children[0])) {
                return true;
        }

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

        __parser_ParseTreeUpdate(parse_tree, ParseTreeNode_StructDeclarator, 3);

        if (ParseDeclarator(tokenizer, &parse_tree->children[0]) &&
            Token_Colon == (token = GetToken(tokenizer)).type &&
            ParseConstantExpression(tokenizer, &parse_tree->children[2])) {
                ParseTreeSet(&parse_tree->children[1], ParseTreeNode_Symbol, token);
                return true;
        }

        __parser_ParseTreeClearChildren(parse_tree);

        *tokenizer = start;
        if (Token_Colon == (token = GetToken(tokenizer)).type &&
            ParseConstantExpression(tokenizer, &parse_tree->children[1])) {
                ParseTreeSet(&parse_tree->children[0], ParseTreeNode_Symbol, token);
                return true;
        }

        __parser_ParseTreeClearChildren(parse_tree);

        *tokenizer = start;
        if (ParseDeclarator(tokenizer, &parse_tree->children[0])) {
                return true;
        }

        *tokenizer = start;
        return false;
}

bool ParseStructDeclaratorListI(Tokenizer *tokenizer, ParseTreeNode *parse_tree) {
        Tokenizer start = *tokenizer;
        Token token;

        __parser_ParseTreeUpdate(parse_tree, ParseTreeNode_StructDeclaratorListI, 3);

        if (Token_Comma == (token = GetToken(tokenizer)).type &&
            ParseStructDeclarator(tokenizer, &parse_tree->children[1]) &&
            ParseStructDeclaratorListI(tokenizer, &parse_tree->children[2])) {
                ParseTreeSet(&parse_tree->children[0], ParseTreeNode_Symbol, token);
                return true;
        }

        *tokenizer = start;
        return true;
}

/*
  struct-declarator-list:
  struct-declarator
  struct-declarator-list , struct-declarator
*/
bool ParseStructDeclaratorList(Tokenizer *tokenizer, ParseTreeNode *parse_tree) {
        Tokenizer start = *tokenizer;

        __parser_ParseTreeUpdate(parse_tree, ParseTreeNode_StructDeclaratorList, 2);

        if (ParseStructDeclarator(tokenizer, &parse_tree->children[0]) &&
            ParseStructDeclaratorListI(tokenizer, &parse_tree->children[1])) {
                return true;
        }

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

        __parser_ParseTreeUpdate(parse_tree, ParseTreeNode_SpecifierQualifierList, 2);

        if (ParseTypeSpecifier(tokenizer, &parse_tree->children[0]) &&
            ParseSpecifierQualifierList(tokenizer, &parse_tree->children[1])) {
                return true;
        }

        __parser_ParseTreeClearChildren(parse_tree);

        *tokenizer = start;
        if (ParseTypeSpecifier(tokenizer, &parse_tree->children[0])) {
                return true;
        }

        *tokenizer = start;
        if (ParseTypeQualifier(tokenizer, &parse_tree->children[0]) &&
            ParseSpecifierQualifierList(tokenizer, &parse_tree->children[1])) {
                return true;
        }

        __parser_ParseTreeClearChildren(parse_tree);

        *tokenizer = start;
        if (ParseTypeQualifier(tokenizer, &parse_tree->children[0])) {
                return true;
        }

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

        __parser_ParseTreeUpdate(parse_tree, ParseTreeNode_StructDeclaration, 3);

        if (ParseSpecifierQualifierList(tokenizer, &parse_tree->children[0]) &&
            ParseStructDeclaratorList(tokenizer, &parse_tree->children[1]) &&
            Token_SemiColon == (token = GetToken(tokenizer)).type) {
                ParseTreeSet(&parse_tree->children[2], ParseTreeNode_StructDeclaration, token);
                return true;
        }

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

        __parser_ParseTreeUpdate(parse_tree, ParseTreeNode_InitDeclarator, 3);

        if (ParseDeclarator(tokenizer, &parse_tree->children[0]) &&
            Token_EqualSign == (token = GetToken(tokenizer)).type &&
            ParseInitializer(tokenizer, &parse_tree->children[2])) {
                ParseTreeSet(&parse_tree->children[1], ParseTreeNode_Symbol, token);
                return true;
        }

        __parser_ParseTreeClearChildren(parse_tree);

        *tokenizer = start;
        if (ParseDeclarator(tokenizer, &parse_tree->children[0])) {
                return true;
        }

        *tokenizer = start;
        return false;
}

bool ParseInitDeclaratorListI(Tokenizer *tokenizer, ParseTreeNode *parse_tree) {
        Tokenizer start = *tokenizer;
        Token token;

        __parser_ParseTreeUpdate(parse_tree, ParseTreeNode_InitDeclaratorListI, 2);

        if (Token_Comma == (token = GetToken(tokenizer)).type &&
            ParseInitDeclarator(tokenizer, &parse_tree->children[1])) {
                ParseTreeSet(&parse_tree->children[0], ParseTreeNode_Symbol, token);
                return true;
        }

        *tokenizer = start;
        return true;
}

/*
  init-declarator-list:
  init-declarator
  init-declarator-list , init-declarator
*/
bool ParseInitDeclaratorList(Tokenizer *tokenizer, ParseTreeNode *parse_tree) {
        Tokenizer start = *tokenizer;

        __parser_ParseTreeUpdate(parse_tree, ParseTreeNode_InitDeclarationList, 2);

        if (ParseInitDeclarator(tokenizer, &parse_tree->children[0]) &&
            ParseInitDeclaratorListI(tokenizer, &parse_tree->children[1])) {
                return true;
        }

        *tokenizer = start;
        return false;
}

bool ParseStructDeclarationListI(Tokenizer *tokenizer, ParseTreeNode *parse_tree) {
        Tokenizer start = *tokenizer;

        __parser_ParseTreeUpdate(parse_tree, ParseTreeNode_StructDeclarationListI, 2);

        if (ParseStructDeclaration(tokenizer, &parse_tree->children[0]) &&
            ParseStructDeclarationListI(tokenizer, &parse_tree->children[1])) {
                return true;
        }

        *tokenizer = start;
        return true;
}

/*
  struct-declaration-list:
  struct-declaration
  struct-declaration-list struct-declaration
*/
bool ParseStructDeclarationList(Tokenizer *tokenizer, ParseTreeNode *parse_tree) {
        Tokenizer start = *tokenizer;

        __parser_ParseTreeUpdate(parse_tree, ParseTreeNode_StructDeclarationList, 2);

        if (ParseStructDeclaration(tokenizer, &parse_tree->children[0]) &&
            ParseStructDeclarationListI(tokenizer, &parse_tree->children[1])) {
                return true;
        }

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

        __parser_ParseTreeUpdate(parse_tree, ParseTreeNode_StructOrUnionSpecifier, 5);

        Token tokens[2];

        if (ParseStructOrUnion(tokenizer, &parse_tree->children[0]) &&
            ParseIdentifier(tokenizer, &parse_tree->children[1]) &&
            Token_OpenBrace == (tokens[0] = GetToken(tokenizer)).type &&
            ParseStructDeclarationList(tokenizer, &parse_tree->children[3]) &&
            Token_CloseBrace == (tokens[1] = GetToken(tokenizer)).type) {
                ParseTreeSet(&parse_tree->children[2], ParseTreeNode_Symbol, tokens[0]);
                ParseTreeSet(&parse_tree->children[4], ParseTreeNode_Symbol, tokens[1]);
                return true;
        }

        __parser_ParseTreeClearChildren(parse_tree);

        *tokenizer = start;
        if (ParseStructOrUnion(tokenizer, &parse_tree->children[0]) &&
            Token_OpenBrace == (tokens[0] = GetToken(tokenizer)).type &&
            ParseStructDeclarationList(tokenizer, &parse_tree->children[2]) &&
            Token_CloseBrace == (tokens[1] = GetToken(tokenizer)).type) {
                ParseTreeSet(&parse_tree->children[1], ParseTreeNode_Symbol, tokens[0]);
                ParseTreeSet(&parse_tree->children[3], ParseTreeNode_Symbol, tokens[1]);
                return true;
        }

        __parser_ParseTreeClearChildren(parse_tree);

        *tokenizer = start;
        if (ParseStructOrUnion(tokenizer, &parse_tree->children[0]) &&
            ParseIdentifier(tokenizer, &parse_tree->children[1])) {
                return true;
        }

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

        __parser_ParseTreeUpdate(parse_tree, ParseTreeNode_TypeSpecifier, 1);

        *tokenizer = start;
        if (ParseStructOrUnionSpecifier(tokenizer, &parse_tree->children[0])) {
                return true;
        }

        *tokenizer = start;
        if (ParseEnumSpecifier(tokenizer, &parse_tree->children[0])) {
                return true;
        }

        *tokenizer = start;
        if (ParseTypedefName(tokenizer, &parse_tree->children[0])) {
                return true;
        }

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

        __parser_ParseTreeUpdate(parse_tree, ParseTreeNode_DeclarationSpecifiers, 2);

        if (ParseStorageClassSpecifier(tokenizer, &parse_tree->children[0]) &&
            ParseDeclarationSpecifiers(tokenizer, &parse_tree->children[1])) {
                return true;
        }

        *tokenizer = start;
        if (ParseTypeSpecifier(tokenizer, &parse_tree->children[0]) &&
            ParseDeclarationSpecifiers(tokenizer, &parse_tree->children[1])) {
                return true;
        }

        *tokenizer = start;
        if (ParseTypeQualifier(tokenizer, &parse_tree->children[0]) &&
            ParseDeclarationSpecifiers(tokenizer, &parse_tree->children[1])) {
                return true;
        }

        *tokenizer = start;
        if (ParseStorageClassSpecifier(tokenizer, &parse_tree->children[0])) {
                return true;
        }

        __parser_ParseTreeClearChildren(parse_tree);

        *tokenizer = start;
        if (ParseTypeSpecifier(tokenizer, &parse_tree->children[0])) {
                return true;
        }

        *tokenizer = start;
        if (ParseTypeQualifier(tokenizer, &parse_tree->children[0])) {
                return true;
        }

        *tokenizer = start;
        return false;
}

bool ParseDeclarationListI(Tokenizer *tokenizer, ParseTreeNode *parse_tree) {
        Tokenizer start = *tokenizer;

        __parser_ParseTreeUpdate(parse_tree, ParseTreeNode_DeclarationListI, 2);

        if (ParseDeclaration(tokenizer, &parse_tree->children[0]) &&
            ParseDeclarationListI(tokenizer, &parse_tree->children[1])) {
                return true;
        }

        *tokenizer = start;
        return true;
}

/*
  declaration-list:
  declaration
  declaration-list declaration
*/
bool ParseDeclarationList(Tokenizer *tokenizer, ParseTreeNode *parse_tree) {
        Tokenizer start = *tokenizer;

        __parser_ParseTreeUpdate(parse_tree, ParseTreeNode_DeclarationList, 2);

        if (ParseDeclaration(tokenizer, &parse_tree->children[0]) &&
            ParseDeclarationListI(tokenizer, &parse_tree->children[1])) {
                return true;
        }

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

        __parser_ParseTreeUpdate(parse_tree, ParseTreeNode_Declaration, 3);

        if (ParseDeclarationSpecifiers(tokenizer, &parse_tree->children[0]) &&
            ParseInitDeclaratorList(tokenizer, &parse_tree->children[1]) &&
            Token_SemiColon == (token = GetToken(tokenizer)).type) {
                ParseTreeSet(&parse_tree->children[2], ParseTreeNode_Symbol, token);
                return true;
        }

        *tokenizer = start;
        if (ParseDeclarationSpecifiers(tokenizer, &parse_tree->children[0]) &&
            Token_SemiColon == (token = GetToken(tokenizer)).type) {
                ParseTreeSet(&parse_tree->children[1], ParseTreeNode_Symbol, token);
                return true;
        }

        *tokenizer = start;
        return false;
}

/*
  function-definition:
  declaration-specifiers(opt) declarator declaration-list(opt) compound-statement
*/
bool ParseFunctionDefinition(Tokenizer *tokenizer, ParseTreeNode *parse_tree) {
        Tokenizer start = *tokenizer;

        __parser_ParseTreeUpdate(parse_tree, ParseTreeNode_FunctionDefinition, 4);

        if (ParseDeclarationSpecifiers(tokenizer, &parse_tree->children[0]) &&
            ParseDeclarator(tokenizer, &parse_tree->children[1]) &&
            ParseDeclarationList(tokenizer, &parse_tree->children[2]) &&
            ParseCompoundStatement(tokenizer, &parse_tree->children[3])) {
                return true;
        }

        __parser_ParseTreeClearChildren(parse_tree);

        *tokenizer = start;
        if (ParseDeclarationSpecifiers(tokenizer, &parse_tree->children[0]) &&
            ParseDeclarator(tokenizer, &parse_tree->children[1]) &&
            ParseCompoundStatement(tokenizer, &parse_tree->children[2])) {
                return true;
        }

        *tokenizer = start;
        if (ParseDeclarator(tokenizer, &parse_tree->children[0]) &&
            ParseDeclarationList(tokenizer, &parse_tree->children[1]) &&
            ParseCompoundStatement(tokenizer, &parse_tree->children[2])) {
                return true;
        }

        __parser_ParseTreeClearChildren(parse_tree);

        *tokenizer = start;
        if (ParseDeclarator(tokenizer, &parse_tree->children[0]) &&
            ParseCompoundStatement(tokenizer, &parse_tree->children[1])) {
                return true;
        }

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

        __parser_ParseTreeUpdate(parse_tree, ParseTreeNode_ExternalDeclaration, 1);

        if (ParseFunctionDefinition(tokenizer, &parse_tree->children[0])) return true;

        *tokenizer = start;
        if (ParseDeclaration(tokenizer, &parse_tree->children[0])) return true;

        *tokenizer = start;
        return false;
}

bool ParseTranslationUnitI(Tokenizer *tokenizer, ParseTreeNode *parse_tree) {
        Tokenizer start = *tokenizer;

        __parser_ParseTreeUpdate(parse_tree, ParseTreeNode_TranslationUnitI, 2);

        if (ParseExternalDeclaration(tokenizer, &parse_tree->children[0]) &&
            ParseTranslationUnitI(tokenizer, &parse_tree->children[1])) {
                return true;
        }

        *tokenizer = start;
        return true;
}

/*
  translation-unit:
  external-declaration
  translation-unit external-declaration
*/
bool ParseTranslationUnit(Tokenizer *tokenizer, ParseTreeNode *parse_tree) {
        Tokenizer start = *tokenizer;

        __parser_ParseTreeUpdate(parse_tree, ParseTreeNode_TranslationUnit, 2);

        if (ParseExternalDeclaration(tokenizer, &parse_tree->children[0]) &&
            ParseTranslationUnitI(tokenizer, &parse_tree->children[1])) {
                return true;
        }

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
