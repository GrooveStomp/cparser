/******************************************************************************
 * File: gs.h
 * Created: 2016-07-14
 * Updated: 2016-11-03
 * Package: gslibc
 * Creator: Aaron Oman (GrooveStomp)
 * Copyright 2016 - 2020, Aaron Oman and the gslibc contributors
 * SPDX-License-Identifier: LGPL-3.0-only
 *-----------------------------------------------------------------------------
 *
 * Standard library for personal use. Heavily influenced by Sean Barrett's stb.
 *
 ******************************************************************************/
#ifndef GS_H
#define GS_H
#define GS_VERSION 0.2.0-dev

#define GSArraySize(Array) (sizeof((Array)) / sizeof((Array)[0]))

/******************************************************************************
 * Usage:
 *
 * int Numbers[] = { 1, 2, 3, 4, 5 };
 * GSArrayForEach(int *Number, Numbers) {
 *         printf("Number[%i]: %i\n", Index, *Number);
 * }
 *
 * NOTE:
 * The variable `Index' is automatically generated for you.
 * `Item' must be a pointer to the type of variable used in the Array.
 *
 * Implementation taken from: http://stackoverflow.com/a/400970
 ******************************************************************************/
#define GSArrayForEach(Item, Array) \
        for (int Keep##__LINE__ = 1, \
                Count##__LINE__ = 0, \
                Index = 0, \
                Size##__LINE__ = sizeof((Array)) / sizeof(*(Array)); \
            Keep##__LINE__ && Count##__LINE__ != Size##__LINE__; \
            Keep##__LINE__ = !Keep##__LINE__, Count##__LINE__++) \
                for (Item = (Array) + Count##__LINE__; Keep##__LINE__; Keep##__LINE__ = !Keep##__LINE__, Index++)

#define GSMax(A, B) ((A) < (B) ? (B) : (A))
#define GSMin(A, B) ((A) < (B) ? (A) : (B))

// TODO: Move to platform-specific header
/* #define GSAbortWithMessage(...) \ */
/*         { \ */
/*                 char String##__LINE__[256];                             \ */
/*                 sprintf(String##__LINE__, "In %s() at line #%i: ", __func__, __LINE__); \ */
/*                 fprintf(stderr, String##__LINE__);                       \ */
/*                 fprintf(stderr, __VA_ARGS__); \ */
/*                 exit(EXIT_FAILURE); \ */
/*         } */

// TODO: Move to platform-specific header
/* #define GSLog(...) \ */
/*         { \ */
/*                 char String##__LINE__[256];                             \ */
/*                 sprintf(String##__LINE__, "In %s() at line #%i: ", __func__, __LINE__); \ */
/*                 fprintf(stdout, String##__LINE__);                       \ */
/*                 fprintf(stdout, __VA_ARGS__); \ */
/*         } */

#define GS1024Inverse 1.0/1024
#define GSBytesToKilobytes(X) (X) * GS1024Inverse
#define GSBytesToMegabytes(X) GSBytesToKilobytes((X)) * GS1024Inverse
#define GSBytesToGigabytes(X) GSBytesToMegabytes((X)) * GS1024Inverse

/******************************************************************************
 * Primitive Type Definitions
 * TODO: Conditionally do typedefs?
 ******************************************************************************/

#define GSNullChar '\0'

#ifndef NULL
#define NULL 0
#endif

#define GSNullPtr NULL

typedef int bool;
#ifndef false
#define false 0
#define true !false
#endif

typedef char i8;
typedef unsigned char u8;
typedef short i16;
typedef unsigned short u16;
typedef int i32;
typedef unsigned int u32;
typedef long i64;
typedef unsigned long u64;
typedef long long i128;
typedef unsigned long long u128;

typedef float f32;
typedef double f64;
typedef long double f128;

/******************************************************************************
 * Allocator
 ******************************************************************************/

typedef struct gs_allocator {
        void *(*Alloc)(u64);
        void (*Free)(void *);
        void *(*Realloc)(void *, u64);
        void *(*ArrayAlloc)(u64, u64);
} gs_allocator;

/******************************************************************************
 * Character Definitions
 *-----------------------------------------------------------------------------
 * Functions to interact with C's basic ASCII char type.
 ******************************************************************************/

bool GSCharIsEndOfStream(char c) {
	return c == '\0';
}

bool GSCharIsEndOfLine(char c) {
	return (c == '\n') || (c == '\r');
}

bool GSCharIsWhitespace(char c) {
	return (c == ' ') ||
	       (c == '\t') ||
	       (c == '\v') ||
	       (c == '\f') ||
	       GSCharIsEndOfLine(c);
}

bool GSCharIsOctal(char c) {
	return (c >= '0' && c <= '7');
}

bool GSCharIsDecimal(char c) {
	return (c >= '0' && c <= '9');
}

bool GSCharIsHexadecimal(char c) {
	return ((c >= '0' && c <= '9') ||
                (c >= 'a' && c <= 'f') ||
                (c >= 'A' && c <= 'F'));
}

bool GSCharIsAlphabetical(char c) {
	return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

bool GSCharIsAlphanumeric(char c) {
        return GSCharIsAlphabetical(c) || GSCharIsDecimal(c);
}

bool GSCharIsUpcase(char c) {
        return GSCharIsAlphabetical(c) && (c >= 'A') && (c <= 'z');
}

char GSCharUpcase(char c) {
        char Result = c;

        if (GSCharIsAlphabetical(c) && (c >= 'a' && c <= 'z')) {
                int Delta = c - 'a';
                Result = Delta + 'A';
        }

        return Result;
}

bool GSCharIsDowncase(char c) {
        return GSCharIsAlphabetical(c) && (c >= 'a') && (c <= 'z');
}

char GSCharDowncase(char c) {
        char Result = c;

        if (GSCharIsAlphabetical(c) && (c >= 'A' && c <= 'Z')) {
                int Delta = c - 'A';
                Result = Delta + 'a';
        }

        return Result;
}

/******************************************************************************
 * String Definitions
 *-----------------------------------------------------------------------------
 * C string type. That is, ASCII characters with terminating NULL.
 ******************************************************************************/

bool GSStringIsEqual(char *LeftString, char *RightString, int MaxNumToMatch) {
	int NumMatched = 0;

        if (*LeftString == GSNullChar ||
           *RightString == GSNullChar &&
           *LeftString != *RightString)
        {
                return false;
        }

	while (NumMatched < MaxNumToMatch) {
		if (*LeftString == *RightString) {
			LeftString++;
			RightString++;
			NumMatched++;
		} else {
			return false;
		}
	}

	return true;
}

u32 GSStringLength(char *String) {
	char *c = String;
	while (*c != '\0') c++;
	return c - String;
}

bool GSStringCopy(char *Source, char *Dest, int Max) {
        if (Source == NULL || Dest == NULL) {
                return false;
        }

        int i = 0;
        for (; Source[i] != '\0' && i < Max; i++) {
                Dest[i] = Source[i];
        }
        Dest[i] = '\0';

        return true;
}

bool GSStringCopyNoNull(char *Source, char *Dest, int Max) {
        if (Source == NULL || Dest == NULL) {
                return false;
        }

        for (int i = 0; Source[i] != '\0' && i < Max; i++) {
                Dest[i] = Source[i];
        }

        return true;
}

/* NOTE: Assumes a maximum string length of 512 bytes. */
/* Returns number of bytes copied. */
u32 GSStringTrimWhitespace(char *Source, u32 MaxLength) {
        char Dest[512];
        MaxLength = GSMin(512, MaxLength);

        int FirstChar, LastChar;
        for (FirstChar = 0; GSCharIsWhitespace(Source[FirstChar]); FirstChar++);

        int StringLength = GSStringLength(Source);
        for (LastChar = StringLength - 1; GSCharIsWhitespace(Source[LastChar]); LastChar--);

        int Count = 0;
        for (int i = FirstChar; i <= LastChar && Count < MaxLength; Count++, i++) {
                Dest[Count] = Source[i];
        }

        for (int i = 0; i < Count; i++) {
                Source[i] = Dest[i];
        }
        Source[Count] = GSNullChar;

        return Count;
}

/*
  For any ascii character following an underscore, remove the underscore
  and capitalize the ascii char.
  This function assumes a maximum string size of 512 bytes.
  The first character is capitalized.
*/
u32 GSStringSnakeCaseToCamelCase(char *Source, u32 SourceLength) {
        char Dest[512]; /* Scratch buffer. */
        int Si = 0, Di = 0; /* Iterable indices for Source and Dest. */

        if ((Source[Si] == '_') &&
           (Si+1 < SourceLength) &&
           GSCharIsAlphabetical(Source[Si+1])) {
                Si++;
        }
        Dest[Di] = GSCharUpcase(Source[Si]);
        Si++;
        Di++;

        SourceLength = GSMin(512, SourceLength);

        for (Si, Di; Si<SourceLength; Si++, Di++) {
                /* Replace any '_*' with 'upcase(*)' where * is an ascii char. */
                if ((Source[Si] == '_') &&
                   (Si+1 < SourceLength) &&
                   GSCharIsAlphabetical(Source[Si+1])) {
                        Dest[Di] = GSCharUpcase(Source[Si+1]);
                        Si++;
                }
                /* Copy chars normally. */
                else {
                        Dest[Di] = Source[Si];
                }
        }

        /* Write the modified string back to source. */
        for (int i = 0; i < Di; i++) {
                Source[i] = Dest[i];
        }
        Source[Di] = GSNullChar;

        return Di;
}

/*
  Prerequisites:
  - Dest must be large enough to contain the modified string.

  For any Capitalized ascii character, replace with an underscore followed by
  the lowercase version of that character. This does not apply to leading char.
  eg.: CamelCase -> Camel_case
*/
u32 GSStringCamelCaseToSnakeCase(char *Source, char *Dest, u32 SourceLength) {
        int i = 0, j = 0; /* Iterable indices for Source and Dest. */
        Dest[i] = GSCharDowncase(Source[i]);
        i++;
        j++;

        for (i, j; i<SourceLength && Source[i] != GSNullChar; i++, j++) {
                /* Replace upcase ascii char with '_' and downcase ascii char. */
                if (GSCharIsUpcase(Source[i])) {
                        Dest[j] = '_';
                        j++;
                        Dest[j] = GSCharDowncase(Source[i]);
                }
                /* Copy chars normally. */
                else {
                        Dest[j] = Source[i];
                }
        }
        Dest[j] = GSNullChar;

        return j;
}

/*
  Capitalizes the first character found.
  Modifies Source in-place.
  Returns Source.
  eg.: hello -> Hello
       123foos -> 123Foos
*/
char *GSStringCapitalize(char *Source, u32 Length) {
        int i = 0;

        while (true) {
                if (i >= Length)
                        break;
                if (Source[i] == GSNullChar)
                        break;
                if (GSCharIsAlphabetical(Source[i]))
                        break;
                i++;
        }

        if (i >= Length)
                return Source;

        Source[i] = GSCharUpcase(Source[i]);

        return Source;
}

typedef bool (*GSStringFilterFn)(char C);

/* Returns length of new string */
int GSStringKeep(char *Source, char *Dest, u32 MaxLength, GSStringFilterFn FilterFn) {
        int i = 0;
        int j = 0;

	while (i < MaxLength) {
                if (FilterFn(Source[i])) {
                        Dest[j] = Source[i];
                        j++;
                }
                i++;
        }
        Dest[j] = GSNullChar;

        return j + 1;
}

/* Returns length of new string */
int GSStringReject(char *Source, char *Dest, u32 MaxLength, GSStringFilterFn FilterFn) {
        int i = 0;
        int j = 0;

	while (i < MaxLength) {
                if (!FilterFn(Source[i])) {
                        Dest[j] = Source[i];
                        j++;
                }
                i++;
        }
        Dest[j] = GSNullChar;

        return j + 1;
}

/******************************************************************************
 * Hash Map
 *-----------------------------------------------------------------------------
 *
 * Usage:
 *     char *Value = "value";
 *     int StringLength = 256;
 *     int NumElements = 13;
 *     u32 BytesRequired = GSHashMapBytesRequired(StringLength, NumElements);
 *     gs_hash_map *Map = GSHashMapInit(alloca(BytesRequired), StringLength, NumElements);
 *     GSHashMapSet(Map, "key", Value);
 *     if (GSHashMapHasKey(Map, "key")) {
 *         char *Result = (char *)GSHashMapGet(Map, "key");
 *         printf("Key(%s), Value(%s)\n", "key", Result);
 *     }
 ******************************************************************************/

typedef struct gs_hash_map {
        u32 Count;
        u32 AllocatedBytes;
        u32 Capacity;
        u32 MaxKeyLength;

        char *Keys;
        void **Values;
} gs_hash_map;

/* String must be a NULL-terminated string */
u32 __GSHashMapComputeHash(gs_hash_map *Self, char *String) {
        /*
          sdbm hash function: http://stackoverflow.com/a/14409947
        */
        u32 HashAddress = 0;
        for (u32 i = 0; String[i] != GSNullChar; i++) {
                HashAddress = String[i] +
                        (HashAddress << 6) +
                        (HashAddress << 16) -
                        HashAddress;
        }
        u32 Result = HashAddress % Self->Capacity;
        return Result;
}

u32 GSHashMapBytesRequired(u32 MaxKeyLength, u32 NumEntries) {
        int AllocSize =
                sizeof(gs_hash_map) +
                (sizeof(char) * MaxKeyLength * NumEntries) +
                (sizeof(void *) * NumEntries);

        return AllocSize;
}

gs_hash_map *GSHashMapInit(void *Memory, u32 MaxKeyLength, u32 NumEntries) {
        gs_hash_map *Self = (gs_hash_map *)Memory;

        char *KeyValueMemory = (char *)Memory + sizeof(gs_hash_map);

        Self->MaxKeyLength = MaxKeyLength;
        Self->Capacity = NumEntries;
        Self->AllocatedBytes = GSHashMapBytesRequired(MaxKeyLength, NumEntries);
        Self->Count = 0;

        int KeysMemLength = MaxKeyLength * NumEntries;

        Self->Keys = KeyValueMemory;
        for (int i = 0; i < KeysMemLength; i++) {
                Self->Keys[i] = 0;
        }

        Self->Values = (void **)(Self->Keys + KeysMemLength);
        for (int i = 0; i < NumEntries; i++) {
                Self->Values[i] = 0;
        }

        return Self;
}

bool __GSHashMapUpdate(gs_hash_map *Self, char *Key, void *Value) {
        u32 KeyLength = GSStringLength(Key);
        u32 HashIndex = __GSHashMapComputeHash(Self, Key);

        u32 StartHash = HashIndex;

        do {
                if (GSStringIsEqual(&Self->Keys[HashIndex * Self->MaxKeyLength],
                                   Key,
                                   GSStringLength(Key))) {
                        Self->Values[HashIndex] = Value;
                        return true;
                }
                HashIndex = (HashIndex + 1) % Self->Capacity;
        } while (HashIndex != StartHash);

        /* Couldn't find Key to update. */
        return false;
}

/* Wanted must be a NULL terminated string */
bool GSHashMapHasKey(gs_hash_map *Self, char *Wanted) {
        u32 HashIndex = __GSHashMapComputeHash(Self, Wanted);
        char *Key = &Self->Keys[HashIndex * Self->MaxKeyLength];
        if (GSStringIsEqual(Wanted, Key, GSStringLength(Wanted))) {
                return true;
        }

        u32 StartHash = HashIndex;
        HashIndex = (HashIndex + 1) % Self->Capacity;

        while (true) {
                if (HashIndex == StartHash) break;

                Key = &Self->Keys[HashIndex * Self->MaxKeyLength];
                if (GSStringIsEqual(Wanted, Key, GSStringLength(Wanted))) {
                        return true;
                }
                HashIndex = (HashIndex + 1) % Self->Capacity;
        }

        return false;
}

/*
  Input: Key as string
  Computation: Hash key value into an integer.
  Algorithm: Open-addressing hash. Easy to predict space usage.
             See: https://en.wikipedia.org/wiki/Open_addressing
  Key must be a NULL terminated string.
 */
bool GSHashMapSet(gs_hash_map *Self, char *Key, void *Value) {
        u32 KeyLength = GSStringLength(Key);
        u32 HashIndex = __GSHashMapComputeHash(Self, Key);

        if (GSHashMapHasKey(Self, Key)) {
                return __GSHashMapUpdate(Self, Key, Value);
        }

        /* We're not updating, so return false if we're at capacity. */
        if (Self->Count >= Self->Capacity) return false;

        /* Add a brand-new key in. */
        if (Self->Keys[HashIndex * Self->MaxKeyLength] == GSNullChar) {
                GSStringCopy(Key, &Self->Keys[HashIndex * Self->MaxKeyLength], KeyLength);
                Self->Values[HashIndex] = Value;
                Self->Count++;
                return true;
        }

        /* We have a collision! Find a free index. */
        u32 StartHash = HashIndex;
        HashIndex = (HashIndex + 1) % Self->Capacity;

        while (true) {
                if (HashIndex == StartHash) break;

                if (Self->Keys[HashIndex * Self->MaxKeyLength] == GSNullChar) {
                        GSStringCopy(Key, &Self->Keys[HashIndex * Self->MaxKeyLength], KeyLength);
                        Self->Values[HashIndex] = Value;
                        Self->Count++;
                        return true;
                }
                HashIndex = (HashIndex + 1) % Self->Capacity;
        }

        /* Couldn't find any free space. */
        return false;
}

/* Memory must be large enough for the resized Hash. Memory _cannot_ overlap! */
bool GSHashMapGrow(gs_hash_map **Self, u32 NumEntries, void *New) {
        gs_hash_map *Old = *Self;

        /* No point in making smaller... */
        if (NumEntries <= Old->Capacity) return false;
        if (New == NULL) return false;

        *Self = GSHashMapInit(New, Old->MaxKeyLength, NumEntries);
        for (int i = 0; i < Old->Capacity; i++) {
                char *Key = &Old->Keys[i * Old->MaxKeyLength];
                char *Value = (char *)(Old->Values[i]);
                if (Key != NULL) {
                        bool Success = GSHashMapSet(*Self, Key, Value);
                        if (!Success) return false;
                }
        }

        return true;
}

/* Wanted must be a NULL terminated string */
void *GSHashMapGet(gs_hash_map *Self, char *Wanted) {
        u32 HashIndex = __GSHashMapComputeHash(Self, Wanted);
        char *Key = &Self->Keys[HashIndex * Self->MaxKeyLength];
        if (GSStringIsEqual(Wanted, Key, GSStringLength(Key))) {
                void *Result = Self->Values[HashIndex];
                return Result;
        }

        u32 StartHash = HashIndex;
        HashIndex = (HashIndex + 1) % Self->Capacity;

        while (true) {
                if (HashIndex == StartHash) break;

                Key = &Self->Keys[HashIndex * Self->MaxKeyLength];
                if (GSStringIsEqual(Wanted, Key, GSStringLength(Key))) {
                        void *Result = Self->Values[HashIndex];
                        return Result;
                }
                HashIndex = (HashIndex + 1) % Self->Capacity;
        }

        return NULL;
}

/* Wanted must be a NULL terminated string */
void *GSHashMapDelete(gs_hash_map *Self, char *Wanted) {
        u32 HashIndex = __GSHashMapComputeHash(Self, Wanted);
        char *Key = &Self->Keys[HashIndex * Self->MaxKeyLength];
        if (GSStringIsEqual(Wanted, Key, GSStringLength(Key))) {
                void *Result = Self->Values[HashIndex];
                Self->Values[HashIndex] = NULL;
                Self->Keys[HashIndex * Self->MaxKeyLength] = GSNullChar;
                Self->Count--;
                return Result;
        }

        u32 StartHash = HashIndex;
        HashIndex = (HashIndex + 1) % Self->Capacity;

        while (true) {
                if (HashIndex == StartHash) break;

                Key = &Self->Keys[HashIndex * Self->MaxKeyLength];
                if (GSStringIsEqual(Wanted, Key, GSStringLength(Key))) {
                        void *Result = Self->Values[HashIndex];
                        Self->Values[HashIndex] = NULL;
                        Self->Keys[HashIndex * Self->MaxKeyLength] = GSNullChar;
                        Self->Count--;
                        return Result;
                }
                HashIndex = (HashIndex + 1) % Self->Capacity;
        }

        return NULL;
}

/******************************************************************************
 * Byte streams / Buffers / File IO
 ******************************************************************************/

typedef struct gs_buffer {
        char *Start;
        char *Cursor;
        u64 Capacity;
        u64 Length;
        char *SavedCursor;
} gs_buffer;

gs_buffer *GSBufferInit(gs_buffer *Buffer, char *Start, u64 Size) {
        Buffer->Start = Start;
        Buffer->Cursor = Start;
        Buffer->Length = 0;
        Buffer->Capacity = Size;
        Buffer->SavedCursor = NULL;
        return Buffer;
}

bool GSBufferIsEOF(gs_buffer *Buffer) {
        u64 Size = Buffer->Cursor - Buffer->Start;
        bool Result = Size >= Buffer->Length;
        return Result;
}

void GSBufferNextLine(gs_buffer *Buffer) {
        while (true) {
                if (Buffer->Cursor[0] == '\n' ||
                   Buffer->Cursor[0] == '\0') {
                        break;
                }
                Buffer->Cursor++;
        }
        Buffer->Cursor++;
}

bool GSBufferSaveCursor(gs_buffer *Buffer) {
        Buffer->SavedCursor = Buffer->Cursor;
        return true;
}

bool GSBufferRestoreCursor(gs_buffer *Buffer) {
        if (Buffer->SavedCursor == NULL) return false;

        Buffer->Cursor = Buffer->SavedCursor;
        Buffer->SavedCursor = NULL;
        return true;
}

// TODO: Move to platform-specific header include.
/* size_t */
/* GSFileSize(char *FileName) */
/* { */
/*         size_t FileSize = 0; */
/*         FILE *File = fopen(FileName, "r"); */
/*         if (File != NULL) */
/*         { */
/*                 fseek(File, 0, SEEK_END); */
/*                 FileSize = ftell(File); */
/*                 fclose(File); */
/*         } */
/*         return FileSize; */
/* } */

// TODO: Move to platform-specific header include
/* bool */
/* GSFileCopyToBuffer(char *FileName, gs_buffer *Buffer) */
/* { */
/*         FILE *File = fopen(FileName, "r"); */
/*         if (File == NULL) return false; */

/*         fseek(File, 0, SEEK_END); */
/*         size_t FileSize = ftell(File); */
/*         int Remaining = (Buffer->Start + Buffer->Capacity) - Buffer->Cursor; */
/*         if (FileSize > Remaining) return false; */

/*         fseek(File, 0, SEEK_SET); */
/*         size_t BytesRead = fread(Buffer->Cursor, 1, FileSize, File); */
/*         Buffer->Length += FileSize; */
/*         Buffer->Cursor += FileSize; */
/*         *(Buffer->Cursor) = '\0'; */

/*         return true; */
/* } */

#endif /* GS_H */
