/******************************************************************************
 * File: ast.c
 * Created: 2020-11-16
 * Updated: 2020-11-16
 * Package: C-Parser
 * Creator: Aaron Oman (GrooveStomp)
 * Homepage: https://git.sr.ht/~groovestomp/c-parser
 * Copyright 2020 - 2020, Aaron Oman and the C-Parser contributors
 * SPDX-License-Identifier: LGPL-3.0-only
 ******************************************************************************/
#ifndef AST_C
#define AST_C

#include "gs.h"
#include "parse_tree.c"

typedef union AstValue {
        u8 unsigned_byte;
        i8 signed_byte;
        u16 unsigned_short;
        i16 signed_short;
        u32 unsigned_word;
        i32 signed_word;
        u64 unsigned_long;
        i64 signed_long;
        f32 floating;
        f64 double_precision;
        f128 quad_precision;
        void *pointer;
        bool boolean;
        u8 none;
} AstValue;

typedef enum AstValueType {
        AstValue_UnsignedInt,
        AstValue_SignedByte,
        AstValue_UnsignedShort,
        AstValue_SignedShort,
        AstValue_UnsignedWord,
        AstValue_SignedWord,
        AstValue_UnsignedLong,
        AstValue_SignedLong,
        AstValue_UnsignedLongLong,
        AstValue_SignedLongLong,
        AstValue_Floating,
        AstValue_DoublePrecision,
        AstValue_QuadPrecision,
        AstValue_Pointer,
        AstValue_Boolean,

        AstValue_None,
        AstValue_Count = AstValue_None,
} AstValueType;

typedef enum AstNodeType {
        AstNode_LogicalAnd,
        AstNode_LogicalOr,
        AstNode_MathAdd,
        AstNode_MathSub,
        AstNode_MathMult,
        AstNode_MathDiv,
        AstNode_Assignment,

        AstNode_None,
        AstNode_Count = AstNode_None,
} AstNodeType;

typedef struct AstNode {
        gs_Allocator allocator;
        struct AstNode *children;
        u32 num_children;
        u32 capacity;
        AstValueType value_type;
        AstValue value;
        AstNodeType type;
} AstNode;

typedef struct Ast {
        gs_Allocator allocator;
} Ast;

bool AstInit(Ast *ast, gs_Allocator allocator) {
        ast->allocator = allocator;
}

bool AstNodeDo(AstNode *ast_node, ParseTreeNode *parse_node) {
        switch (parse_node->type) {
                case ParseTreeNode_LogicalAndExpression: {
                        ast_node->type = AstNode_LogicalAnd;
                        ast_node->value_type = AstValue_None;
                        ast_node->num_children = 2;
                } break;
                case ParseTreeNode_CastExpression: {
                        /*
                          type <- (left)
                          expression <- (right)
                        */
                } break;
        }
}

#endif /* AST_C */
