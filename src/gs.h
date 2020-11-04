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
#ifndef GS_VERSION
#define GS_VERSION 0.2.0-dev

#define gs_ArraySize(Array) (sizeof((Array)) / sizeof((Array)[0]))

/******************************************************************************
 * Usage:
 *
 * i32 Numbers[] = { 1, 2, 3, 4, 5 };
 * gs_ArrayForEach(i32 *Number, Numbers) {
 *         printf("Number[%i]: %i\n", Index, *Number);
 * }
 *
 * NOTE:
 * The variable `Index' is automatically generated for you.
 * `Item' must be a pointer to the type of variable used in the Array.
 *
 * Implementation taken from: http://stackoverflow.com/a/400970
 ******************************************************************************/
#define gs_ArrayForEach(Item, Array) \
        for (i32 Keep##__LINE__ = 1, \
                Count##__LINE__ = 0, \
                Index = 0, \
                Size##__LINE__ = sizeof((Array)) / sizeof(*(Array)); \
            Keep##__LINE__ && Count##__LINE__ != Size##__LINE__; \
            Keep##__LINE__ = !Keep##__LINE__, Count##__LINE__++) \
                for (Item = (Array) + Count##__LINE__; Keep##__LINE__; Keep##__LINE__ = !Keep##__LINE__, Index++)

#define gs_Max(A, B) ((A) < (B) ? (B) : (A))
#define gs_Min(A, B) ((A) < (B) ? (A) : (B))

// TODO: Move to platform-specific header
/* #define gs_AbortWithMessage(...) \ */
/*         { \ */
/*                 char String##__LINE__[256];                             \ */
/*                 sprintf(String##__LINE__, "In %s() at line #%i: ", __func__, __LINE__); \ */
/*                 fprintf(stderr, String##__LINE__);                       \ */
/*                 fprintf(stderr, __VA_ARGS__); \ */
/*                 exit(EXIT_FAILURE); \ */
/*         } */

// TODO: Move to platform-specific header
/* #define gs_Log(...) \ */
/*         { \ */
/*                 char String##__LINE__[256];                             \ */
/*                 sprintf(String##__LINE__, "In %s() at line #%i: ", __func__, __LINE__); \ */
/*                 fprintf(stdout, String##__LINE__);                       \ */
/*                 fprintf(stdout, __VA_ARGS__); \ */
/*         } */

#define GS_1024_INVERSE 1.0/1024
#define gs_BytesToKilobytes(X) (X) * GS_1024_INVERSE
#define gs_BytesToMegabytes(X) gs_BytesToKilobytes((X)) * GS_1024_INVERSE
#define gs_BytesToGigabytes(X) gs_BytesToMegabytes((X)) * GS_1024_INVERSE

/******************************************************************************
 * Primitive Type Definitions
 * TODO: Conditionally do typedefs?
 ******************************************************************************/

#define GS_NULL_CHAR '\0'

#ifndef NULL
#define NULL 0
#endif

#define GS_NULL_PTR NULL

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

typedef struct gs_Allocator {
        void *(*malloc)(u64);
        void (*free)(void *);
        void *(*realloc)(void *, u64);
        void *(*calloc)(u64, u64);
} gs_Allocator;

/******************************************************************************
 * Character Definitions
 *-----------------------------------------------------------------------------
 * Functions to interact with C's basic ASCII char type.
 ******************************************************************************/

bool gs_CharIsEndOfStream(char c) {
	return c == '\0';
}

bool gs_CharIsEndOfLine(char c) {
	return (c == '\n') || (c == '\r');
}

bool gs_CharIsWhitespace(char c) {
	return (c == ' ') ||
	       (c == '\t') ||
	       (c == '\v') ||
	       (c == '\f') ||
	       gs_CharIsEndOfLine(c);
}

bool gs_CharIsOctal(char c) {
	return (c >= '0' && c <= '7');
}

bool gs_CharIsDecimal(char c) {
	return (c >= '0' && c <= '9');
}

bool gs_CharIsHexadecimal(char c) {
	return ((c >= '0' && c <= '9') ||
                (c >= 'a' && c <= 'f') ||
                (c >= 'A' && c <= 'F'));
}

bool gs_CharIsAlphabetical(char c) {
	return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

bool gs_CharIsAlphanumeric(char c) {
        return gs_CharIsAlphabetical(c) || gs_CharIsDecimal(c);
}

bool gs_CharIsUpcase(char c) {
        return gs_CharIsAlphabetical(c) && (c >= 'A') && (c <= 'z');
}

char gs_CharUpcase(char c) {
        char Result = c;

        if (gs_CharIsAlphabetical(c) && (c >= 'a' && c <= 'z')) {
                i32 Delta = c - 'a';
                Result = Delta + 'A';
        }

        return Result;
}

bool gs_CharIsDowncase(char c) {
        return gs_CharIsAlphabetical(c) && (c >= 'a') && (c <= 'z');
}

char gs_CharDowncase(char c) {
        char Result = c;

        if (gs_CharIsAlphabetical(c) && (c >= 'A' && c <= 'Z')) {
                i32 Delta = c - 'A';
                Result = Delta + 'a';
        }

        return Result;
}

/******************************************************************************
 * String Definitions
 *-----------------------------------------------------------------------------
 * C string type. That is, ASCII characters with terminating NULL.
 ******************************************************************************/

bool gs_StringIsEqual(char *LeftString, char *RightString, i32 MaxNumToMatch) {
	i32 NumMatched = 0;

        if (*LeftString == GS_NULL_CHAR || *RightString == GS_NULL_CHAR && *LeftString != *RightString) {
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

u32 gs_StringLength(char *String) {
	char *c = String;
	while (*c != '\0') c++;
	return c - String;
}

bool gs_StringCopy(char *Source, char *Dest, i32 Max) {
        if (Source == NULL || Dest == NULL) {
                return false;
        }

        i32 i = 0;
        for (; Source[i] != '\0' && i < Max; i++) {
                Dest[i] = Source[i];
        }
        Dest[i] = '\0';

        return true;
}

bool gs_StringCopyNoNull(char *Source, char *Dest, i32 Max) {
        if (Source == NULL || Dest == NULL) {
                return false;
        }

        for (i32 i = 0; Source[i] != '\0' && i < Max; i++) {
                Dest[i] = Source[i];
        }

        return true;
}

/* NOTE: Assumes a maximum string length of 512 bytes. */
/* Returns number of bytes copied. */
u32 gs_StringTrimWhitespace(char *Source, u32 MaxLength) {
        char Dest[512];
        MaxLength = gs_Min(512, MaxLength);

        i32 FirstChar, LastChar;
        for (FirstChar = 0; gs_CharIsWhitespace(Source[FirstChar]); FirstChar++);

        i32 StringLength = gs_StringLength(Source);
        for (LastChar = StringLength - 1; gs_CharIsWhitespace(Source[LastChar]); LastChar--);

        i32 Count = 0;
        for (i32 i = FirstChar; i <= LastChar && Count < MaxLength; Count++, i++) {
                Dest[Count] = Source[i];
        }

        for (i32 i = 0; i < Count; i++) {
                Source[i] = Dest[i];
        }
        Source[Count] = GS_NULL_CHAR;

        return Count;
}

/*
  For any ascii character following an underscore, remove the underscore
  and capitalize the ascii char.
  This function assumes a maximum string size of 512 bytes.
  The first character is capitalized.
*/
u32 gs_StringSnakeCaseToCamelCase(char *Source, u32 SourceLength) {
        char Dest[512]; /* Scratch buffer. */
        i32 i = 0, j = 0; /* Iterable indices for Source and Dest. */

        if ((Source[i] == '_') && (i + 1 < SourceLength) && gs_CharIsAlphabetical(Source[i+1])) {
                i++;
        }

        Dest[j] = gs_CharUpcase(Source[i]);
        i++;
        j++;

        SourceLength = gs_Min(512, SourceLength);

        for (i, j; i<SourceLength; i++, j++) {
                if ((Source[i] == '_') && (i + 1 < SourceLength) && gs_CharIsAlphabetical(Source[i+1])) {
                        /* Replace any '_*' with 'upcase(*)' where * is an ascii char. */
                        Dest[j] = gs_CharUpcase(Source[i + 1]);
                        i++;
                } else {
                        /* Copy chars normally. */
                        Dest[j] = Source[i];
                }
        }

        /* Write the modified string back to source. */
        for (i32 k = 0; k < j; k++) {
                Source[k] = Dest[k];
        }
        Source[j] = GS_NULL_CHAR;

        return j;
}

/*
  Prerequisites:
  - Dest must be large enough to contain the modified string.

  For any Capitalized ascii character, replace with an underscore followed by
  the lowercase version of that character. This does not apply to leading char.
  eg.: CamelCase -> Camel_case
*/
u32 gs_StringCamelCaseToSnakeCase(char *Source, char *Dest, u32 SourceLength) {
        i32 i = 0, j = 0; /* Iterable indices for Source and Dest. */
        Dest[i] = gs_CharDowncase(Source[i]);
        i++;
        j++;

        for (i, j; i<SourceLength && Source[i] != GS_NULL_CHAR; i++, j++) {
                /* Replace upcase ascii char with '_' and downcase ascii char. */
                if (gs_CharIsUpcase(Source[i])) {
                        Dest[j] = '_';
                        j++;
                        Dest[j] = gs_CharDowncase(Source[i]);
                }
                /* Copy chars normally. */
                else {
                        Dest[j] = Source[i];
                }
        }
        Dest[j] = GS_NULL_CHAR;

        return j;
}

/*
  Capitalizes the first character found.
  Modifies Source in-place.
  Returns Source.
  eg.: hello -> Hello
       123foos -> 123Foos
*/
char *gs_StringCapitalize(char *Source, u32 Length) {
        i32 i = 0;

        while (true) {
                if (i >= Length) break;
                if (Source[i] == GS_NULL_CHAR) break;
                if (gs_CharIsAlphabetical(Source[i])) break;

                i++;
        }

        if (i >= Length) return Source;

        Source[i] = gs_CharUpcase(Source[i]);

        return Source;
}

typedef bool (*gs_StringFilterFn)(char C);

/* Returns length of new string */
i32 gs_StringKeep(char *Source, char *Dest, u32 MaxLength, gs_StringFilterFn FilterFn) {
        i32 i = 0;
        i32 j = 0;

	while (i < MaxLength) {
                if (FilterFn(Source[i])) {
                        Dest[j] = Source[i];
                        j++;
                }
                i++;
        }
        Dest[j] = GS_NULL_CHAR;

        return j + 1;
}

/* Returns length of new string */
i32 gs_StringReject(char *Source, char *Dest, u32 MaxLength, gs_StringFilterFn FilterFn) {
        i32 i = 0;
        i32 j = 0;

	while (i < MaxLength) {
                if (!FilterFn(Source[i])) {
                        Dest[j] = Source[i];
                        j++;
                }
                i++;
        }
        Dest[j] = GS_NULL_CHAR;

        return j + 1;
}

/******************************************************************************
 * Hash Map
 *-----------------------------------------------------------------------------
 *
 * Usage:
 *     char *Value = "value";
 *     i32 StringLength = 256;
 *     i32 NumElements = 13;
 *     u32 BytesRequired = gs_HashMapBytesRequired(StringLength, NumElements);
 *     gs_hash_map *Map = gs_HashMapInit(alloca(BytesRequired), StringLength, NumElements);
 *     gs_HashMapSet(Map, "key", Value);
 *     if (GSHashMapHasKey(Map, "key")) {
 *         char *Result = (char *)GSHashMapGet(Map, "key");
 *         printf("Key(%s), Value(%s)\n", "key", Result);
 *     }
 ******************************************************************************/

typedef struct gs_HashMap {
        u32 count;
        u32 allocated_bytes;
        u32 capacity;
        u32 max_key_length;

        char *keys;
        void **values;
} gs_HashMap;

/* String must be a NULL-terminated string */
u32 __gs_HashMapComputeHash(gs_HashMap *self, char *string) {
        /* sdbm hash function: http://stackoverflow.com/a/14409947 */
        u32 hash_address = 0;
        for (u32 i = 0; string[i] != GS_NULL_CHAR; i++) {
                hash_address = string[i] +
                        (hash_address << 6) +
                        (hash_address << 16) -
                        hash_address;
        }
        u32 result = hash_address % self->capacity;

        return result;
}

u32 gs_HashMapBytesRequired(u32 max_key_length, u32 num_entries) {
        i32 alloc_size =
                sizeof(gs_HashMap) +
                (sizeof(char) * max_key_length * num_entries) +
                (sizeof(void *) * num_entries);

        return alloc_size;
}

gs_HashMap *gs_HashMapInit(void *memory, u32 max_key_length, u32 num_entries) {
        gs_HashMap *self = (gs_HashMap *)memory;

        char *key_value_memory = (char *)memory + sizeof(gs_HashMap);

        self->max_key_length = max_key_length;
        self->capacity = num_entries;
        self->allocated_bytes = gs_HashMapBytesRequired(max_key_length, num_entries);
        self->count = 0;

        i32 keys_mem_length = max_key_length * num_entries;

        self->keys = key_value_memory;
        for (i32 i = 0; i < keys_mem_length; i++) {
                self->keys[i] = 0;
        }

        self->values = (void **)(self->keys + keys_mem_length);
        for (i32 i = 0; i < num_entries; i++) {
                self->values[i] = 0;
        }

        return self;
}

bool __gs_HashMapUpdate(gs_HashMap *self, char *key, void *value) {
        u32 key_length = gs_StringLength(key);
        u32 hash_index = __gs_HashMapComputeHash(self, key);

        u32 start_hash = hash_index;

        do {
                if (gs_StringIsEqual(&self->keys[hash_index * self->max_key_length], key, gs_StringLength(key))) {
                        self->values[hash_index] = value;
                        return true;
                }
                hash_index = (hash_index + 1) % self->capacity;
        } while (hash_index != start_hash);

        /* Couldn't find Key to update. */
        return false;
}

/* wanted must be a NULL terminated string */
bool gs_HashMapHasKey(gs_HashMap *self, char *wanted) {
        u32 hash_index = __gs_HashMapComputeHash(self, wanted);
        char *key = &self->keys[hash_index * self->max_key_length];
        if (gs_StringIsEqual(wanted, key, gs_StringLength(wanted))) {
                return true;
        }

        u32 start_hash = hash_index;
        hash_index = (hash_index + 1) % self->capacity;

        while (true) {
                if (hash_index == start_hash) break;

                key = &self->keys[hash_index * self->max_key_length];
                if (gs_StringIsEqual(wanted, key, gs_StringLength(wanted))) {
                        return true;
                }
                hash_index = (hash_index + 1) % self->capacity;
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
bool gs_HashMapSet(gs_HashMap *self, char *key, void *value) {
        u32 key_length = gs_StringLength(key);
        u32 hash_index = __gs_HashMapComputeHash(self, key);

        if (gs_HashMapHasKey(self, key)) {
                return __gs_HashMapUpdate(self, key, value);
        }

        /* We're not updating, so return false if we're at capacity. */
        if (self->count >= self->capacity) return false;

        /* Add a brand-new key in. */
        if (self->keys[hash_index * self->max_key_length] == GS_NULL_CHAR) {
                gs_StringCopy(key, &self->keys[hash_index * self->max_key_length], key_length);
                self->values[hash_index] = value;
                self->count++;
                return true;
        }

        /* We have a collision! Find a free index. */
        u32 start_hash = hash_index;
        hash_index = (hash_index + 1) % self->capacity;

        while (true) {
                if (hash_index == start_hash) break;

                if (self->keys[hash_index * self->max_key_length] == GS_NULL_CHAR) {
                        gs_StringCopy(key, &self->keys[hash_index * self->max_key_length], key_length);
                        self->values[hash_index] = value;
                        self->count++;
                        return true;
                }
                hash_index = (hash_index + 1) % self->capacity;
        }

        /* Couldn't find any free space. */
        return false;
}

/* Memory must be large enough for the resized Hash. Memory _cannot_ overlap! */
bool gs_HashMapGrow(gs_HashMap **self, u32 num_entries, void *memory) {
        gs_HashMap *old = *self;

        /* No point in making smaller... */
        if (num_entries <= old->capacity) return false;
        if (memory == GS_NULL_PTR) return false;

        *self = gs_HashMapInit(memory, old->max_key_length, num_entries);
        for (i32 i = 0; i < old->capacity; i++) {
                char *key = &old->keys[i * old->max_key_length];
                char *value = (char *)(old->values[i]);
                if (key != NULL) {
                        bool success = gs_HashMapSet(*self, key, value);
                        if (!success) return false;
                }
        }

        return true;
}

/* Wanted must be a NULL terminated string */
void *gs_HashMapGet(gs_HashMap *self, char *wanted) {
        u32 hash_index = __gs_HashMapComputeHash(self, wanted);
        char *key = &self->keys[hash_index * self->max_key_length];
        if (gs_StringIsEqual(wanted, key, gs_StringLength(key))) {
                void *result = self->values[hash_index];
                return result;
        }

        u32 start_hash = hash_index;
        hash_index = (hash_index + 1) % self->capacity;

        while (true) {
                if (hash_index == start_hash) break;

                key = &self->keys[hash_index * self->max_key_length];
                if (gs_StringIsEqual(wanted, key, gs_StringLength(key))) {
                        void *result = self->values[hash_index];
                        return result;
                }
                hash_index = (hash_index + 1) % self->capacity;
        }

        return NULL;
}

/* Wanted must be a NULL terminated string */
void *gs_HashMapDelete(gs_HashMap *self, char *wanted) {
        u32 hash_index = __gs_HashMapComputeHash(self, wanted);
        char *key = &self->keys[hash_index * self->max_key_length];
        if (gs_StringIsEqual(wanted, key, gs_StringLength(key))) {
                void *result = self->values[hash_index];
                self->values[hash_index] = NULL;
                self->keys[hash_index * self->max_key_length] = GS_NULL_CHAR;
                self->count--;
                return result;
        }

        u32 start_hash = hash_index;
        hash_index = (hash_index + 1) % self->capacity;

        while (true) {
                if (hash_index == start_hash) break;

                key = &self->keys[hash_index * self->max_key_length];
                if (gs_StringIsEqual(wanted, key, gs_StringLength(key))) {
                        void *result = self->values[hash_index];
                        self->values[hash_index] = NULL;
                        self->keys[hash_index * self->max_key_length] = GS_NULL_CHAR;
                        self->count--;
                        return result;
                }
                hash_index = (hash_index + 1) % self->capacity;
        }

        return NULL;
}

/******************************************************************************
 * Byte streams / Buffers / File IO
 ******************************************************************************/

typedef struct gs_Buffer {
        char *start;
        char *cursor;
        u64 capacity;
        u64 length;
        char *saved_cursor;
} gs_Buffer;

gs_Buffer *gs_BufferInit(gs_Buffer *buffer, char *start, u64 size) {
        buffer->start = start;
        buffer->cursor = start;
        buffer->length = 0;
        buffer->capacity = size;
        buffer->saved_cursor = GS_NULL_PTR;

        return buffer;
}

bool gs_BufferIsEOF(gs_Buffer *buffer) {
        u64 size = buffer->cursor - buffer->start;
        bool result = size >= buffer->length;
        return result;
}

void gs_BufferNextLine(gs_Buffer *buffer) {
        while (true) {
                if (buffer->cursor[0] == '\n' || buffer->cursor[0] == '\0') break;

                buffer->cursor++;
        }
        buffer->cursor++;
}

bool gs_BufferSaveCursor(gs_Buffer *buffer) {
        buffer->saved_cursor = buffer->cursor;
        return true;
}

bool gs_BufferRestoreCursor(gs_Buffer *buffer) {
        if (buffer->saved_cursor == GS_NULL_PTR) return false;

        buffer->cursor = buffer->saved_cursor;
        buffer->saved_cursor = NULL;

        return true;
}

// TODO: Move to platform-specific header include.
/* size_t */
/* gs_Filesize(char *FileName) */
/* { */
/*         size_t Filesize = 0; */
/*         FILE *File = fopen(FileName, "r"); */
/*         if (File != NULL) */
/*         { */
/*                 fseek(File, 0, SEEK_END); */
/*                 Filesize = ftell(File); */
/*                 fclose(File); */
/*         } */
/*         return Filesize; */
/* } */

// TODO: Move to platform-specific header include
/* bool */
/* gs_FileCopyToBuffer(char *FileName, gs_Buffer *buffer) */
/* { */
/*         FILE *File = fopen(FileName, "r"); */
/*         if (File == NULL) return false; */

/*         fseek(File, 0, SEEK_END); */
/*         size_t Filesize = ftell(File); */
/*         i32 Remaining = (buffer->start + buffer->Capacity) - buffer->Cursor; */
/*         if (Filesize > Remaining) return false; */

/*         fseek(File, 0, SEEK_SET); */
/*         size_t BytesRead = fread(buffer->Cursor, 1, Filesize, File); */
/*         buffer->Length += Filesize; */
/*         buffer->Cursor += Filesize; */
/*         *(buffer->Cursor) = '\0'; */

/*         return true; */
/* } */

#endif /* GS_VERSION */
