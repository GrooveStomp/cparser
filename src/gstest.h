/******************************************************************************
 * File: gstest.h
 * Created: 2016-08-19
 * Updated: 2020-11-03
 * Package: glsibc
 * Creator: Aaron Oman (GrooveStomp)
 * Copyright 2016 - 2020, Aaron Oman and the gslibc contributors
 * SPDX-License-Identifier: LGPL-3.0-only
 *-----------------------------------------------------------------------------
 *
 * Library containing functions and macros to aid in testing other C code.
 *
 ******************************************************************************/
#define GS_TEST_VERSION 0.1.0-dev

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
