#ifndef false
#define false 0
#define true !false
typedef int bool;
#endif

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
