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
        Token_OpenParen,
        Token_CloseParen,
        Token_OpenBracket,
        Token_CloseBracket,
        Token_OpenBrace,
        Token_CloseBrace,
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
        Token_PreprocessorCommand,
        Token_Comment,

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
        char *Beginning;
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
        while(true)
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
GetCharacter(struct tokenizer *Tokenizer, struct token *Token)
{
        if(Tokenizer->At[0] != '\'') return(false);

        char *Close = Tokenizer->At + 1;
        if(*Close != '\'') Close += 1;
        if(*Close != '\'') return(false);
        if(*Close == '\'' && *(Close-1) == '\\') Close += 1;
        if(*Close != '\'') return(false);

        Token->Text = Tokenizer->At;
        Token->TextLength = Close - Tokenizer->At + 1;
        Token->Type = Token_Character;
        Tokenizer->At += Token->TextLength;
        return(true);
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
GetComment(struct tokenizer *Tokenizer, struct token *Token)
{
        if(Tokenizer->At[0] != '/' || Tokenizer->At[1] != '*') return(false);

        char *Text = Tokenizer->At;

        while(true)
        {
                if(Tokenizer->At[0] == '\0') return(false);
                if(Tokenizer->At[0] == '*' && Tokenizer->At[1] == '/') break;
                ++Tokenizer->At;
        }

        Tokenizer->At += 2; /* Swallow last two characters. */

        Token->Text = Text;
        Token->TextLength = Tokenizer->At - Token->Text;
        Token->Type = Token_Comment;
        return(true);
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
                        Tokenizer->At += Token->TextLength;
                        return(true);
                }
        }

        return(false);
}

bool
GetPreprocessorCommand(struct tokenizer *Tokenizer, struct token *Token)
{
        if(Tokenizer->At[0] != '#') return(false);

        char *Text, *Marker;
        Text = Marker = Tokenizer->At++;

        /* Preprocessor commands must start a line on their own. */
        for(--Marker; Marker > Tokenizer->Beginning && IsWhitespace(*Marker); --Marker);
        if(*(++Marker) != '\n') return(false);

        while(true)
        {
                if(Tokenizer->At[0] == '\n' && *(Tokenizer->At - 1) != '\\') break;
                if(Tokenizer->At[0] == '\0') break;
                ++Tokenizer->At;
        }

        Token->Text = Text;
        Token->TextLength = Tokenizer->At - Token->Text;
        Token->Type = Token_PreprocessorCommand;
        return(true);
}

struct token
GetToken(struct tokenizer *Tokenizer)
{
        EatAllWhitespace(Tokenizer);
        struct buffer match; /* TODO(AARON): Legacy - should be removed when unused */

        struct token Token;
        Token.Text = Tokenizer->At;
        Token.TextLength = 1;
        Token.Type = Token_Unknown;

        char C = Tokenizer->At[0];
        ++Tokenizer->At;

        switch(C)
        {
                /*
                  NOTE(AARON): Still need to check for these symbols!
                  '#', ',', '-', '+', '=', '^', '&', '%', '$', '?', '<', '>',
                  '!', '/', '|', '~', '.', '\'', '"'
                */
                case '\0':{ Token.Type = Token_EndOfStream; } break;
                case '(': { Token.Type = Token_OpenParen; } break;
                case ')': { Token.Type = Token_CloseParen; } break;
                case ':': { Token.Type = Token_Colon; } break;
                case ';': { Token.Type = Token_SemiColon; } break;
                case '*': { Token.Type = Token_Asterisk; } break;
                case '[': { Token.Type = Token_OpenBracket; } break;
                case ']': { Token.Type = Token_CloseBracket; } break;
                case '{': { Token.Type = Token_OpenBrace; } break;
                case '}': { Token.Type = Token_CloseBrace; } break;

                default: {
                        /*
                          Put the Tokenizer back to the start of the current
                          phrase. The following functions assume a "clean
                          slate" for operation.
                        */
                        --Tokenizer->At;

                        if(GetKeyword(Tokenizer, &Token)) {}
                        else if(GetCharacter(Tokenizer, &Token)) {}
                        else if(GetPreprocessorCommand(Tokenizer, &Token)) {}
                        else if(GetComment(Tokenizer, &Token)) {}
                        else
                        {
                                /*
                                  Put the Tokenizer forward again to consume
                                  the current input.  We don't know what the
                                  input was, but we need to move on.
                                */
                                ++Tokenizer->At;
                        }
                } break;
        }

        /* else if(GetFirstString(&FileContents, &match) && match.Data == FileContents.Data) */
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
        Tokenizer.Beginning = FileContents.Data;
        Tokenizer.At = FileContents.Data;

        bool Parsing = true;
        while(Parsing)
        {
                struct token Token = GetToken(&Tokenizer);
                switch(Token.Type)
                {
                        case Token_EndOfStream:         { Parsing = false; } break;

                        case Token_Keyword:             { printf("Keyword: "); } break;
                        case Token_Character:           { printf("Character: "); } break;
                        case Token_PreprocessorCommand: { printf("Preprocessor: "); } break;
                        case Token_Comment:             { printf("Comment: "); } break;

                        case Token_Asterisk:
                        case Token_Ampersand:
                        case Token_OpenParen:
                        case Token_CloseParen:
                        case Token_OpenBracket:
                        case Token_CloseBracket:
                        case Token_OpenBrace:
                        case Token_CloseBrace:
                        case Token_Colon:
                        case Token_SemiColon:
                        case Token_Comma:
                        case Token_Cross:
                        case Token_Dash:
                        case Token_Equal:
                        case Token_Slash:
                        case Token_Dot:
                        case Token_Arrow:               { printf("Symbol: "); } break;

                        case Token_Unknown:             {} break;

                        default:                        {} break;
                }

                if (Token.Type != Token_Unknown)
                {
                        printf("%.*s\n", (int)Token.TextLength, Token.Text);
                }
        }

        return(0);
}
