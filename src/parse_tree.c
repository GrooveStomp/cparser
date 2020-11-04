/******************************************************************************
 * File: parse_tree.c
 * Created:
 * Updated: 2016-11-04
 * Package: C-Parser
 * Creator: Aaron Oman (GrooveStomp)
 * Copyright - 2020, Aaron Oman and the C-Parser contributors
 * SPDX-License-Identifier: LGPL-3.0-only
 ******************************************************************************/
#ifndef PARSE_TREE
#define PARSE_TREE

#include "gs.h"
#include "lexer.c"

#define DEFAULT_ALLOC_COUNT 2

typedef struct ParseTreeNode {
        gs_Allocator allocator;
        u32 name_length;
        u32 capacity;
        u32 num_children;
        Token token;
        char *name;
        struct ParseTreeNode *children;
} ParseTreeNode;

typedef enum ParseTreeErrorEnum {
        ParseTreeErrorChildAlloc,
        ParseTreeErrorNone,
} ParseTreeErrorEnum;

const char *__parse_tree_ErrorStrings[] = {
        "Couldn't allocate memory for new child node",
        "No Error"
};

ParseTreeErrorEnum __parse_tree_LastError = ParseTreeErrorNone;

const char *ParseTreeErrorString() {
        const char *result = __parse_tree_ErrorStrings[__parse_tree_LastError];
        __parse_tree_LastError = ParseTreeErrorNone;

        return result;
}

ParseTreeNode *ParseTreeInit(gs_Allocator allocator);
void ParseTreeDeinit(ParseTreeNode *node);

void ParseTreeSetName(ParseTreeNode *node, char *name);
void ParseTreeSetToken(ParseTreeNode *node, Token token);
void ParseTreeSet(ParseTreeNode *self, char *name, Token token);
void ParseTreeNewChildren(ParseTreeNode *self, u32 Count);
void ParseTreePrint(ParseTreeNode *self, u32 IndentLevel, u32 IndentIncrement);

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

        node->name = GS_NULL_PTR;
        node->name_length = 0;
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

/* name must be a NULL-terminated string! */
void ParseTreeSetName(ParseTreeNode *node, char *name) {
        u32 name_length = gs_StringLength(name);
        if (node->name != NULL) {
                node->allocator.free(node->name);
        }

        node->name = (char *)node->allocator.malloc(name_length + 1);
        gs_StringCopy(name, node->name, name_length);
        node->name_length = name_length;
}

void ParseTreeSetToken(ParseTreeNode *node, Token token) {
        Token *this = &(node->token);
        this->text = token.text;
        this->text_length = token.text_length;
        this->type = token.type;
        this->line = token.line;
        this->column = token.column;
}

void ParseTreeSet(ParseTreeNode *self, char *name, Token token) {
        ParseTreeSetName(self, name);
        ParseTreeSetToken(self, token);
}

bool __parse_tree_AddChild(ParseTreeNode *self, char *name) {
        u32 name_length = gs_StringLength(name);
        u32 alloc_count = DEFAULT_ALLOC_COUNT;

        if (self->children == NULL) {
                self->children = (ParseTreeNode *)self->allocator.malloc(sizeof(ParseTreeNode) * alloc_count);
                if (self->children == NULL) {
                        __parse_tree_LastError = ParseTreeErrorChildAlloc;
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
                                __parse_tree_LastError = ParseTreeErrorChildAlloc;
                                return false;
                        }
                        self->capacity = alloc_count;
                        for (int i=self->num_children; i<alloc_count; i++) {
                                __parse_tree_Init(&self->children[i], self->allocator);
                        }
                }
        }

        ParseTreeSetName(&self->children[self->num_children], name);
        self->num_children++;

        return true;
}

void ParseTreeNewChildren(ParseTreeNode *self, u32 count) {
        for (int i = 0; i < count; i++) {
                __parse_tree_AddChild(self, "Empty");
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

        if (self->name != NULL) {
                self->allocator.free(self->name);
                self->name = NULL;
                self->name_length = 0;
        }

        self->allocator.free(self);
}

void ParseTreePrint(ParseTreeNode *self, u32 IndentLevel, u32 IndentIncrement) {
        int min_compare_length = gs_Min(self->name_length, 5);
        if (gs_StringIsEqual("Empty", self->name, min_compare_length)) return;

        if (self->token.type != Token_Unknown) {
                printf("[%4d,%3d] ", self->token.line, self->token.column);
        } else {
                printf("           ");
        }

        if (IndentLevel > 0) printf("%*c", IndentLevel * IndentIncrement, ' ');

        if (self->name_length > 0) {
                printf("%s", self->name);
        } else {
                printf("Unknown name");
        }

        if (self->token.type != Token_Unknown) printf("( %.*s )", (u32)(self->token.text_length), self->token.text);

        printf("\n");

        for (int i=0; i<self->num_children; i++) {
                ParseTreePrint(&self->children[i], IndentLevel + 1, IndentIncrement);
        }
}

#endif // PARSE_TREE
