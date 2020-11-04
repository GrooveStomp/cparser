/******************************************************************************
 * File: parse_tree.c
 * Created:
 * Updated: 2016-11-03
 * Package: C-Parser
 * Creator: Aaron Oman (GrooveStomp)
 * Copyright - 2020, Aaron Oman and the C-Parser contributors
 * SPDX-License-Identifier: LGPL-3.0-only
 ******************************************************************************/
#ifndef PARSE_TREE
#define PARSE_TREE

#include "gs.h"

#define DEFAULT_ALLOC_COUNT 2

typedef struct parse_tree_node {
        gs_allocator Allocator;
        u32 NameLength;
        u32 Capacity;
        u32 NumChildren;
        token Token;
        char *Name;
        struct parse_tree_node *Children;
} parse_tree_node;

typedef enum ParseTreeErrorEnum {
        ParseTreeErrorChildAlloc,
        ParseTreeErrorNone,
} ParseTreeErrorEnum;

const char *__ParseTree_ErrorStrings[] = {
        "Couldn't allocate memory for new child node",
        "No Error"
};

ParseTreeErrorEnum __ParseTree_LastError = ParseTreeErrorNone;

const char *ParseTreeErrorString() {
        const char *Result = __ParseTree_ErrorStrings[__ParseTree_LastError];
        __ParseTree_LastError = ParseTreeErrorNone;

        return Result;
}

parse_tree_node *ParseTreeInit(gs_allocator Allocator);
void ParseTreeDeinit(parse_tree_node *Node);

void ParseTreeSetName(parse_tree_node *Node, char *Name);
void ParseTreeSetToken(parse_tree_node *Node, token Token);
void ParseTreeSet(parse_tree_node *Self, char *Name, token Token);
void ParseTreeNewChildren(parse_tree_node *Self, u32 Count);
void ParseTreePrint(parse_tree_node *Self, u32 IndentLevel, u32 IndentIncrement);

parse_tree_node *__ParseTree_Alloc(gs_allocator Allocator) {
        parse_tree_node *Node = (parse_tree_node *)Allocator.Alloc(sizeof(*Node));
        return Node;
}

void __ParseTree_Init(parse_tree_node *Node, gs_allocator Allocator) {
        Node->Allocator = Allocator;

        Node->Token.Text = NULL;
        Node->Token.TextLength = 0;
        Node->Token.Type = Token_Unknown;
        Node->Token.Line = 0;
        Node->Token.Column = 0;

        Node->Name = GSNullPtr;
        Node->NameLength = 0;
        Node->Children = GSNullPtr;
        Node->NumChildren = 0;
        Node->Capacity = 0;

        return;
}

parse_tree_node *ParseTreeInit(gs_allocator Allocator) {
        parse_tree_node *Node = __ParseTree_Alloc(Allocator);
        __ParseTree_Init(Node, Allocator);
        return Node;
}

/* Name must be a NULL-terminated string! */
void ParseTreeSetName(parse_tree_node *Node, char *Name) {
        u32 NameLength = GSStringLength(Name);
        if (Node->Name != NULL) {
                Node->Allocator.Free(Node->Name);
        }

        Node->Name = (char *)Node->Allocator.Alloc(NameLength + 1);
        GSStringCopy(Name, Node->Name, NameLength);
        Node->NameLength = NameLength;
}

void ParseTreeSetToken(parse_tree_node *Node, token Token) {
        token *This = &(Node->Token);
        This->Text = Token.Text;
        This->TextLength = Token.TextLength;
        This->Type = Token.Type;
        This->Line = Token.Line;
        This->Column = Token.Column;
}

void ParseTreeSet(parse_tree_node *Self, char *Name, token Token) {
        ParseTreeSetName(Self, Name);
        ParseTreeSetToken(Self, Token);
}

bool __ParseTree_AddChild(parse_tree_node *Self, char *Name) {
        u32 NameLength = GSStringLength(Name);
        u32 AllocCount = DEFAULT_ALLOC_COUNT;

        if (Self->Children == NULL) {
                Self->Children = (parse_tree_node *)Self->Allocator.Alloc(sizeof(parse_tree_node) * AllocCount);
                if (Self->Children == NULL) {
                        __ParseTree_LastError = ParseTreeErrorChildAlloc;
                        return false;
                }
                Self->Capacity = AllocCount;
                for (int i=0; i<AllocCount; i++) {
                        __ParseTree_Init(&Self->Children[i], Self->Allocator);
                }
        } else if (Self->Capacity <= Self->NumChildren) {
                if (Self->Capacity > 0) {
                        AllocCount = Self->Capacity * 2;
                        Self->Children = (parse_tree_node *)Self->Allocator.Realloc(Self->Children, sizeof(parse_tree_node) * AllocCount);
                        if (Self->Children == NULL) {
                                __ParseTree_LastError = ParseTreeErrorChildAlloc;
                                return false;
                        }
                        Self->Capacity = AllocCount;
                        for (int i=Self->NumChildren; i<AllocCount; i++) {
                                __ParseTree_Init(&Self->Children[i], Self->Allocator);
                        }
                }
        }

        ParseTreeSetName(&Self->Children[Self->NumChildren], Name);
        Self->NumChildren++;

        return true;
}

void ParseTreeNewChildren(parse_tree_node *Self, u32 Count) {
        for (int i=0; i<Count; i++) {
                __ParseTree_AddChild(Self, "Empty");
        }
}

void ParseTreeDeinit(parse_tree_node *Self) {
        if (Self == NULL) {
                return;
        }

        if (Self->Children != NULL) {
                Self->Allocator.Free(Self->Children);
                Self->Children = NULL;
                Self->NumChildren = 0;
                Self->Capacity = 0;
        }

        if (Self->Name != NULL) {
                Self->Allocator.Free(Self->Name);
                Self->Name = NULL;
                Self->NameLength = 0;
        }

        Self->Allocator.Free(Self);
}

void ParseTreePrint(parse_tree_node *Self, u32 IndentLevel, u32 IndentIncrement) {
        int MinCompareLength = GSMin(Self->NameLength, 5);
        if (GSStringIsEqual("Empty", Self->Name, MinCompareLength)) return;

        if (Self->Token.Type != Token_Unknown) {
                printf("[%4d,%3d] ", Self->Token.Line, Self->Token.Column);
        } else {
                printf("           ");
        }

        if (IndentLevel > 0) printf("%*c", IndentLevel * IndentIncrement, ' ');

        if (Self->NameLength > 0) {
                printf("%s", Self->Name);
        } else {
                printf("Unknown Name");
        }

        if (Self->Token.Type != Token_Unknown) printf("( %.*s )", (u32)(Self->Token.TextLength), Self->Token.Text);

        printf("\n");

        for (int i=0; i<Self->NumChildren; i++) {
                ParseTreePrint(&Self->Children[i], IndentLevel + 1, IndentIncrement);
        }
}

#endif // PARSE_TREE
