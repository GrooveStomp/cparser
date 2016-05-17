/*
 * TODO(AARON):
 * - Parse data stream into tokens instead of buffers.
 * - Build a parse tree consisting of all tokens.
 */
#include <stdio.h>
#include <alloca.h>
#include <string.h>
#include <stdlib.h> /* EXIT_SUCCESS, EXIT_FAILURE */

#include "file_buffer.c"

typedef int bool;

#define ARRAY_SIZE(Array) (sizeof((Array)) / sizeof((Array)[0]))
#define false 0
#define true !false

#define min(a,b) ((a) < (b) ? (a) : (b))
#define bytes(n) (n)
#define kilobytes(n) (bytes(n) * 1000)

enum token_type
{
        Token_Unknown,

        Token_Asterisk,
        Token_Ampersand,
        Token_LeftParen,
        Token_RightParen,
        Token_LeftBracket,
        Token_RightBracket,
        Token_LeftBrace,
        Token_RightBrace,
        Token_Colon,
        Token_SemiColon,
        Token_Comma,            /*  , */
        Token_Cross,            /*  + */
        Token_Dash,             /*  - */
        Token_Equal,            /*  = */
        Token_Slash,            /*  / */
        Token_Dot,              /*  . */
        Token_Arrow,            /* -> */

        Token_Character,
        Token_String,
        Token_Identifier,
        Token_Keyword,

        Token_EndOfStream,
};

struct token
{
        char *Text;
        size_t TextLength;
        enum token_type Type;
};

struct tokenizer
{
        char *At;
};

void
AbortWithMessage(const char *msg)
{
        fprintf(stderr, "%s\n", msg);
        exit(EXIT_FAILURE);
}

void
Usage()
{
        printf("Usage: program file\n");
        printf("  file: must be a file in this directory\n");
        printf("  Specify '-h' or '--help' for this help text\n");
        exit(EXIT_SUCCESS);
}

bool
IsEndOfLine(char C)
{
        return((C == '\n') ||
               (C == '\r'));
}

bool
IsWhitespace(char C)
{
        return((C == ' ') ||
               (C == '\t') ||
               (C == '\v') ||
               (C == '\f') ||
               IsEndOfLine(C));
}

static void
EatAllWhitespace(struct tokenizer *Tokenizer)
{
        for(;;)
        {
                /* Eat all regular 'ol whitespace. */
                if(IsWhitespace(Tokenizer->At[0]))
                {
                        ++Tokenizer->At;
                }
                else
                {
                        break;
                }
        }
}

bool
IsSymbol(const char character)
{
        static char symbols[] = {
                '#', '{', '}', '[', ']', ',', ';', '-', '+', '=', '*', '^', '&', '%', '$', '?', '<', '>', '(', ')', '!',
                '/', '|', '~', '.', '\'', '"'
        };

        for(int i=0; i < ARRAY_SIZE(symbols); ++i)
        {
                if(character == symbols[i]) return(true);
        }
        return(false);
}

bool
GetFirstCharacterLiteral(const struct buffer *buff, struct buffer *character)
{
        char *string_open_match;
        char *string_close_match;

        /* TODO(AARON): Assuming entirety of string fits within both buffer and tmp_buffer. */
        char tmp_buffer[5] = {0};
        strncpy(tmp_buffer, buff->Data, min(buff->Length, 4));

        char *match;
        if((match = strstr(tmp_buffer, "'")) != NULL)
        {
                char *close = match + 2;
                if (*close != '\'') close += 1;
                if(*close != '\'') return(false);
                if(*close == '\'' && *(close-1) == '\\') close += 1;
                if(*close != '\'') return(false);

                character->Data = buff->Data + (match - tmp_buffer);
                character->Length = close - match + 1;
                character->Capacity = character->Length;
                return(true);
        }

        return(false);
}

bool
GetFirstStringLiteral(const struct buffer *buff, struct buffer *string)
{
        char *string_open_match;
        char *string_close_match;

        /* TODO(AARON): Assuming entirety of string fits within both buffer and tmp_buffer. */
        char tmp_buffer[kilobytes(1)] = {0};
        strncpy(tmp_buffer, buff->Data, min(buff->Length, kilobytes(1)-1));

        if((string_open_match = strstr(tmp_buffer, "\"")) != NULL)
        {
                /* TODO(AARON): Handle nested strings. */
                string_close_match = strstr(tmp_buffer + 1, "\"");
                char *close_tmp_buffer;
                while(*(string_close_match - 1) == '\\' || strncmp(string_close_match - 1, "'\"'", 4) == 0)
                {
                        close_tmp_buffer = string_close_match + 1;

                        if(close_tmp_buffer >= tmp_buffer + min(buff->Length, kilobytes(1)-1))
                        {
                                AbortWithMessage("Couldn't terminate string properly");
                        }

                        string_close_match = strstr(close_tmp_buffer, "\"");
                }
                /* TODO(AARON): Raise a big fuss if string_close_match is NULL. */

                string->Data = buff->Data + (string_open_match - tmp_buffer);
                string->Length = string_close_match - string_open_match + 1;
                string->Capacity = string->Length;

                return(true);
        }

        return(false);
}

bool
GetFirstComment(const struct buffer *buff, struct buffer *comment)
{
        char *comment_open_match;
        char *comment_close_match;

        /* TODO(AARON): Assuming entirety of comment fits within both buffer and tmp_buffer. */
        char tmp_buffer[kilobytes(1)] = {0};
        strncpy(tmp_buffer, buff->Data, min(buff->Length, kilobytes(1)-1));

        if((comment_open_match = strstr(tmp_buffer, "/*")) != NULL)
        {
                /* TODO(AARON): Handle nested comments. */
                comment_close_match = strstr(tmp_buffer, "*/");
                /* TODO(AARON): Raise a big fuss if comment_close_match is NULL. */

                comment->Data = buff->Data + (comment_open_match - tmp_buffer);
                comment->Length = comment_close_match - comment_open_match + 2;
                comment->Capacity = comment->Length;

                return(true);
        }

        return(false);
}

bool
GetKeyword(struct tokenizer *Tokenizer, struct token *Token)
{
        static char *Keywords[] = {
                "auto", "break", "case", "char", "const", "continue", "default", "do", "double", "else", "enum", "extern",
                "float", "for", "goto", "if", "int", "long", "register", "return", "short", "signed", "sizeof", "static",
                "struct", "switch", "typedef", "union", "unsigned", "void", "volatile", "while"
        };

        Token->Text = Tokenizer->At;
        Token->TextLength = 0;
        Token->Type = Token_Unknown;

        for(int i = 0; i < ARRAY_SIZE(Keywords); ++i)
        {
                if(strstr(Tokenizer->At, Keywords[i]) == Tokenizer->At)
                {
                        Token->TextLength = strlen(Keywords[i]);
                        Token->Type = Token_Keyword;
                        return(true);
                }
        }

        return(false);
}

bool
GetFirstPreprocessorCommand(const struct buffer *buff, struct buffer *preprocessor)
{
        char *match_start;
        char *match_end;

        char tmp_buffer[kilobytes(1)] = {0};
        strncpy(tmp_buffer, buff->Data, min(buff->Length, kilobytes(1)-1));

        if((match_start = strstr(tmp_buffer, "#")) != NULL)
        {
                for(--match_start; match_start > tmp_buffer && IsWhitespace(*match_start); --match_start);
                /* Needs to be a newline followed by any amount of whitespace, then '#'. */
                if(*match_start != '\n' && *match_start != '\0') return(false);
                ++match_start;

                match_end = match_start;

                while(1)
                {
                        if(match_end >= tmp_buffer + strlen(tmp_buffer)) return(false);

                        if(*match_end == '\n' && *(match_end-1) != '\\') break;

                        ++match_end;
                }

                preprocessor->Data = buff->Data + (match_start - tmp_buffer);
                preprocessor->Length = match_end - match_start;
                preprocessor->Capacity = preprocessor->Length;
                return(true);
        }
}

void
IncrementBuffer(struct buffer *buff, unsigned int count)
{
        buff->Data += count;
        buff->Length -= count;
        buff->Capacity -= count;
}

struct token
GetToken(struct tokenizer *Tokenizer)
{
        EatAllWhitespace(Tokenizer);

        struct token Token;
        struct buffer match;
        if(GetKeyword(Tokenizer, &Token))
        {
                /* snprintf(print_buffer, match.Length + strlen("<keyword>") + 1, "<keyword>%s", match.Data); */
                /* puts(print_buffer); */
                /* IncrementBuffer(&FileContents, match.Length); */
                /* pos += match.Length; */
        }
        /* else if(GetFirstCharacterLiteral(&FileContents, &match) && match.Data == FileContents.Data) */
        /*         { */
        /*                 snprintf(print_buffer, match.Length + strlen("<character literal>") + 1, "<character literal>%s", match.Data); */
        /*                 puts(print_buffer); */
        /*                 IncrementBuffer(&FileContents, match.Length); */
        /*                 pos += match.Length; */
        /*         } */
        /* else if(GetFirstComment(&FileContents, &match) && match.Data == FileContents.Data) */
        /*         { */
        /*                 snprintf(print_buffer, match.Length + strlen("<comment>") + 1, "<comment>%s", match.Data); */
        /*                 puts(print_buffer); */
        /*                 IncrementBuffer(&FileContents, match.Length); */
        /*                 pos += match.Length; */
        /*         } */
        /* else if(GetFirstStringLiteral(&FileContents, &match) && match.Data == FileContents.Data) */
        /*         { */
        /*                 snprintf(print_buffer, match.Length + strlen("<string literal>") + 1, "<string literal>%s", match.Data); */
        /*                 puts(print_buffer); */
        /*                 IncrementBuffer(&FileContents, match.Length); */
        /*                 pos += match.Length; */
        /*         } */
        /* else if(GetFirstPreprocessorCommand(&FileContents, &match) && match.Data == FileContents.Data) */
        /*         { */
        /*                 snprintf(print_buffer, match.Length + strlen("<preprocessor command>") + 1, "<preprocessor command>%s", match.Data); */
        /*                 puts(print_buffer); */
        /*                 IncrementBuffer(&FileContents, match.Length); */
        /*                 pos += match.Length; */
        /*         } */
        /* else if(IsSymbol(FileContents.Data[0])) */
        /*         { */
        /*                 snprintf(print_buffer, 2 + strlen("<symbol>"), "<symbol>%c", FileContents.Data[0]); */
        /*                 puts(print_buffer); */
        /*                 IncrementBuffer(&FileContents, 1); */
        /*                 pos += 1; */
        /*         } */
        /* else */
        /*         { */
        /*                 /\* printf("No token found at character [%u]\n", pos); *\/ */
        /*                 IncrementBuffer(&FileContents, 1); */
        /*                 pos += 1; */
        /*         } */
        return(Token);
}

/* argv[1] is the input file name. */
int
main(int argc, char *argv[])
{
        if(argc != 2 || strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0) Usage();

        size_t AllocSize = FileSize(argv[1]);
        struct buffer FileContents;

        /* Allocate space on the stack. */
        BufferSet(&FileContents, (char *)alloca(AllocSize), 0, AllocSize);

        if(COPY_FILE_OK != CopyFileIntoBuffer(argv[1], &FileContents))
        {
                AbortWithMessage("Couldn't copy entire file to buffer");
        }

        char print_buffer[kilobytes(1)] = { 0 };
        int pos = 0;

        struct tokenizer Tokenizer;
        Tokenizer.At = FileContents.Data;

        bool Parsing = true;
        while(Parsing)
        {
                struct token Token = GetToken(&Tokenizer);
                switch(Token.Type)
                {
                        case Token_EndOfStream: { Parsing = false; } break;
                        default: { } break;
                }
        }

        return(0);
}
