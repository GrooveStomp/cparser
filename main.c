#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef char byte;
typedef int bool;
typedef int word;
typedef unsigned int uint;
#define global_variable static
#define local_persist static
#define false 0
#define true ~false
#define MAX(a, b) ((a) >= (b) ? (a) : (b))
#define MIN(a, b) ((a) <= (b) ? (a) : (b))
#define ARRAY_SIZE(Array) (sizeof((Array)) / sizeof((Array)[0]))

/*
 * Comments: See this block. Should nest!
 */

char *Keywords[] = {"auto", "break", "case", "char", "const", "continue", "default", "do", "double", "else",
                    "enum", "extern", "float", "for", "goto", "if", "int", "long", "register", "return",
                    "short", "signed", "sizeof", "static", "struct", "switch", "typedef", "union",
                    "unsigned", "void", "volatile", "while"};

enum {
  TokenNone,
  TokenIdentifier,
  TokenKeyword,
  TokenSymbol,
  TokenConstant,
  TokenStringLiteral,
  TokenOperator,
  TokenSeparator,
  TokenWhitespace
} TokenTypes;

char *TokenNames[] = {"TokenNone", "TokenIdentifier", "TokenKeyword", "TokenSymbol", "TokenConstant",
                      "TokenStringLiteral", "TokenOperator", "TokenSeparator", "TokenWhitespace"};

// TODO(AARON): Look up full list of symbols.
char *Symbols = "{}[],;-+=*^&%$?<>()!";

bool is_whitespace(byte Byte) {
  // TODO(AARON): Look up full list of whitespace characters.
  return (Byte == ' ' || Byte == '\t' || Byte == '\n' || Byte == '\r');
}

bool is_letter(byte Byte) {
  return (Byte >= 'A' && Byte <= 'Z') || (Byte >= 'a' && Byte <= 'z') || (Byte == '_');
}

bool is_digit(byte Byte) {
  return (Byte >= '0' && Byte <= '9');
}

void check_symbol(word InputChar);

struct {
  uint Word : 1;
  uint Whitespace : 1;
} CurrentState;

int main() {
  FILE *InFile = fopen("main.c", "r");
  byte ByteBuffer[256] = {0};
  word InputChar;
  int CurrentToken = TokenNone;
  uint BufferTail, i; BufferTail = 0;

  while (1) {
    if (EOF == (InputChar = fgetc(InFile)))
    {
      if (ferror(InFile)) {
        perror("Error!");
        fclose(InFile);
        exit(1);
      } else {
        printf("Done reading file\n");
        if (CurrentState.Word)
        {
          ByteBuffer[BufferTail] = '\0';
          BufferTail++;
          printf("Last partial token: %s\n", ByteBuffer);
        }
        break;
      }
    }
    else
    {
      if (is_letter(InputChar) && !CurrentState.Word) { CurrentState.Word = true; CurrentState.Whitespace = false; }

      if ((is_letter(InputChar) || is_digit(InputChar)) && CurrentState.Word && BufferTail < 255)
      {
        ByteBuffer[BufferTail] = (char)InputChar;
        BufferTail = MIN(BufferTail + 1, 255);
      }
      else if (!is_letter(InputChar) && !is_digit(InputChar) && CurrentState.Word)
      {
        CurrentState.Word = false;
        ByteBuffer[BufferTail] = '\0';
        BufferTail++;

        CurrentToken = TokenIdentifier;

        // Check if this token is a keyword
        for (i=0; i < ARRAY_SIZE(Keywords); ++i) {
          if (0 == strncmp(ByteBuffer, Keywords[i], BufferTail)) {
            CurrentToken = TokenKeyword;
            break;
          }
        }

        printf("<%s> %s\n", TokenNames[CurrentToken], ByteBuffer);
        BufferTail = 0;

        check_symbol(InputChar);
        if (is_whitespace(InputChar)) { CurrentState.Whitespace = true; }
      }
      else
      {
        check_symbol(InputChar);
      }
    }
  }

  fclose(InFile);
}

void check_symbol(word InputChar) {
  int CurrentToken, i; CurrentToken = TokenNone;

  // Check if this token is a symbol
  for (i = 0; i < strlen(Symbols); ++i) {
    if (InputChar == Symbols[i]) {
      CurrentToken = TokenSymbol;
      break;
    }
  }

  // Check if this token is whitespace
  if (is_whitespace(InputChar)) {
    CurrentToken = TokenWhitespace;
    CurrentState.Word = false;
  }

  if (CurrentToken) {
    if (CurrentToken == TokenWhitespace) {
      if (!CurrentState.Whitespace) {
        CurrentState.Whitespace = true;
        printf("<%s>\n", TokenNames[CurrentToken]);
      }
    }
    else {
      printf("<%s> %c\n", TokenNames[CurrentToken], InputChar);
    }
  }
  else {
    printf("%c\n", InputChar);
  }
}
