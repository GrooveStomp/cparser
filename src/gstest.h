/******************************************************************************
 * File: gstest.h
 * Created: 2016-08-19
 * Last Updated: 2016-08-22
 * Creator: Aaron Oman (a.k.a GrooveStomp)
 * Notice: (C) Copyright 2016 by Aaron Oman
 *-----------------------------------------------------------------------------
 *
 * Library containing functions and macros to aid in testing other C code.
 *
 ******************************************************************************/
#define GS_TEST_VERSION 0.1.0

#include <stdlib.h> /* EXIT_SUCCESS, EXIT_FAILURE */

#define GSTestAssert(Expression, ...) \
        { \
                if(!(Expression)) \
                { \
                        fprintf(stderr, "Assertion failed in %s() at line #%d: ", __func__, __LINE__); \
                        fprintf(stderr, __VA_ARGS__); \
                        exit(EXIT_FAILURE); \
                } \
        }

unsigned int /* Memory should be MaxLength size, at least. */
GSTestRandomString(char *Memory, unsigned int MinLength, unsigned int MaxLength)
{
        int Length = rand() % (MaxLength - MinLength) + MinLength;
        int Range = 'z' - 'a';
        for(int I=0; I<Length; I++)
        {
                Memory[I] = rand() % Range + 'a';
        }
        Memory[Length] = '\0';

        return(Length);
}
