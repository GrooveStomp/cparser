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
        struct parse_tree_node *Children;
        unsigned int NumChildren;
} parse_tree_node;

parse_tree_node *
ParseTree__Init(parse_tree_node *Self)
{
        Self->Token = GSNullPtr;
        Self->Name = GSNullPtr;
        Self->NameLength = 0;
        Self->Children = GSNullPtr;
        Self->NumChildren = 0;
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
ParseTreeSet(parse_tree_node *Node, char *Name)
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

void ParseTreeNewChild(parse_tree_node *);
void ParseTreeAddChild(parse_tree_node *, char *);

void
ParseTreeNewChildren(parse_tree_node *Self, unsigned int Count)
{
        for(int i=0; i<Count; i++)
        {
                ParseTreeAddChild(Self, "Empty");
        }
}

void
ParseTreeNewChild(parse_tree_node *Self)
{
        /* NOTE(AARON): Self->NumChildren might not be zero. */
        if(Self->Children == NULL)
        {
                Self->Children = (parse_tree_node *)malloc(sizeof(token));
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
                ParseTreeSet(&Self->Children[0], "Empty");
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

                ParseTreeSet(&Self->Children[Self->NumChildren], "Empty");
                Self->NumChildren++;
        }
}

void
ParseTreeAddChild(parse_tree_node *Self, char *Name)
{
        unsigned int NameLength = GSStringLength(Name);

        /* NOTE(AARON): Self->NumChildren might not be zero. */
        if(Self->Children == NULL)
        {
                Self->Children = (parse_tree_node *)malloc(sizeof(token));
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
                ParseTree__Init(&Self->Children[0]);
                ParseTreeSet(&Self->Children[0], Name);
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

                ParseTree__Init(&Self->Children[Self->NumChildren]);
                ParseTreeSet(&Self->Children[Self->NumChildren], Name);
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
