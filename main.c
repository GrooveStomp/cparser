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

//------------------------------------------------------------------------------------------------------------
// Global State
//------------------------------------------------------------------------------------------------------------

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

char *Symbols = "{}[],;-+=*^&%$?<>()!";

//------------------------------------------------------------------------------------------------------------
// Identity Tests
//------------------------------------------------------------------------------------------------------------

bool is_whitespace(byte Byte) {
  return (Byte == ' ' || Byte == '\t' || Byte == '\n' || Byte == '\r');
}

bool is_letter(byte Byte) {
  return (Byte >= 'A' && Byte <= 'Z') || (Byte >= 'a' && Byte <= 'z') || (Byte == '_');
}

bool is_digit(byte Byte) {
  return (Byte >= '0' && Byte <= '9');
}

bool is_integer_constant() {
}

bool is_character_constant() {
}

bool is_float_constant() {
}

bool is_enumeration_constant() {
}

bool is_string_literal() {
}

bool is_symbol(word Character) {
  int i;
  for (i = 0; i < strlen(Symbols); ++i) {
    if (Character == (word)Symbols[i]) {
      return true;
    }
  }
  return false;
}

bool is_keyword(byte ByteBuffer[], uint BufferSize) {
  int i;

  if (BufferSize < 2) { return false; }

  for (i=0; i < ARRAY_SIZE(Keywords); ++i) {
    if (0 == strncmp(ByteBuffer, Keywords[i], BufferSize)) {
      return true;
    }
  }
  return false;
}

bool is_identifier(byte ByteBuffer[], uint BufferSize) {
  int i;

  if (is_keyword(ByteBuffer, BufferSize)) { return false; }

  if (!is_letter(ByteBuffer[0])) { return false; }

  for (i = 1; i < BufferSize; ++i) {
    if (!(is_letter(ByteBuffer[i]) || is_digit(ByteBuffer[i]))) {
      return false;
    }
  }

  return true;
}

//------------------------------------------------------------------------------------------------------------
// Main
//------------------------------------------------------------------------------------------------------------

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
    if (EOF == (InputChar = fgetc(InFile))) {
      if (ferror(InFile)) {
        perror("Error!");
        fclose(InFile);
        exit(1);
      } else {
        printf("Done reading file\n");
        if (CurrentState.Word) {
          ByteBuffer[BufferTail] = '\0';
          BufferTail++;
          printf("Last partial token: %s\n", ByteBuffer);
        }
        break;
      }
    }
    else {

      if (is_letter(InputChar) && !CurrentState.Word) {
        CurrentState.Word = true;
        CurrentState.Whitespace = false;
      }

      if (is_symbol(InputChar) || is_whitespace(InputChar)) {

        if (is_keyword(ByteBuffer, BufferTail)) {
          printf("<%s> %s\n", TokenNames[TokenKeyword], ByteBuffer);
        }
        else if (is_identifier(ByteBuffer, BufferTail)) {
          printf("<%s> %s\n", TokenNames[TokenIdentifier], ByteBuffer);
        }

        if (is_symbol(InputChar)) {
          printf("<%s> %c\n", TokenNames[TokenSymbol], InputChar);
        }
        else if (is_whitespace(InputChar)) {
          if (!CurrentState.Whitespace) {
            printf("<%s> %c\n", TokenNames[TokenWhitespace], InputChar);
            CurrentState.Whitespace = true;
          }
        }

        memset(ByteBuffer, '\0', BufferTail);
        BufferTail = 0;
        CurrentState.Word = false;
      }
      else if ((is_letter(InputChar) || is_digit(InputChar)) && CurrentState.Word && BufferTail < 255) {
        ByteBuffer[BufferTail] = (char)InputChar;
        BufferTail = MIN(BufferTail + 1, 255);
      }
    }
  }

  fclose(InFile);
}
