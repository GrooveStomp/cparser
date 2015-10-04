#include <stdio.h>
#include <stdlib.h>

typedef char byte;
typedef int bool;
typedef int word;
typedef unsigned int uint;
#define false 0
#define true !false
#define MAX(a, b) ((a) >= (b) ? (a) : (b))
#define MIN(a, b) ((a) <= (b) ? (a) : (b))

/*
 * Comments: See this block. Should nest!
 */

// TODO(AARON): Look up full list of symbols.
char * symbols = "{}[],;-+=*^&%$?<>";

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

int main() {
  FILE *InFile = fopen("input.c", "r");
  byte ByteBuffer[256] = {0};
  uint BufferTail = 0;
  word InputChar;
  bool InWord = false;

  /*
    TODO(AARON): New tokenizer algorithm:
    ----
    Read char into InputChar from InFile
    if InputChar is letter or digit
      Set ByteBuffer @ BufferTail to InputChar
      Increment BufferTail (ensuring it never exceeds 255)
    otherwise if InputChar is whitespace
      Set ByteBuffer @ BufferTail to '\0'
      Increment BufferTail
      print value of ByteBuffer (BufferTail is now the size of the word)
      Reset BufferTail to 0
    end
  */

  while (1) {
    if (EOF == (InputChar = fgetc(InFile)))
    {
      if (ferror(InFile)) {
        perror("Error!");
        fclose(InFile);
        exit(1);
      } else {
        printf("Done reading file\n");
        if (InWord)
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
      if (is_letter(InputChar) && !InWord) { InWord = true; }

      if ((is_letter(InputChar) || is_digit(InputChar)) && InWord && BufferTail < 255)
      {
        ByteBuffer[BufferTail] = (char)InputChar;
        BufferTail = MIN(BufferTail + 1, 255);
      }
      else if (is_whitespace(InputChar) && InWord)
      {
        InWord = false;
        ByteBuffer[BufferTail] = '\0';
        BufferTail++;
        printf("Token read: %s\n", ByteBuffer);
        BufferTail = 0;
      }
      else
      {
        printf("Read input: %c\n", InputChar);
      }
    }
  }

  fclose(InFile);
}
