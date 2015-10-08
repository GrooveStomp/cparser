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
  TokenCharacterConstant,
  TokenOperator,
  TokenSeparator,
  TokenWhitespace
} TokenTypes;

char *TokenNames[] = {"None", "Identifier", "Keyword", "Symbol", "Constant", "IntegerConstant",
                      "StringLiteral", "CharacterConstant", "Operator", "Separator", "Whitespace"};

char *Symbols = "#{}[],;-+=*^&%$?<>()!/|~.'";

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

void print_character(byte ByteBuffer[], int Size) {
  byte Char;
  if (Size == 1) {
    Char = ByteBuffer[0];
    if (Char >= 32 && Char <= 126) {
      printf("<%s> '%c'\n", TokenNames[TokenCharacterConstant], Char);
      return;
    }
    else {
      printf("<%s> DEC: '%d'\n", TokenNames[TokenCharacterConstant], Char);
      return;
    }
  }
  else if (Size == 2 && ByteBuffer[0] == '\\') {
    Char = ByteBuffer[1];
    if (Char >= 32 && Char <= 126) {
      printf("<%s> '\\%c'\n", TokenNames[TokenCharacterConstant], Char);
      return;
    }
    else {
      printf("<%s> DEC: '\\%d'\n", TokenNames[TokenCharacterConstant], Char);
      return;
    }
  }
  printf("<UnknownToken> '%s'\n", ByteBuffer);
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
  return true;
}

bool is_float_constant() {
}

bool is_enumeration_constant() {
}

bool is_string_literal() {
  return true;
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

void render_buffer_token(byte ByteBuffer[], uint BufferSize);
void flush_buffer(byte ByteBuffer[], uint *BufferSize);

enum {
  StatusNormal,
  StatusLineComment,
  StatusBlockComment,
  StatusWhitespace,
  StatusStringLiteral,
  StatusCharacterConstant,
};
int CurrentState = StatusNormal;

void usage() {
  printf("Usage: ./a.out file\n");
  printf("  file: must be a file in this directory\n");
  printf("  Specify '-h' or '--help' instead of file for this help text\n");
  exit(0);
}

local_persist int BlockComment = 0;

void increment_block_comment() {
  ++BlockComment;
  CurrentState = StatusBlockComment;
}

void decrement_block_comment() {
  --BlockComment;
  if (!BlockComment && CurrentState == StatusBlockComment) {
    CurrentState = StatusNormal;
  }
}

bool in_comment() {
  return (CurrentState == StatusLineComment || CurrentState == StatusBlockComment);
}

int main(int argc, char *argv[]) {
  if (argc != 2 || argv[1] == "-h" || argv[1] == "--help") { usage(); }

  FILE *InFile = fopen(argv[1], "r");
  byte ByteBuffer[256] = {0};
  word InputChar = 0;
  word LastChar = 0;
  int CurrentToken = TokenNone;
  int BlockComment = 0; // Increment on entry, decrement on exit.
  uint BufferTail, i; BufferTail = 0;

  while (1) {
    LastChar = InputChar;
    if (EOF == (InputChar = fgetc(InFile))) {
      if (ferror(InFile)) {
        perror("Error!");
        fclose(InFile);
        exit(1);
      } else {
        printf("Done reading file\n");
        if (BufferTail > 0) {
          ByteBuffer[BufferTail] = '\0';
          BufferTail++;
          printf("Last partial token: %s\n", ByteBuffer);
        }
        break;
      }
    }
    else {

      if (InputChar == '\'' && CurrentState != StatusCharacterConstant && !in_comment() && CurrentState != StatusStringLiteral) {
        if (BufferTail != 0) {
          ungetc(InputChar, InFile);
          InputChar = ' ';
        }
        else {
          CurrentState = StatusCharacterConstant;
          continue;
        }
      }
      else if (InputChar == '\'' && CurrentState == StatusCharacterConstant && LastChar != '\\') {
        if (is_character_constant(ByteBuffer, BufferTail)) {
          print_character(ByteBuffer, BufferTail);
        }
        else {
          printf("<UnknownToken> %s\n", ByteBuffer);
        }
        flush_buffer(ByteBuffer, &BufferTail);
        CurrentState = StatusNormal;
        continue;
      }

      if ((InputChar == '"') && CurrentState != StatusStringLiteral && !in_comment() && CurrentState != StatusCharacterConstant) {
        CurrentState = StatusStringLiteral;
        continue;
      }
      else if ((InputChar == '"') && CurrentState == StatusStringLiteral && LastChar != '\\') {
        CurrentState = StatusNormal;
        if (is_string_literal(ByteBuffer, BufferTail)) {
          printf("<%s> \"%s\"\n", TokenNames[TokenStringLiteral], ByteBuffer);
          flush_buffer(ByteBuffer, &BufferTail);
          continue;
        }
      }

      if (InputChar == '\n' && CurrentState == StatusLineComment) { CurrentState = StatusNormal; }
      if (InputChar == '/' && CurrentState != StatusLineComment) {
        if (LastChar == '/' && CurrentState != StatusBlockComment) {
          CurrentState = StatusLineComment;
          InputChar = '\0';
          continue;
        }
        else if (LastChar == '*') {
          decrement_block_comment();
          InputChar = '\0';
          continue;
        }
        else if (CurrentState != StatusBlockComment) {
          continue;
        }
      }

      if (LastChar == '/' && CurrentState != StatusLineComment) {
        if (InputChar == '*') {
          increment_block_comment();
          continue;
        }
        else if (CurrentState != StatusBlockComment) {
          ungetc(InputChar, InFile);
          InputChar = LastChar;
          LastChar = '\0';
        }
      }

      if ((is_symbol(InputChar) || is_whitespace(InputChar)) && !in_comment() && CurrentState != StatusStringLiteral && CurrentState != StatusCharacterConstant) {
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
          if (CurrentState == StatusWhitespace) { CurrentState = StatusNormal; }
          printf("<%s> %c\n", TokenNames[TokenSymbol], InputChar);
        }
        else if (is_whitespace(InputChar)) {
          if (InputChar == '\n' && CurrentState == StatusLineComment) {
            CurrentState = StatusNormal;
          }
          if (CurrentState != StatusWhitespace) {
            CurrentState = StatusWhitespace;
          }
        }

        flush_buffer(ByteBuffer, &BufferTail);
        if (InputChar == '/') { InputChar = '\0'; } // NOTE(AARON): To prevent '/' from being handled next go.
      }
      else if (InputChar == '\n' && CurrentState == StatusLineComment) {
        CurrentState = StatusNormal;
      }
      else if ((is_letter(InputChar) || is_digit(InputChar)) && BufferTail < 255 && !in_comment()) {
        if (CurrentState == StatusWhitespace) { CurrentState = StatusNormal; }
        ByteBuffer[BufferTail] = (char)InputChar;
        BufferTail = MIN(BufferTail + 1, 255);
      }
      else if (CurrentState == StatusStringLiteral || CurrentState == StatusCharacterConstant) {
        ByteBuffer[BufferTail] = (char)InputChar;
        BufferTail = MIN(BufferTail + 1, 255);
        if ((InputChar == '\\' && LastChar == '\\') || (InputChar != '\\')) {
          InputChar = '\0';
        }
      }
    }
  }

  fclose(InFile);
}

void flush_buffer(byte ByteBuffer[], uint *BufferSize) {
  memset(ByteBuffer, '\0', *BufferSize);
  *BufferSize = 0;
}
