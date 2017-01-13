#ifndef _TREE_C
#define _TREE_C

#include "gs.h"
#include "lexer.c"

#include <errno.h>

/*
  NOTE: Super Shitty Debug Tree Implementation.
  This is just used for debugging purposes!
  Does allocation via malloc and stuff. Blech.
*/

typedef struct parse_tree_node
{
        token *Token;
        char *Name;
        unsigned int NameLength;
        struct parse_tree_node **Children;
        unsigned int NumChildren;
} parse_tree_node;

parse_tree_node *
ParseTreeNew(void)
{
        parse_tree_node *Self = (parse_tree_node *)malloc(sizeof(parse_tree_node));
        Self->Token = GSNullPtr;
        Self->Name = GSNullPtr;
        Self->NameLength = 0;
        Self->Children = GSNullPtr;
        Self->NumChildren = 0;
        return(Self);
}

void
ParseTreeSet(parse_tree_node *Node, token *Token, char *Name, unsigned int NameLength)
{
        if(Node->Name != NULL)
        {
                free(Node->Name);
        }
        Node->Name = malloc(NameLength + 1);
        GSStringCopy(Name, Node->Name, NameLength);
        Node->NameLength = NameLength;

        /* Early exit. No need to copy Token data. */
        if(Token == NULL)
        {
                return;
        }

        if(Node->Token != NULL)
        {
                free(Node->Token);
        }
        Node->Token = (token *)malloc(sizeof(token));
        if(Node->Token == NULL)
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
        Node->Token->Text = Token->Text;
        Node->Token->TextLength = Token->TextLength;
        Node->Token->Type = Token->Type;
        Node->Token->Line = Token->Line;
        Node->Token->Column = Token->Column;
}

void
ParseTreeAddChild(parse_tree_node *Self, token *Node, char *Name, unsigned int NameLength)
{
        /* NOTE(AARON): Self->NumChildren might not be zero. */
        if(Self->Children == NULL)
        {
                Self->Children = (parse_tree_node **)malloc(sizeof(token));
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
                ParseTreeSet(Self->Children[0], Node, Name, NameLength);
                Self->NumChildren = 1;
        }
        else
        {
                Self->Children = realloc(Self->Children, sizeof(token) * Self->NumChildren + 1);
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

                ParseTreeSet(Self->Children[Self->NumChildren], Node, Name, NameLength);
                Self->NumChildren++;
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
        }

        free(Self);
}

#endif /* _TREE_C */
