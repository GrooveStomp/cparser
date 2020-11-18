/******************************************************************************
 * File: parse_tree.c
 * Created: 2017-01-12
 * Updated: 2020-11-17
 * Package: C-Parser
 * Creator: Aaron Oman (GrooveStomp)
 * Homepage: https://git.sr.ht/~groovestomp/c-parser
 * Copyright 2017 - 2020, Aaron Oman and the C-Parser contributors
 * SPDX-License-Identifier: LGPL-3.0-only
 ******************************************************************************/
#ifndef PARSE_TREE
#define PARSE_TREE

#include "gs.h"
#include "lexer.c"

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
        ParseTreeNode_MultiplicativeExpressionI,
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

char *__parse_tree_node_type_names[] = {
        "AbstractDeclarator",
        "AdditiveExpression",
        "AndExpression",
        "ArgumentExpressionList",
        "AssignmentExpression",
        "AssignmentOperator",
        "CastExpression",
        "CompoundStatement",
        "ConditionalExpression",
        "Constant",
        "ConstantExpression",
        "Declaration",
        "DeclarationList",
        "DeclarationSpecifiers",
        "Declarator",
        "DirectAbstractDeclarator",
        "DirectDeclarator",
        "EnumSpecifier",
        "Enumerator",
        "EnumeratorList",
        "EqualityExpression",
        "ExclusiveOrExpression",
        "Expression",
        "ExpressionStatement",
        "ExternalDeclaration",
        "FunctionDefinition",
        "Identifier",
        "IdentifierList",
        "InclusiveOrExpression",
        "InitDeclarationList",
        "InitDeclarator",
        "InitDeclaratorList",
        "Initializer",
        "InitializerList",
        "IterationStatement",
        "JumpStatement",
        "Keyword",
        "LabeledStatement",
        "LogicalAndExpression",
        "LogicalOrExpression",
        "MultiplicativeExpression",
        "ParameterDeclaration",
        "ParameterList",
        "ParameterTypeList",
        "Pointer",
        "PostfixExpression",
        "PrimaryExpression",
        "RelationalExpression",
        "SelectionStatement",
        "ShiftExpression",
        "SpecifierQualifierList",
        "Statement",
        "StatementList",
        "StorageClassSpecifier",
        "String",
        "StructDeclaration",
        "StructDeclarationList",
        "StructDeclarator",
        "StructDeclaratorList",
        "StructOrUnion",
        "StructOrUnionSpecifier",
        "Symbol",
        "TranslationUnit",
        "TypeName",
        "TypeQualifier",
        "TypeQualifierList",
        "TypeSpecifier",
        "TypedefName",
        "UnaryExpression",
        "UnaryOperator",

        "AdditiveExpression'",
        "AndExpression'",
        "ArgumentExpressionList'",
        "DeclarationList'",
        "DirectAbstractDeclarator'",
        "DirectDeclarator'",
        "EnumeratorList'",
        "EqualityExpression'",
        "ExclusiveOrExpression'",
        "Expression'",
        "IdentifierList'",
        "InclusiveOrExpression'",
        "InitDeclaratorList'",
        "InitializerList'",
        "LogicalAndExpression'",
        "LogicalOrExpression'",
        "MultiplicativeExpression'",
        "ParameterList'",
        "PostfixExpression'",
        "RelationalExpression'",
        "ShiftExpression'",
        "StatementList'",
        "StructDeclarationList'",
        "StructDeclaratorList'",
        "TranslationUnit'",
        "TypeQualifierList'",

        "Unknown",
};

char *ParseTreeNodeName(ParseTreeNodeType type) {
        if (type > ParseTreeNode_Unknown) {
                return __parse_tree_node_type_names[ParseTreeNode_Unknown];
        }

        return __parse_tree_node_type_names[type];
}

static gs_Allocator __parse_tree_allocator;

typedef struct ParseTreeNode {
        ParseTreeNodeType type;
        Token token;
        gs_TreeNode tree;
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

void __ParseTreeInit(ParseTreeNode *node) {
        node->token.text = NULL;
        node->token.text_length = 0;
        node->token.type = Token_Unknown;
        node->token.line = 0;
        node->token.column = 0;

        node->type = ParseTreeNode_Unknown;
        gs_TreeInit(&(node->tree), __parse_tree_allocator);

        return;
}

ParseTreeNode *ParseTreeInit(gs_Allocator allocator) {
        __parse_tree_allocator = allocator;
        ParseTreeNode *node = (ParseTreeNode *)allocator.malloc(sizeof(*node));
        __ParseTreeInit(node);
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

ParseTreeNode *ParseTreeAddChild(ParseTreeNode *self) {
        gs_TreeNode *child_tree = gs_TreeAddChild(&self->tree, ParseTreeNode, tree, __parse_tree_allocator);
        if (child_tree == GS_NULL_PTR) {
                // TODO: Error handling
                return GS_NULL_PTR;
        }
        ParseTreeNode *child = gs_TreeContainer(child_tree, ParseTreeNode, tree);
        __ParseTreeInit(child);

        return child;
}

// TODO: Move to gs.h
bool __ParseTreeRecursiveDestroy(ParseTreeNode *parse_node) {
        gs_TreeNode *tree_node = &parse_node->tree;

        gs_TreeNode *child = tree_node->child;
        if (child != GS_NULL_PTR) {
                __ParseTreeRecursiveDestroy(gs_TreeContainer(child, ParseTreeNode, tree));
        }

        gs_TreeNode *sibling = tree_node->sibling;
        if (sibling != GS_NULL_PTR) {
                __ParseTreeRecursiveDestroy(gs_TreeContainer(sibling, ParseTreeNode, tree));
        }

        __parse_tree_allocator.free(parse_node);
}

void ParseTreeRemoveAllChildren(ParseTreeNode *node) {
        gs_TreeNode *child = node->tree.child;
        if (child == GS_NULL_PTR) {
                return;
        }

        __ParseTreeRecursiveDestroy(gs_TreeContainer(child, ParseTreeNode, tree));

        node->tree.child = GS_NULL_PTR;
}

// TODO: Move to gs.h
bool ParseTreeRemoveChild(ParseTreeNode *node, ParseTreeNode *child) {
        gs_TreeNode *current = node->tree.child;
        if (current == GS_NULL_PTR) {
                return false;
        }

        gs_TreeNode *last = GS_NULL_PTR;
        while (current->sibling != GS_NULL_PTR) {
                ParseTreeNode *container = gs_TreeContainer(current, ParseTreeNode, tree);
                if (container == child) {
                        if (last == GS_NULL_PTR) {
                                node->tree.child = GS_NULL_PTR;
                        } else {
                                last->sibling = current->sibling;
                        }
                        __ParseTreeRecursiveDestroy(child);

                        return true;
                }
                last = current;
                current = current->sibling;
        }

        return false;
}

void __ParseTreeDeinit(void *ptr) {
        ParseTreeNode *self = (ParseTreeNode *)ptr;
        __parse_tree_allocator.free(self);
}

void ParseTreeDeinit(ParseTreeNode *self) {
        if (self == NULL) {
                return;
        }

        gs_TreeDeinit(&self->tree, ParseTreeNode, tree, __ParseTreeDeinit);
}

void ParseTreePrint(ParseTreeNode *self, u32 indent_level, u32 indent_increment, int (*print_func)(const char *format, ...)) {
        if (self->type != ParseTreeNode_Unknown) {
                if (self->token.type != Token_Unknown) {
                        print_func("[%4d,%3d] ", self->token.line, self->token.column);
                } else {
                        print_func("           ");
                }

                if (indent_level > 0) print_func("%*c", indent_level * indent_increment, ' ');

                print_func("%s", ParseTreeNodeName(self->type));

                if (self->token.type != Token_Unknown) {
                        print_func("( %.*s )", (u32)(self->token.text_length), self->token.text);
                }

                print_func("\n");
        }

        if (self->tree.child != GS_NULL_PTR) {
                ParseTreeNode *child = gs_TreeContainer(self->tree.child, ParseTreeNode, tree);
                ParseTreePrint(child, indent_level + 1, indent_increment, print_func);
        }

        if (self->tree.sibling != GS_NULL_PTR) {
                ParseTreeNode *sibling = gs_TreeContainer(self->tree.sibling, ParseTreeNode, tree);
                ParseTreePrint(sibling, indent_level, indent_increment, print_func);
        }
}

#endif // PARSE_TREE
