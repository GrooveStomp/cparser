#ifndef _FILE_BUFFER_C
#define _FILE_BUFFER_C

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define min(a,b) ((a) < (b) ? (a) : (b))
#define bytes(n) (n)
#define kilobytes(n) (bytes(n) * 1000)

struct buffer {
  char *Data;
  size_t Capacity;
  size_t Size;
};

struct buffer *
BufferSet(struct buffer *Buff, char *BuffData, size_t BuffCapacity, size_t BuffSize)
{
  Buff->Data = BuffData;
  Buff->Capacity = BuffCapacity;
  Buff->Size = BuffSize;
  return Buff;
}

typedef enum {
  COPY_FILE_OK,
  COPY_FILE_DATA_REMAINING,
  COPY_FILE_INSUFFICIENT_SPACE,
  COPY_FILE_ERROR
} copy_file_result;

copy_file_result
CopyFileIntoBuffer(char *FileName, struct buffer *Mem)
{
  FILE *File = fopen(FileName, "r");
  if(File) {
    fseek(File, 0, SEEK_END);
    size_t FileSize = ftell(File);
    if(FileSize + 1 > Mem->Capacity) {
      return COPY_FILE_INSUFFICIENT_SPACE;
    }

    fseek(File, 0, SEEK_SET);
    fread(Mem->Data, 1, FileSize, File);
    Mem->Data[FileSize] = 0;
    Mem->Size = FileSize + 1;

    fclose(File);
  }

  return(COPY_FILE_OK);
}

/*
  Return the size in bytes required to hold the contents of the specified file,
  plus one trailing byte containing NULL.
*/
size_t
FileSize(char *FileName)
{
  FILE *File = fopen(FileName, "r");
  if(File) {
    fseek(File, 0, SEEK_END);
    size_t FileSize = ftell(File);
    fclose(File);
    return(FileSize + 1); /* +1 for trailing null byte. */
  }

  return(0);
}

#endif /* _FILE_BUFFER_C */
