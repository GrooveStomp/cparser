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
  TokenIntegerConstant,
  TokenStringLiteral,
  TokenOperator,
  TokenSeparator,
  TokenWhitespace
} TokenTypes;

char *TokenNames[] = {"TokenNone", "TokenIdentifier", "TokenKeyword", "TokenSymbol", "TokenConstant",
                      "TokenIntegerConstant", "TokenStringLiteral", "TokenOperator", "TokenSeparator",
                      "TokenWhitespace"};

char *Symbols = "{}[],;-+=*^&%$?<>()!";

void integer_type() {
  /*
    Octal: 0[0-7]+
    Hex: 0X[0-9A-F]+/i # ignore case

    Any Integer can have suffix of [uU] or [lL] for unsigned and long, respectively.

    if unsuffixed and decimal, it has the first of these types in which it can be represented:
    [int, long int, unsigned long int]

    if it is unsuffxed octal or hexadecimal, it has the first possible of these types:
    [int, unsigned int, long int, unsigned long int]

    if it is suffixed by u or U then:
    [unsigned int, unsigned long int]

    if it is suffixed by l or L then:
    [long int, unsigned long int]
  */
}

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

bool is_integer_suffix(byte LastByte) {
  if (LastByte != 'u' && LastByte != 'U' && LastByte != 'l' && LastByte != 'L' && !is_digit(LastByte)) {
    return false;
  }
  return true;
}

bool is_octal_digit(byte Byte) {
  return (Byte >= '0' && Byte <= '7');
}

bool is_octal(byte ByteBuffer[], uint BufferSize) {
  byte FirstByte, LastByte, CurrentByte;
  int i;

  if (BufferSize < 3) { return false; }

  FirstByte = ByteBuffer[0];
  LastByte = ByteBuffer[BufferSize - 1];

  if (BufferSize < 2) { return false; }
  if (FirstByte != '0') { return false; }
  if (! (LastByte == 'u' || LastByte == 'U' || LastByte == 'l' || LastByte == 'L' || (LastByte >= '0' && LastByte <= '7')) ) {
    return false;
  }

  for (i = 1; i < BufferSize-1; ++i) {
    if (!is_octal_digit(ByteBuffer[i])) {
      return false;
    }
  }

  return true;
}

bool is_hex_digit(byte Byte) {
  return (is_digit(Byte) || (Byte >= 'a' && Byte <= 'f') || (Byte >= 'A' && Byte <= 'F'));
}

bool is_hexadecimal(byte ByteBuffer[], uint BufferSize) {
  byte FirstByte, SecondByte, LastByte, CurrentByte;
  int i;

  if (BufferSize < 2) { return false; }

  FirstByte = ByteBuffer[0];
  SecondByte = ByteBuffer[1];
  LastByte = ByteBuffer[BufferSize - 1];

  if (BufferSize < 2) { return false; }
  if (FirstByte != '0') { return false; }
  if (SecondByte != 'x' && SecondByte != 'X') { return false; }
  if (! (is_hex_digit(LastByte) || LastByte == 'u' || LastByte == 'U' || LastByte == 'l' || LastByte == 'L') ) {
    return false;
  }

  for (i = 2; i < BufferSize-1; ++i) {
    if (!is_hex_digit(ByteBuffer[i])) {
      return false;
    }
  }

  return true;
}

bool is_integer_constant(byte ByteBuffer[], uint BufferSize) {
  int i;
  byte LastByte;

  if (BufferSize < 1) { return false; }

  LastByte = ByteBuffer[BufferSize - 1];

  if (is_octal(ByteBuffer, BufferSize) || is_hexadecimal(ByteBuffer, BufferSize)) { return true; }

  if (BufferSize == 1 && ByteBuffer[0] == '0') { return true; }
  if (ByteBuffer[0] == '0') { return false; }

  for (i = 0; i < BufferSize - 1; ++i) {
    if (!is_digit(ByteBuffer[i])) {
      return false;
    }
  }

  return is_integer_suffix(LastByte);
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

void usage() {
  printf("Usage: ./a.out file\n");
  printf("  file: must be a file in this directory\n");
  printf("  Specify '-h' or '--help' instead of file for this help text\n");
  exit(0);
}

int main(int argc, char *argv[]) {
  if (argc != 2 || argv[1] == "-h" || argv[1] == "--help") {
    usage();
  }

  FILE *InFile = fopen(argv[1], "r");
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
      if (is_symbol(InputChar) || is_whitespace(InputChar)) {

        if (is_keyword(ByteBuffer, BufferTail)) {
          printf("<%s> %s\n", TokenNames[TokenKeyword], ByteBuffer);
        }
        else if (is_identifier(ByteBuffer, BufferTail)) {
          printf("<%s> %s\n", TokenNames[TokenIdentifier], ByteBuffer);
        }
        else if (is_integer_constant(ByteBuffer, BufferTail)) {
          printf("<%s> %s\n", TokenNames[TokenIntegerConstant], ByteBuffer);
        }
        else if (BufferTail > 0) {
          printf("<UnknownToken> %s\n", ByteBuffer);
        }

        if (is_symbol(InputChar)) {
          CurrentState.Whitespace = false;
          printf("<%s> %c\n", TokenNames[TokenSymbol], InputChar);
        }
        else if (is_whitespace(InputChar)) {
          if (!CurrentState.Whitespace) {
            /* printf("<%s>\n", TokenNames[TokenWhitespace]); */
            CurrentState.Whitespace = true;
          }
        }

        memset(ByteBuffer, '\0', BufferTail);
        BufferTail = 0;
        CurrentState.Word = false;
      }
      else if ((is_letter(InputChar) || is_digit(InputChar)) && BufferTail < 255) {
        CurrentState.Whitespace = false;
        ByteBuffer[BufferTail] = (char)InputChar;
        BufferTail = MIN(BufferTail + 1, 255);
      }
    }
  }

  fclose(InFile);
}
