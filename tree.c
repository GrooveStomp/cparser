#ifndef TREE2
#define TREE2

#include <stdint.h>
#include <stddef.h>
#include <errno.h>
#include "gs.h"
#include "lexer.c"

#define DEFAULT_ALLOC_COUNT 2

typedef void *(*parse_tree_alloc)(size_t);
typedef void (*parse_tree_free)(void *);
typedef void *(*parse_tree_realloc)(void *, size_t);

typedef struct parse_tree_allocator {
        parse_tree_alloc Alloc;
        parse_tree_free Free;
        parse_tree_realloc Realloc;
} parse_tree_allocator;

typedef struct parse_tree_node {
        parse_tree_allocator Allocator;
        uint32_t NameLength;
        uint32_t Capacity;
        uint32_t NumChildren;
        token Token;
        char *Name;
        struct parse_tree_node *Children;
} parse_tree_node;

parse_tree_node *ParseTreeInit(parse_tree_allocator Allocator);
void ParseTreeDeinit(parse_tree_node *Node);

void ParseTreeSetName(parse_tree_node *Node, char *Name);
void ParseTreeSetToken(parse_tree_node *Node, token Token);
void ParseTreeSet(parse_tree_node *Self, char *Name, token Token);
void ParseTreeNewChildren(parse_tree_node *Self, uint32_t Count);
void ParseTreePrint(parse_tree_node *Self, uint32_t IndentLevel, uint32_t IndentIncrement);

parse_tree_node *__tree_Alloc(parse_tree_allocator Allocator) {
        parse_tree_node *Node = (parse_tree_node *)Allocator.Alloc(sizeof(*Node));
        return Node;
}

void __tree_Init(parse_tree_node *Node, parse_tree_allocator Allocator) {
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

parse_tree_node *ParseTreeInit(parse_tree_allocator Allocator) {
        parse_tree_node *Node = __tree_Alloc(Allocator);
        __tree_Init(Node, Allocator);
        return Node;
}

/* Name must be a NULL-terminated string! */
void ParseTreeSetName(parse_tree_node *Node, char *Name) {
        unsigned int NameLength = GSStringLength(Name);
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

void __tree_AddChild(parse_tree_node *Self, char *Name) {
        unsigned int NameLength = GSStringLength(Name);
        unsigned int AllocCount = DEFAULT_ALLOC_COUNT;

        if (Self->Children == NULL) {
                Self->Children = (parse_tree_node *)malloc(sizeof(parse_tree_node) * AllocCount);
                if (Self->Children == NULL) {
                        int errval = errno;
                        if (errval == ENOMEM) {
                                GSAbortWithMessage("Couldn't allocate node in debug tree! Out of memory!\n");
                        } else {
                                GSAbortWithMessage("Couldn't allocate node in debug tree! I don't know why!\n");
                        }
                }
                Self->Capacity = AllocCount;
                for (int i=0; i<AllocCount; i++) {
                        __tree_Init(&Self->Children[i], Self->Allocator);
                }
        } else if (Self->Capacity <= Self->NumChildren) {
                if (Self->Capacity > 0) {
                        AllocCount = Self->Capacity * 2;
                        Self->Children = (parse_tree_node *)realloc(Self->Children, sizeof(parse_tree_node) * AllocCount);
                        if (Self->Children == NULL) {
                                int errval = errno;
                                if (errval == ENOMEM) {
                                        GSAbortWithMessage("Couldn't allocate node in debug tree! Out of memory!\n");
                                } else {
                                        GSAbortWithMessage("Couldn't allocate node in debug tree! I don't know why!\n");
                                }
                        }
                        Self->Capacity = AllocCount;
                        for (int i=Self->NumChildren; i<AllocCount; i++) {
                                __tree_Init(&Self->Children[i], Self->Allocator);
                        }
                }
        }

        ParseTreeSetName(&Self->Children[Self->NumChildren], Name);
        Self->NumChildren++;
}

void ParseTreeNewChildren(parse_tree_node *Self, unsigned int Count) {
        for (int i=0; i<Count; i++) {
                __tree_AddChild(Self, "Empty");
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

void ParseTreePrint(parse_tree_node *Self, uint32_t IndentLevel, uint32_t IndentIncrement) {
        int MinCompareLength = GSMin(Self->NameLength, 5);
        if (GSStringIsEqual("Empty", Self->Name, MinCompareLength)) return;

        if (Self->Token.Type != Token_Unknown) {
                printf("[%4d,%3d] ", Self->Token.Line, Self->Token.Column);
        }
        else {
                printf("           ");
        }

        if (IndentLevel > 0) printf("%*c", IndentLevel * IndentIncrement, ' ');

        if (Self->NameLength > 0) {
                printf("%s", Self->Name);
        } else {
                printf("Unknown Name");
        }

        if (Self->Token.Type != Token_Unknown) printf("( %.*s )", Self->Token.TextLength, Self->Token.Text);

        printf("\n");

        for (int i=0; i<Self->NumChildren; i++) {
                ParseTreePrint(&Self->Children[i], IndentLevel + 1, IndentIncrement);
        }
}

#endif // TREE2
