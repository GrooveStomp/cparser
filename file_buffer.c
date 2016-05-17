#ifndef _FILE_BUFFER_C
#define _FILE_BUFFER_C

#include <stdio.h>

struct buffer {
  char *Data;
  size_t Capacity;
  size_t Length;
};

struct buffer *
BufferSet(struct buffer *Buffer, char *Data, size_t Length, size_t Capacity)
{
  Buffer->Data = Data;
  Buffer->Capacity = Capacity;
  Buffer->Length = Length;
  return Buffer;
}

enum copy_file_result
{
  COPY_FILE_OK,
  COPY_FILE_DATA_REMAINING,
  COPY_FILE_INSUFFICIENT_SPACE,
  COPY_FILE_ERROR
};

enum copy_file_result
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
    Mem->Length = FileSize;

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
