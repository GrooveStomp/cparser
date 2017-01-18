#ifndef _TREE_C
#define _TREE_C

#include "gs.h"
#include "lexer.c"

#include <errno.h>

#define DEFAULT_ALLOC_COUNT 2

/*
  NOTE: Super Shitty Debug Tree Implementation.
  This is just used for debugging purposes!
  Does allocation via malloc and stuff. Blech.
*/

typedef struct parse_tree_node
{
        token Token;
        char *Name;
        unsigned int NameLength;
        struct parse_tree_node *Children;
        unsigned int NumChildren;
        unsigned int NumAllocatedChildren;
} parse_tree_node;

void
ParseTree__InitToken(token *Token)
{
        Token->Text = NULL;
        Token->TextLength = 0;
        Token->Type = Token_Unknown;
        Token->Line = 0;
        Token->Column = 0;
}

parse_tree_node *
ParseTree__Init(parse_tree_node *Self)
{
        ParseTree__InitToken(&Self->Token);
        Self->Name = GSNullPtr;
        Self->NameLength = 0;
        Self->Children = GSNullPtr;
        Self->NumChildren = 0;
        Self->NumAllocatedChildren = 0;
        return(Self);
}

parse_tree_node *
ParseTreeNew(void)
{
        parse_tree_node *Self = (parse_tree_node *)malloc(sizeof(parse_tree_node));
        ParseTree__Init(Self);
        return(Self);
}

void /* Name must be a NULL-terminated string! */
ParseTreeSetName(parse_tree_node *Node, char *Name)
{
        unsigned int NameLength = GSStringLength(Name);
        if(Node->Name != NULL)
        {
                free(Node->Name);
        }
        Node->Name = (char *)malloc(NameLength + 1);
        GSStringCopy(Name, Node->Name, NameLength);
        Node->NameLength = NameLength;
}

void
ParseTreeSetToken(parse_tree_node *Node, token Token)
{
        token *This = &(Node->Token);
        This->Text = Token.Text;
        This->TextLength = Token.TextLength;
        This->Type = Token.Type;
        This->Line = Token.Line;
        This->Column = Token.Column;
}

void
ParseTreeSet(parse_tree_node *Self, char *Name, token Token)
{
        ParseTreeSetName(Self, Name);
        ParseTreeSetToken(Self, Token);
}

void
ParseTreeAddChild(parse_tree_node *Self, char *Name)
{
        unsigned int NameLength = GSStringLength(Name);
        unsigned int AllocCount = DEFAULT_ALLOC_COUNT;

        if(Self->Children == NULL)
        {
                Self->Children = (parse_tree_node *)malloc(sizeof(parse_tree_node) * AllocCount);
                if(Self->Children == NULL)
                {
                        int errval = errno;
                        if(errval == ENOMEM)
                        {
                                GSAbortWithMessage("Couldn't allocate node in debug tree! Out of memory!\n");
                        }
                        else
                        {
                                GSAbortWithMessage("Couldn't allocate node in debug tree! I don't know why!\n");
                        }
                }
                Self->NumAllocatedChildren = AllocCount;
                for(int i=0; i<AllocCount; i++)
                {
                        ParseTree__Init(&Self->Children[i]);
                }
        }
        else if(Self->NumAllocatedChildren <= Self->NumChildren)
        {
                if(Self->NumAllocatedChildren > 0)
                {
                        AllocCount = Self->NumAllocatedChildren * 2;
                        Self->Children = (parse_tree_node *)realloc(Self->Children, sizeof(parse_tree_node) * AllocCount);
                        if(Self->Children == NULL)
                        {
                                int errval = errno;
                                if(errval == ENOMEM)
                                {
                                        GSAbortWithMessage("Couldn't allocate node in debug tree! Out of memory!\n");
                                }
                                else
                                {
                                        GSAbortWithMessage("Couldn't allocate node in debug tree! I don't know why!\n");
                                }
                        }
                        Self->NumAllocatedChildren = AllocCount;
                        for(int i=Self->NumChildren; i<AllocCount; i++)
                        {
                                ParseTree__Init(&Self->Children[i]);
                        }
                }
        }

        ParseTreeSetName(&Self->Children[Self->NumChildren], Name);
        Self->NumChildren++;
}

void
ParseTreeNewChildren(parse_tree_node *Self, unsigned int Count)
{
        for(int i=0; i<Count; i++)
        {
                ParseTreeAddChild(Self, "Empty");
        }
}

void
ParseTreeFree(parse_tree_node *Self)
{
        if(Self == NULL)
        {
                return;
        }

        if(Self->Children != NULL)
        {
                free(Self->Children);
                Self->Children = NULL;
                Self->NumChildren = 0;
                Self->NumAllocatedChildren = 0;
        }

        if(Self->Name != NULL)
        {
                free(Self->Name);
                Self->Name = NULL;
                Self->NameLength = 0;
        }

        free(Self);
}

void
ParseTreePrint(parse_tree_node *Self, unsigned int IndentLevel, unsigned int IndentIncrement)
{
        int MinCompareLength = GSMin(Self->NameLength, 5);
        if(GSStringIsEqual("Empty", Self->Name, MinCompareLength)) return;

        if(Self->Token.Type != Token_Unknown)
        {
                printf("[%4d,%3d] ", Self->Token.Line, Self->Token.Column);
        }
        else
                printf("           ");

        if(IndentLevel > 0) printf("%*c", IndentLevel * IndentIncrement, ' ');

        if(Self->NameLength > 0)
                printf("%s", Self->Name);
        else
                printf("Unknown Name");

        if(Self->Token.Type != Token_Unknown) printf("( %.*s )", Self->Token.TextLength, Self->Token.Text);

        printf("\n");

        for(int i=0; i<Self->NumChildren; i++)
        {
                ParseTreePrint(&Self->Children[i], IndentLevel + 1, IndentIncrement);
        }
}

#endif /* _TREE_C */
