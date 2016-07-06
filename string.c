#ifndef _STRING_C
#define _STRING_C

#include "bool.c"

bool
IsStringEqual(char *LeftString, char *RightString, int MaxNumToMatch)
{
	int NumMatched = 0;

	while(NumMatched < MaxNumToMatch)
	{
		if(*LeftString == *RightString)
		{
			LeftString++;
			RightString++;
			NumMatched++;
		}
		else
		{
			return(false);
		}
	}

	return(true);
}

int
StringLength(char *String)
{
	char *P = String;
	while(*P != '\0') P++;
	return(P - String);
}

#endif /* _STRING_C */
