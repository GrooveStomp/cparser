/******************************************************************************
 * File: parse_tree.c
 * Created: 2017-01-12
 * Updated: 2020-11-16
 * Package: C-Parser
 * Creator: Aaron Oman (GrooveStomp)
 * Copyright 2017 - 2020, Aaron Oman and the C-Parser contributors
 * SPDX-License-Identifier: LGPL-3.0-only
 ******************************************************************************/
#ifndef PARSE_TREE
#define PARSE_TREE

#include "gs.h"
#include "lexer.c"

#define DEFAULT_ALLOC_COUNT 2

typedef enum ParseTreeNodeType {
        ParseTreeNode_AbstractDeclarator,
        ParseTreeNode_AdditiveExpression,
        ParseTreeNode_AndExpression,
        ParseTreeNode_ArgumentExpressionList,
        ParseTreeNode_AssignmentExpression,
        ParseTreeNode_AssignmentOperator,
        ParseTreeNode_CastExpression,
        ParseTreeNode_CompoundStatement,
        ParseTreeNode_ConditionalExpression,
        ParseTreeNode_Constant,
        ParseTreeNode_ConstantExpression,
        ParseTreeNode_Declaration,
        ParseTreeNode_DeclarationList,
        ParseTreeNode_DeclarationSpecifiers,
        ParseTreeNode_Declarator,
        ParseTreeNode_DirectAbstractDeclarator,
        ParseTreeNode_DirectDeclarator,
        ParseTreeNode_EnumSpecifier,
        ParseTreeNode_Enumerator,
        ParseTreeNode_EnumeratorList,
        ParseTreeNode_EqualityExpression,
        ParseTreeNode_ExclusiveOrExpression,
        ParseTreeNode_Expression,
        ParseTreeNode_ExpressionStatement,
        ParseTreeNode_ExternalDeclaration,
        ParseTreeNode_FunctionDefinition,
        ParseTreeNode_Identifier,
        ParseTreeNode_IdentifierList,
        ParseTreeNode_InclusiveOrExpression,
        ParseTreeNode_InitDeclarationList,
        ParseTreeNode_InitDeclarator,
        ParseTreeNode_InitDeclaratorList,
        ParseTreeNode_Initializer,
        ParseTreeNode_InitializerList,
        ParseTreeNode_IterationStatement,
        ParseTreeNode_JumpStatement,
        ParseTreeNode_Keyword,
        ParseTreeNode_LabeledStatement,
        ParseTreeNode_LogicalAndExpression,
        ParseTreeNode_LogicalOrExpression,
        ParseTreeNode_MultiplicativeExpression,
        ParseTreeNode_ParameterDeclaration,
        ParseTreeNode_ParameterList,
        ParseTreeNode_ParameterTypeList,
        ParseTreeNode_Pointer,
        ParseTreeNode_PostfixExpression,
        ParseTreeNode_PrimaryExpression,
        ParseTreeNode_RelationalExpression,
        ParseTreeNode_SelectionStatement,
        ParseTreeNode_ShiftExpression,
        ParseTreeNode_SpecifierQualifierList,
        ParseTreeNode_Statement,
        ParseTreeNode_StatementList,
        ParseTreeNode_StorageClassSpecifier,
        ParseTreeNode_String,
        ParseTreeNode_StructDeclaration,
        ParseTreeNode_StructDeclarationList,
        ParseTreeNode_StructDeclarator,
        ParseTreeNode_StructDeclaratorList,
        ParseTreeNode_StructOrUnion,
        ParseTreeNode_StructOrUnionSpecifier,
        ParseTreeNode_Symbol,
        ParseTreeNode_TranslationUnit,
        ParseTreeNode_TypeName,
        ParseTreeNode_TypeQualifier,
        ParseTreeNode_TypeQualifierList,
        ParseTreeNode_TypeSpecifier,
        ParseTreeNode_TypedefName,
        ParseTreeNode_UnaryExpression,
        ParseTreeNode_UnaryOperator,

        ParseTreeNode_AdditiveExpressionI,
        ParseTreeNode_AndExpressionI,
        ParseTreeNode_ArgumentExpressionListI,
        ParseTreeNode_DeclarationListI,
        ParseTreeNode_DirectAbstractDeclaratorI,
        ParseTreeNode_DirectDeclaratorI,
        ParseTreeNode_EnumeratorListI,
        ParseTreeNode_EqualityExpressionI,
        ParseTreeNode_ExclusiveOrExpressionI,
        ParseTreeNode_ExpressionI,
        ParseTreeNode_IdentifierListI,
        ParseTreeNode_InclusiveOrExpressionI,
        ParseTreeNode_InitDeclaratorListI,
        ParseTreeNode_InitializerListI,
        ParseTreeNode_LogicalAndExpressionI,
        ParseTreeNode_LogicalOrExpressionI,
        ParseTreeNode_ParameterListI,
        ParseTreeNode_PostfixExpressionI,
        ParseTreeNode_RelationalExpressionI,
        ParseTreeNode_ShiftExpressionI,
        ParseTreeNode_StatementListI,
        ParseTreeNode_StructDeclarationListI,
        ParseTreeNode_StructDeclaratorListI,
        ParseTreeNode_TranslationUnitI,
        ParseTreeNode_TypeQualifierListI,

        ParseTreeNode_Unknown,
} ParseTreeNodeType;

char *ParseTreeNodeName(ParseTreeNodeType type) {
        switch (type) {
                case ParseTreeNode_AbstractDeclarator: { return "AbstractDeclarator"; } break;
                case ParseTreeNode_AdditiveExpression: { return "AdditiveExpression"; } break;
                case ParseTreeNode_AndExpression: { return "AndExpression"; } break;
                case ParseTreeNode_ArgumentExpressionList: { return "ArgumentExpressionList"; } break;
                case ParseTreeNode_AssignmentExpression: { return "AssignmentExpression"; } break;
                case ParseTreeNode_AssignmentOperator: { return "AssignmentOperator"; } break;
                case ParseTreeNode_CastExpression: { return "CastExpression"; } break;
                case ParseTreeNode_CompoundStatement: { return "CompoundStatement"; } break;
                case ParseTreeNode_ConditionalExpression: { return "ConditionalExpression"; } break;
                case ParseTreeNode_Constant: { return "Constant"; } break;
                case ParseTreeNode_ConstantExpression: { return "ConstantExpression"; } break;
                case ParseTreeNode_Declaration: { return "Declaration"; } break;
                case ParseTreeNode_DeclarationList: { return "DeclarationList"; } break;
                case ParseTreeNode_DeclarationSpecifiers: { return "DeclarationSpecifiers"; } break;
                case ParseTreeNode_Declarator: { return "Declarator"; } break;
                case ParseTreeNode_DirectAbstractDeclarator: { return "DirectAbstractDeclarator"; } break;
                case ParseTreeNode_DirectDeclarator: { return "DirectDeclarator"; } break;
                case ParseTreeNode_EnumSpecifier: { return "EnumSpecifier"; } break;
                case ParseTreeNode_Enumerator: { return "Enumerator"; } break;
                case ParseTreeNode_EnumeratorList: { return "EnumeratorList"; } break;
                case ParseTreeNode_EqualityExpression: { return "EqualityExpression"; } break;
                case ParseTreeNode_ExclusiveOrExpression: { return "ExclusiveOrExpression"; } break;
                case ParseTreeNode_Expression: { return "Expression"; } break;
                case ParseTreeNode_ExpressionStatement: { return "ExpressionStatement"; } break;
                case ParseTreeNode_ExternalDeclaration: { return "ExternalDeclaration"; } break;
                case ParseTreeNode_FunctionDefinition: { return "FunctionDefinition"; } break;
                case ParseTreeNode_Identifier: { return "Identifier"; } break;
                case ParseTreeNode_IdentifierList: { return "IdentifierList"; } break;
                case ParseTreeNode_InclusiveOrExpression: { return "InclusiveOrExpression"; } break;
                case ParseTreeNode_InitDeclarationList: { return "InitDeclarationList"; } break;
                case ParseTreeNode_InitDeclarator: { return "InitDeclarator"; } break;
                case ParseTreeNode_InitDeclaratorList: { return "InitDeclaratorList"; } break;
                case ParseTreeNode_Initializer: { return "Initializer"; } break;
                case ParseTreeNode_InitializerList: { return "InitializerList"; } break;
                case ParseTreeNode_IterationStatement: { return "IterationStatement"; } break;
                case ParseTreeNode_JumpStatement: { return "JumpStatement"; } break;
                case ParseTreeNode_Keyword: { return "Keyword"; } break;
                case ParseTreeNode_LabeledStatement: { return "LabeledStatement"; } break;
                case ParseTreeNode_LogicalAndExpression: { return "LogicalAndExpression"; } break;
                case ParseTreeNode_LogicalOrExpression: { return "LogicalOrExpression"; } break;
                case ParseTreeNode_MultiplicativeExpression: { return "MultiplicativeExpression"; } break;
                case ParseTreeNode_ParameterDeclaration: { return "ParameterDeclaration"; } break;
                case ParseTreeNode_ParameterList: { return "ParameterList"; } break;
                case ParseTreeNode_ParameterTypeList: { return "ParameterTypeList"; } break;
                case ParseTreeNode_Pointer: { return "Pointer"; } break;
                case ParseTreeNode_PostfixExpression: { return "PostfixExpression"; } break;
                case ParseTreeNode_PrimaryExpression: { return "PrimaryExpression"; } break;
                case ParseTreeNode_RelationalExpression: { return "RelationalExpression"; } break;
                case ParseTreeNode_SelectionStatement: { return "SelectionStatement"; } break;
                case ParseTreeNode_ShiftExpression: { return "ShiftExpression"; } break;
                case ParseTreeNode_SpecifierQualifierList: { return "SpecifierQualifierList"; } break;
                case ParseTreeNode_Statement: { return "Statement"; } break;
                case ParseTreeNode_StatementList: { return "StatementList"; } break;
                case ParseTreeNode_StorageClassSpecifier: { return "StorageClassSpecifier"; } break;
                case ParseTreeNode_String: { return "String"; } break;
                case ParseTreeNode_StructDeclaration: { return "StructDeclaration"; } break;
                case ParseTreeNode_StructDeclarationList: { return "StructDeclarationList"; } break;
                case ParseTreeNode_StructDeclarator: { return "StructDeclarator"; } break;
                case ParseTreeNode_StructDeclaratorList: { return "StructDeclaratorList"; } break;
                case ParseTreeNode_StructOrUnion: { return "StructOrUnion"; } break;
                case ParseTreeNode_StructOrUnionSpecifier: { return "StructOrUnionSpecifier"; } break;
                case ParseTreeNode_Symbol: { return "Symbol"; } break;
                case ParseTreeNode_TranslationUnit: { return "TranslationUnit"; } break;
                case ParseTreeNode_TypeName: { return "TypeName"; } break;
                case ParseTreeNode_TypeQualifier: { return "TypeQualifier"; } break;
                case ParseTreeNode_TypeQualifierList: { return "TypeQualifierList"; } break;
                case ParseTreeNode_TypeSpecifier: { return "TypeSpecifier"; } break;
                case ParseTreeNode_TypedefName: { return "TypedefName"; } break;
                case ParseTreeNode_UnaryExpression: { return "UnaryExpression"; } break;
                case ParseTreeNode_UnaryOperator: { return "UnaryOperator"; } break;

                case ParseTreeNode_AdditiveExpressionI: { return "AdditiveExpression'"; } break;
                case ParseTreeNode_AndExpressionI: { return "AndExpression'"; } break;
                case ParseTreeNode_ArgumentExpressionListI: { return "ArgumentExpressionList'"; } break;
                case ParseTreeNode_DeclarationListI: { return "DeclarationList'"; } break;
                case ParseTreeNode_DirectAbstractDeclaratorI: { return "DirectAbstractDeclarator'"; } break;
                case ParseTreeNode_DirectDeclaratorI: { return "DirectDeclarator'"; } break;
                case ParseTreeNode_EnumeratorListI: { return "EnumeratorList'"; } break;
                case ParseTreeNode_EqualityExpressionI: { return "EqualityExpression'"; } break;
                case ParseTreeNode_ExclusiveOrExpressionI: { return "ExclusiveOrExpression'"; } break;
                case ParseTreeNode_ExpressionI: { return "Expression'"; } break;
                case ParseTreeNode_IdentifierListI: { return "IdentifierList'"; } break;
                case ParseTreeNode_InclusiveOrExpressionI: { return "InclusiveOrExpression'"; } break;
                case ParseTreeNode_InitDeclaratorListI: { return "InitDeclaratorList'"; } break;
                case ParseTreeNode_InitializerListI: { return "InitializerList'"; } break;
                case ParseTreeNode_LogicalAndExpressionI: { return "LogicalAndExpression'"; } break;
                case ParseTreeNode_LogicalOrExpressionI: { return "LogicalOrExpression'"; } break;
                case ParseTreeNode_ParameterListI: { return "ParameterList'"; } break;
                case ParseTreeNode_PostfixExpressionI: { return "PostfixExpression'"; } break;
                case ParseTreeNode_RelationalExpressionI: { return "RelationalExpression'"; } break;
                case ParseTreeNode_ShiftExpressionI: { return "ShiftExpression'"; } break;
                case ParseTreeNode_StatementListI: { return "StatementList'"; } break;
                case ParseTreeNode_StructDeclarationListI: { return "StructDeclarationList'"; } break;
                case ParseTreeNode_StructDeclaratorListI: { return "StructDeclaratorList'"; } break;
                case ParseTreeNode_TranslationUnitI: { return "TranslationUnit'"; } break;
                case ParseTreeNode_TypeQualifierListI: { return "TypeQualifierList'"; } break;

                default: { return "Unknown"; } break;
        }
}

typedef struct ParseTreeNode {
        gs_Allocator allocator;
        ParseTreeNodeType type;
        Token token;
        u32 capacity;
        u32 num_children;
        struct ParseTreeNode *children;
} ParseTreeNode;

typedef enum ParseTreeErrorEnum {
        ParseTreeErrorChildAlloc,
        ParseTreeErrorNone,
} ParseTreeErrorEnum;

const char *__parse_tree_error_strings[] = {
        "Couldn't allocate memory for new child node",
        "No error",
};

ParseTreeErrorEnum __parse_tree_last_error = ParseTreeErrorNone;

const char *ParseTreeErrorString() {
        const char *result = __parse_tree_error_strings[__parse_tree_last_error];
        __parse_tree_last_error = ParseTreeErrorNone;

        return result;
}

ParseTreeNode *__parse_tree_Alloc(gs_Allocator allocator) {
        ParseTreeNode *node = (ParseTreeNode *)allocator.malloc(sizeof(*node));
        return node;
}

void __parse_tree_Init(ParseTreeNode *node, gs_Allocator allocator) {
        node->allocator = allocator;

        node->token.text = NULL;
        node->token.text_length = 0;
        node->token.type = Token_Unknown;
        node->token.line = 0;
        node->token.column = 0;

        node->type = ParseTreeNode_Unknown;
        node->children = GS_NULL_PTR;
        node->num_children = 0;
        node->capacity = 0;

        return;
}

ParseTreeNode *ParseTreeInit(gs_Allocator allocator) {
        ParseTreeNode *node = __parse_tree_Alloc(allocator);
        __parse_tree_Init(node, allocator);
        return node;
}

void ParseTreeSetToken(ParseTreeNode *node, Token token) {
        Token *this = &(node->token);
        this->text = token.text;
        this->text_length = token.text_length;
        this->type = token.type;
        this->line = token.line;
        this->column = token.column;
}

void ParseTreeSet(ParseTreeNode *self, ParseTreeNodeType type, Token token) {
        self->type = type;
        ParseTreeSetToken(self, token);
}

bool __parse_tree_AddChild(ParseTreeNode *self, ParseTreeNodeType type) {
        u32 alloc_count = DEFAULT_ALLOC_COUNT;

        if (self->children == NULL) {
                self->children = (ParseTreeNode *)self->allocator.malloc(sizeof(ParseTreeNode) * alloc_count);
                if (self->children == NULL) {
                        __parse_tree_last_error = ParseTreeErrorChildAlloc;
                        return false;
                }
                self->capacity = alloc_count;
                for (int i=0; i<alloc_count; i++) {
                        __parse_tree_Init(&self->children[i], self->allocator);
                }
        } else if (self->capacity <= self->num_children) {
                if (self->capacity > 0) {
                        alloc_count = self->capacity * 2;
                        self->children = (ParseTreeNode *)self->allocator.realloc(self->children, sizeof(ParseTreeNode) * alloc_count);
                        if (self->children == NULL) {
                                __parse_tree_last_error = ParseTreeErrorChildAlloc;
                                return false;
                        }
                        self->capacity = alloc_count;
                        for (int i=self->num_children; i<alloc_count; i++) {
                                __parse_tree_Init(&self->children[i], self->allocator);
                        }
                }
        }

        self->children[self->num_children].type = type;
        self->num_children++;

        return true;
}

void ParseTreeNewChildren(ParseTreeNode *self, u32 count) {
        for (int i = 0; i < count; i++) {
                __parse_tree_AddChild(self, ParseTreeNode_Unknown);
        }
}

void ParseTreeDeinit(ParseTreeNode *self) {
        if (self == NULL) {
                return;
        }

        if (self->children != NULL) {
                self->allocator.free(self->children);
                self->children = NULL;
                self->num_children = 0;
                self->capacity = 0;
        }

        self->allocator.free(self);
}

void ParseTreePrint(ParseTreeNode *self, u32 indent_level, u32 indent_increment, int (*print_func)(const char *format, ...)) {
        if (self->type == ParseTreeNode_Unknown) return;

        if (self->token.type != Token_Unknown) {
                print_func("[%4d,%3d] ", self->token.line, self->token.column);
        } else {
                print_func("           ");
        }

        if (indent_level > 0) print_func("%*c", indent_level * indent_increment, ' ');

        print_func("%s", ParseTreeNodeName(self->type));

        if (self->token.type != Token_Unknown) print_func("( %.*s )", (u32)(self->token.text_length), self->token.text);

        print_func("\n");

        for (int i = 0; i < self->num_children; i++) {
                ParseTreePrint(&self->children[i], indent_level + 1, indent_increment, print_func);
        }
}

#endif // PARSE_TREE
