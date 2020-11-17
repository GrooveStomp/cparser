/******************************************************************************
 * File: lexer.c
 * Created: 2016-07-06
 * Updated: 2020-11-16
 * Package: C-Parser
 * Creator: Aaron Oman (GrooveStomp)
 * Copyright 2016 - 2020, Aaron Oman and the C-Parser contributors
 * SPDX-License-Identifier: LGPL-3.0-only
 ******************************************************************************/
#ifndef LEXER_C
#define LEXER_C

#include "gs.h"

typedef enum LexerErrorEnum {
        LexerErrorNoSpace,
        LexerReallocFail,
        LexerUnknownToken,
        LexerErrorNone,
} LexerErrorEnum;

const char *__lexer_error_strings[] = {
        "Couldn't allocate space for token stream",
        "Couldn't reallocate space for token stream after lexing completed",
        "Unknown token",
        "No error",
};

LexerErrorEnum __lexer_last_error = LexerErrorNone;

const char *LexerErrorString() {
        const char *result = __lexer_error_strings[__lexer_last_error];
        __lexer_last_error = LexerErrorNone;
        return result;
}

typedef enum TokenType {
        Token_Unknown,

        Token_Asterisk,
        Token_Ampersand,
        Token_OpenParen,
        Token_CloseParen,
        Token_OpenBracket,
        Token_CloseBracket,
        Token_OpenBrace,
        Token_CloseBrace,
        Token_Colon,
        Token_SemiColon,
        Token_PercentSign,
        Token_QuestionMark,
        Token_EqualSign,        /*  = */
        Token_Carat,
        Token_Comma,            /*  , */
        Token_Cross,            /*  + */
        Token_Dash,             /*  - */
        Token_Slash,            /*  / */
        Token_Dot,              /*  . */
        Token_Hash,             /*  # */
        Token_Bang,
        Token_Pipe,
        Token_LessThan,
        Token_GreaterThan,
        Token_Tilde,

        Token_NotEqual,
        Token_GreaterThanEqual,
        Token_LessThanEqual,
        Token_LogicalEqual,
        Token_LogicalOr,
        Token_LogicalAnd,
        Token_BitShiftLeft,
        Token_BitShiftRight,
        Token_Arrow,            /* -> */
        Token_PlusPlus,
        Token_MinusMinus,
        Token_MultiplyEquals,
        Token_DivideEquals,
        Token_ModuloEquals,
        Token_PlusEquals,
        Token_MinusEquals,
        Token_DoubleLessThanEquals,
        Token_DoubleGreaterThanEquals,
        Token_AmpersandEquals,
        Token_CaratEquals,
        Token_PipeEquals,
        Token_Ellipsis,         /* ... */

        Token_Character,
        Token_String,
        Token_Identifier,
        Token_Keyword,
        Token_PreprocessorCommand,
        Token_Comment,
        Token_Integer,
        Token_PrecisionNumber,

        Token_EndOfStream,
} TokenType;

char *TokenName(TokenType type) {
        switch (type) {
                case Token_Unknown: { return "Unknown"; } break;

                case Token_Asterisk: { return "Asterisk"; } break;
                case Token_Ampersand: { return "Ampersand"; } break;
                case Token_OpenParen: { return "OpenParen"; } break;
                case Token_CloseParen: { return "CloseParen"; } break;
                case Token_OpenBracket: { return "OpenBracket"; } break;
                case Token_CloseBracket: { return "CloseBracket"; } break;
                case Token_OpenBrace: { return "OpenBrace"; } break;
                case Token_CloseBrace: { return "CloseBrace"; } break;
                case Token_Colon: { return "Colon"; } break;
                case Token_SemiColon: { return "SemiColon"; } break;
                case Token_PercentSign: { return "PercentSign"; } break;
                case Token_QuestionMark: { return "QuestionMark"; } break;
                case Token_EqualSign: { return "EqualSign"; } break;
                case Token_Carat: { return "Carat"; } break;
                case Token_Comma: { return "Comma"; } break;
                case Token_Cross: { return "Cross"; } break;
                case Token_Dash: { return "Dash"; } break;
                case Token_Slash: { return "Slash"; } break;
                case Token_Dot: { return "Dot"; } break;
                case Token_Hash: { return "Hash"; } break;
                case Token_Bang: { return "Bang"; } break;
                case Token_Pipe: { return "Pipe"; } break;
                case Token_LessThan: { return "LessThan"; } break;
                case Token_GreaterThan: { return "GreaterThan"; } break;
                case Token_Tilde: { return "Tilde"; } break;

                case Token_NotEqual: { return "NotEqual"; } break;
                case Token_GreaterThanEqual: { return "GreaterThanEqual"; } break;
                case Token_LessThanEqual: { return "LessThanEqual"; } break;
                case Token_LogicalEqual: { return "LogicalEqual"; } break;
                case Token_LogicalOr: { return "LogicalOr"; } break;
                case Token_LogicalAnd: { return "LogicalAnd"; } break;
                case Token_BitShiftLeft: { return "BitShiftLeft"; } break;
                case Token_BitShiftRight: { return "BitShiftRight"; } break;
                case Token_Arrow: { return "Arrow"; } break;
                case Token_PlusPlus: { return "PlusPlus"; } break;
                case Token_MinusMinus: { return "MinusMinus"; } break;

                case Token_MultiplyEquals: { return "MultiplyEquals"; } break;
                case Token_DivideEquals: { return "DivideEquals"; } break;
                case Token_ModuloEquals: { return "ModuloEquals"; } break;
                case Token_PlusEquals: { return "PlusEquals"; } break;
                case Token_MinusEquals: { return "MinusEquals"; } break;
                case Token_DoubleLessThanEquals: { return "DoubleLessThanEquals"; } break;
                case Token_DoubleGreaterThanEquals: { return "DoubleGreaterThanEquals"; } break;
                case Token_AmpersandEquals: { return "AmpersandEquals"; } break;
                case Token_CaratEquals: { return "CaratEquals"; } break;
                case Token_PipeEquals: { return "PipeEquals"; } break;
                case Token_Ellipsis: { return "Ellipsis"; } break;

                case Token_Character: { return "Character"; } break;
                case Token_String: { return "String"; } break;
                case Token_Identifier: { return "Identifier"; } break;
                case Token_Keyword: { return "Keyword"; } break;
                case Token_PreprocessorCommand: { return "PreprocessorCommand"; } break;
                case Token_Comment: { return "Comment"; } break;
                case Token_Integer: { return "Integer"; } break;
                case Token_PrecisionNumber: { return "PrecisionNumber"; } break;

                case Token_EndOfStream: { return "EndOfStream"; } break;
                default: { return "UnknownToken"; } break;
        }
}

typedef struct Token {
        char *text;
        u32 text_length;
        TokenType type;
        u32 line;
        u32 column;
} Token;

typedef struct Tokenizer {
        char *beginning;
        char *at;
        u32 line;
        u32 column;
} Tokenizer;

void TokenizerInit(Tokenizer *tokenizer, char *memory) {
        tokenizer->beginning = memory;
        tokenizer->at = memory;
        tokenizer->line = 0;
        tokenizer->column = 0;
}

void AdvanceTokenizer(Tokenizer *tokenizer) {
        if (gs_CharIsEndOfLine(tokenizer->at[0])) {
                ++tokenizer->line;
                tokenizer->column = 1;
        } else {
                ++tokenizer->column;
        }
        ++tokenizer->at;
}

void CopyTokenizer(Tokenizer *source, Tokenizer *dest) {
        dest->beginning = source->beginning;
        dest->at = source->at;
        dest->line = source->line;
        dest->column = source->column;
}

void AdvanceTokenizerToChar(Tokenizer *tokenizer, char c) {
        while (!gs_CharIsEndOfStream(tokenizer->at[0])) {
                if (tokenizer->at[0] == c) break;
                AdvanceTokenizer(tokenizer);
        }
}

void CopyToTokenAndAdvance(Tokenizer *tokenizer, Token *token, u32 length, TokenType type) {
        token->text = tokenizer->at;
        token->text_length = length;
        token->type = type;
        token->line = tokenizer->line;
        token->column = tokenizer->column;

        for (i32 i =  0; i < length; ++i) AdvanceTokenizer(tokenizer);
}

void EatAllWhitespace(Tokenizer *tokenizer) {
        for (; gs_CharIsWhitespace(tokenizer->at[0]); AdvanceTokenizer(tokenizer));
}

bool IsIdentifierCharacter(char c) {
	return (gs_CharIsAlphabetical(c) || gs_CharIsDecimal(c) || c == '_');
}

bool IsIntegerSuffix(char c) {
        return (c == 'u' || c == 'U' || c == 'l' || c == 'L');
}

bool GetCharacter(Tokenizer *tokenizer, Token *token) {
        char *cursor = tokenizer->at;

        /* First character must be a single quote. */
        if (*cursor != '\'') return false;
        ++cursor; /* Skip past the first single quote. */

        /* Read until closing single quote. */
        for (; *cursor != '\''; ++cursor);

        /* If previous character is an escape, then closing quote is next char. */
        if (*(cursor-1) == '\\' && *(cursor -2) != '\\') {
                ++cursor;
        }
        ++cursor; /* Point to character after literal. */

        /* Longest char literal is: '\''. */
        if (cursor - tokenizer->at > 4) return false;

        CopyToTokenAndAdvance(tokenizer, token, cursor - tokenizer->at, Token_Character);

        return true;
}

bool GetString(Tokenizer *tokenizer, Token *token) {
        char *cursor = tokenizer->at;
        if (*cursor != '"') return false;

        while (true) {
                ++cursor;
                if (*cursor == '\0') return false;
                if (*cursor == '"' && *(cursor - 1) != '\\') break;
        }
        ++cursor; /* Swallow the last double quote. */

        CopyToTokenAndAdvance(tokenizer, token, cursor - tokenizer->at, Token_String);

        return true;
}

bool IsOctalString(char *text, int length) {
        if (length < 2) return false;

        char last = text[length-1];

        /* Leading 0 required for octal integers. */
        if ('0' != text[0]) return false;

        if (!(gs_CharIsOctal(last) || IsIntegerSuffix(last))) return false;

        /* Loop from character after leading '0' to second-last. */
        for (int i = 1; i < length - 1; ++i) {
                if (!gs_CharIsOctal(text[i])) return false;
        }

        return true;
}

bool IsHexadecimalString(char *text, int length) {
        if (length < 3) return false;

        char last = text[length-1];

        /* Hex numbers must start with: '0x' or '0X'. */
        if (!(text[0] == '0' && (text[1] == 'x' || text[1] == 'X'))) return false;

        if (!(gs_CharIsHexadecimal(last) || IsIntegerSuffix(last))) return false;

        /* Loop from character after leading '0x' to second-last. */
        for (int i = 2; i < length - 1; ++i) {
                if (!gs_CharIsHexadecimal(text[i])) return false;
        }

        return true;
}

bool GetPrecisionNumber(Tokenizer *tokenizer, Token *token) {
        /*
          A floating constant consists of an integer part, a decimal point, a
          fraction part, an e or E, an optionally signed integer exponent and an
          optional type suffix, one of f, F, l, or L.
          The integer and fraction parts both consist of a sequence of digits.
          Either the integer part or the fraction part (not both) may be
          missing; either the decimal point or the e and the exponent (not both)
          may be missing.
          The type is determined by the suffix; F or f makes it float, L or l
          makes it long double; otherwise it is double.
        */

        char *cursor = tokenizer->at;

        bool has_integer_part = false;
        bool has_decimal_point = false;
        bool has_fractional_part = false;
        bool has_exponent_part = false;

        if (gs_CharIsDecimal(*cursor)) {
                for (; gs_CharIsDecimal(*cursor); ++cursor);
                has_integer_part = true;
        }

        if ('.' == *cursor) {
                ++cursor;
                has_decimal_point = true;
                if (gs_CharIsDecimal(*cursor))
                {
                        for (; gs_CharIsDecimal(*cursor); ++cursor);
                        has_fractional_part = true;
                }
        }

        if ('e' == *cursor || 'E' == *cursor) {
                ++cursor;
                has_exponent_part = true;

                /* Optional negative sign for exponent is allowed. */
                if ('-' == *cursor) {
                        ++cursor;
                }

                /* Exponent must contain an exponent part. */
                if (!gs_CharIsDecimal(*cursor)) {
                        return false;
                }

                for (; gs_CharIsDecimal(*cursor); ++cursor);
        }

        /* IsFloatSuffix(C) */
        char c = *cursor;
        if ('f' == c || 'F' == c || 'l' == c || 'L' == c) {
                ++cursor;
        }

        if ((has_integer_part || has_fractional_part) &&
           (has_decimal_point || has_exponent_part)) {
                CopyToTokenAndAdvance(tokenizer, token, cursor - tokenizer->at, Token_PrecisionNumber);
                return true;
        }

        return false;
}

bool GetInteger(Tokenizer *tokenizer, Token *token) {
        char *last_char = tokenizer->at;
        for (; *last_char && (gs_CharIsDecimal(*last_char) || gs_CharIsAlphabetical(*last_char)); ++last_char);

        int length = last_char - tokenizer->at;
        --last_char;

        /* Token->Type = Token_Unknown; */
        /* Token->Text = Tokenizer->At; */
        /* Token->Textlength = length; */

        if (length < 1) return false;

        char *cursor = tokenizer->at;

        if ((IsOctalString(cursor, length) || IsHexadecimalString(cursor, length)) ||
            (1 == length && '0' == *cursor)) {
                CopyToTokenAndAdvance(tokenizer, token, length, Token_Integer);
                return true;
        }

        /* Can't have a multi-digit integer starting with zero unless it's Octal. */
        if ('0' == *cursor) {
                return false;
        }

        for (int i=0; i<length-1; ++i) {
                if (!gs_CharIsDecimal(cursor[i])) {
                        return false;
                }
        }

        if (gs_CharIsDecimal(*last_char) || IsIntegerSuffix(*last_char)) {
                CopyToTokenAndAdvance(tokenizer, token, length, Token_Integer);
                return true;
        }

        return false;
}

bool GetIdentifier(Tokenizer *tokenizer, Token *token) {
        if (!gs_CharIsAlphabetical(tokenizer->at[0]) &&
           '_' != tokenizer->at[0]) {
                return false;
        }

        char *cursor = tokenizer->at;

        for (; IsIdentifierCharacter(*cursor); ++cursor);
        CopyToTokenAndAdvance(tokenizer, token, cursor - tokenizer->at, Token_Identifier);

        return true;
}

bool GetComment(Tokenizer *tokenizer, Token *token) {
        if (tokenizer->at[0] != '/' || tokenizer->at[1] != '*') return false;

        char *cursor = tokenizer->at;

        while (true) {
                if ('\0' == *cursor) {
                        return false;
                }

                if ('*' == cursor[0] && '/' == cursor[1]) {
                        break;
                }
                ++cursor;
        }

        cursor += 2; /* Swallow last two characters: asterisk, slash */

        CopyToTokenAndAdvance(tokenizer, token, cursor - tokenizer->at, Token_Comment);

        return true;
}

bool GetKeyword(Tokenizer *tokenizer, Token *token) {
        static char *keywords[] = {
                "auto", "break", "case", "char", "const", "continue", "default",
                "double", "do", "else", "enum", "extern", "float", "for",
                "goto", "if", "int", "long", "register", "return", "short",
                "signed", "sizeof", "static", "struct", "switch", "typedef",
                "union", "unsigned", "void", "volatile", "while"
        };

        for (int i = 0; i < gs_ArraySize(keywords); ++i) {
                if (gs_StringIsEqual(tokenizer->at, keywords[i], gs_StringLength(keywords[i]))) {
                        CopyToTokenAndAdvance(tokenizer, token, gs_StringLength(keywords[i]), Token_Keyword);

                        return true;
                }
        }

        return false;
}

bool GetPreprocessorCommand(Tokenizer *tokenizer, Token *token) {
        if (tokenizer->at[0] != '#') return false;

        char *cursor = tokenizer->at;

        /* Preprocessor commands must start a line on their own. */
        for (--cursor; cursor > tokenizer->beginning && gs_CharIsWhitespace(*cursor); --cursor);

        if (*(++cursor) != '\n' && cursor != tokenizer->beginning) {
                return false;
        }
        cursor = tokenizer->at + 1; /* Skip the starting '#' for macros. */

        while (true) {
                if (*cursor == '\n' && *(cursor - 1) != '\\') break;
                if (*cursor == '\0') break;
                ++cursor;
        }

        CopyToTokenAndAdvance(tokenizer, token, cursor - tokenizer->at, Token_PreprocessorCommand);

        return true;
}

bool GetSymbol(Tokenizer *tokenizer, Token *token, char *symbol, TokenType type) {
        int length = gs_StringLength(symbol);
        if (!gs_StringIsEqual(tokenizer->at, symbol, length)) return false;

        CopyToTokenAndAdvance(tokenizer, token, length, type);

        return true;
}

Token GetToken(Tokenizer *tokenizer) {
        EatAllWhitespace(tokenizer);

        Token token;
        token.text = tokenizer->at;
        token.text_length = 0;
        token.type = Token_Unknown;

        {
                GetSymbol(tokenizer, &token, "==", Token_LogicalEqual) ||
                        GetSymbol(tokenizer, &token, "<<=", Token_DoubleLessThanEquals) ||
                        GetSymbol(tokenizer, &token, ">>=", Token_DoubleGreaterThanEquals) ||
                        GetSymbol(tokenizer, &token, "...", Token_Ellipsis) ||
                        GetSymbol(tokenizer, &token, "!=", Token_NotEqual) ||
                        GetSymbol(tokenizer, &token, ">=", Token_GreaterThanEqual) ||
                        GetSymbol(tokenizer, &token, "<=", Token_LessThanEqual) ||
                        GetSymbol(tokenizer, &token, "->", Token_Arrow) ||
                        GetSymbol(tokenizer, &token, "||", Token_LogicalOr) ||
                        GetSymbol(tokenizer, &token, "&&", Token_LogicalAnd) ||
                        GetSymbol(tokenizer, &token, "<<", Token_BitShiftLeft) ||
                        GetSymbol(tokenizer, &token, ">>", Token_BitShiftRight) ||
                        GetSymbol(tokenizer, &token, "++", Token_PlusPlus) ||
                        GetSymbol(tokenizer, &token, "--", Token_MinusMinus) ||

                        GetSymbol(tokenizer, &token, "*=", Token_MultiplyEquals) ||
                        GetSymbol(tokenizer, &token, "/=", Token_DivideEquals) ||
                        GetSymbol(tokenizer, &token, "%=", Token_ModuloEquals) ||
                        GetSymbol(tokenizer, &token, "+=", Token_PlusEquals) ||
                        GetSymbol(tokenizer, &token, "-=", Token_MinusEquals) ||
                        GetSymbol(tokenizer, &token, "&=", Token_AmpersandEquals) ||
                        GetSymbol(tokenizer, &token, "^=", Token_CaratEquals) ||
                        GetSymbol(tokenizer, &token, "|=", Token_PipeEquals);

        }

        if (token.type != Token_Unknown) return token;

        {
                GetKeyword(tokenizer, &token) ||
                        GetCharacter(tokenizer, &token) ||
                        GetPreprocessorCommand(tokenizer, &token) ||
                        GetComment(tokenizer, &token) ||
                        GetString(tokenizer, &token) ||
                        GetPrecisionNumber(tokenizer, &token) ||
                        GetInteger(tokenizer, &token) ||
                        GetIdentifier(tokenizer, &token);
        }

        if (token.type == Token_PreprocessorCommand || token.type == Token_Comment) {
                token = GetToken(tokenizer);
        }

        if (token.type != Token_Unknown) return token;

        char c = tokenizer->at[0];
        CopyToTokenAndAdvance(tokenizer, &token, 1, Token_Unknown);

        switch (c) {
                case '\0':{ token.type = Token_EndOfStream; } break;
                case '(': { token.type = Token_OpenParen; } break;
                case ')': { token.type = Token_CloseParen; } break;
                case ':': { token.type = Token_Colon; } break;
                case ';': { token.type = Token_SemiColon; } break;
                case '*': { token.type = Token_Asterisk; } break;
                case '[': { token.type = Token_OpenBracket; } break;
                case ']': { token.type = Token_CloseBracket; } break;
                case '{': { token.type = Token_OpenBrace; } break;
                case '}': { token.type = Token_CloseBrace; } break;
                case ',': { token.type = Token_Comma; } break;
                case '-': { token.type = Token_Dash; } break;
                case '+': { token.type = Token_Cross; } break;
                case '=': { token.type = Token_EqualSign; } break;
                case '^': { token.type = Token_Carat; } break;
                case '&': { token.type = Token_Ampersand; } break;
                case '%': { token.type = Token_PercentSign; } break;
                case '?': { token.type = Token_QuestionMark; } break;
                case '!': { token.type = Token_Bang; } break;
                case '/': { token.type = Token_Slash; } break;
                case '|': { token.type = Token_Pipe; } break;
                case '<': { token.type = Token_LessThan; } break;
                case '>': { token.type = Token_GreaterThan; } break;
                case '~': { token.type = Token_Tilde; } break;
                case '.': { token.type = Token_Dot; } break;
                case '#': { token.type = Token_Hash; } break;
        }

        return token;
}

bool Lex(gs_Allocator allocator, gs_Buffer *input_stream, Token **out_stream, u32 *out_num_tokens) {
        Tokenizer tokenizer;
        TokenizerInit(&tokenizer, input_stream->start);

        // Overestimate allocation.
        // This assumes one token per char; which is way too much.
        // We'll resize later.
        Token *token_stream = *out_stream;
        token_stream = (Token *)allocator.malloc(sizeof(*token_stream) * input_stream->length);
        if (token_stream == GS_NULL_PTR) {
                __lexer_last_error = LexerErrorNoSpace;
                out_num_tokens = 0;
                return false;
        }

        u32 num_tokens = 0;

        bool lexing = true;
        while (lexing) {
                Token token = GetToken(&tokenizer);
                token_stream[num_tokens++] = token;
                switch (token.type) {
                        case Token_EndOfStream: {
                                lexing = false;
                        } break;

                        case Token_Unknown: {
                                __lexer_last_error = LexerUnknownToken;
                                lexing = false;
                        } break;
                }
        }

        // Resize the stream now.
        *out_stream = (Token *)allocator.realloc(token_stream, sizeof(*token_stream) * num_tokens);
        if (*out_stream == GS_NULL_PTR) {
                __lexer_last_error = LexerReallocFail;
                out_num_tokens = 0;
                return false;
        }

        *out_num_tokens = num_tokens;
        return true;
}

#endif /* LEXER_C */
