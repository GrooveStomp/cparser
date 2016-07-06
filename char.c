#ifndef false
#define false 0
#define true !false
typedef int bool;
#endif

bool
IsEndOfStream(char C)
{
	return(C == '\0');
}

bool
IsEndOfLine(char C)
{
	return((C == '\n') ||
	       (C == '\r'));
}

bool
IsWhitespace(char C)
{
	return((C == ' ') ||
	       (C == '\t') ||
	       (C == '\v') ||
	       (C == '\f') ||
	       IsEndOfLine(C));
}

bool
IsOctal(char C)
{
	bool Result = (C >= '0' && C <= '7');
	return(Result);
}

bool
IsDecimal(char C)
{
	bool Result = (C >= '0' && C <= '9');
	return(Result);
}

bool
IsHexadecimal(char C)
{
	bool Result = ((C >= '0' && C <= '9') ||
		       (C >= 'a' && C <= 'f') ||
		       (C >= 'A' && C <= 'F'));
	return(Result);
}

bool
IsIntegerSuffix(char C)
{
	bool Result = (C == 'u' ||
		       C == 'U' ||
		       C == 'l' ||
		       C == 'L');
	return(Result);
}

bool
IsAlphabetical(char C)
{
	bool Result = ((C >= 'a' && C <= 'z') || (C >= 'A' && C <= 'Z'));
	return(Result);
}

bool
IsIdentifierCharacter(char C)
{
	bool Result = (IsAlphabetical(C) || IsDecimal(C) || C == '_');
	return(Result);
}
