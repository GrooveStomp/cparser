/*
  TODO:
  - Troubleshoot segmentation fault in parser. See corresponding TODO.
  - Integer lexing is not correct? Double check C reference and
    integer-constants.txt test file.
  - Unicode support? C unicode libraries: icu, utf8proc, *microutf8*, *nunicode*
    Both microutf8 and nunicode look quite good.g
 */
#include <stdio.h>
#include <alloca.h>
#include <stdlib.h> /* EXIT_SUCCESS, EXIT_FAILURE */
#include <string.h> /* memcpy */

typedef int bool;
#define false 0
#define true !false

#include "file_buffer.c"
#include "string.h"
#include "char.h"

#define ARRAY_SIZE(Array) (sizeof((Array)) / sizeof((Array)[0]))

#define Min(a,b) ((a) < (b) ? (a) : (b))
#define Bytes(n) (n)
#define Kilobytes(n) (Bytes(n) * 1000)

/* GCC-specific defines that let us remove compiler size warnings below. */
#if __LP64__
#define CastSizeIntTo32Bits(x) (int)(x)
#else
#define CastSizeIntTo32Bits(x) (x)
#endif

enum token_type
{
	Token_Unknown,

	Token_Asterisk,
	Token_Ampersand,
	Token_OpenParen,
	Token_CloseParen,
	Token_OpenBracket,
	Token_CloseBracket,
	Token_OpenBrace,
	Token_CloseBrace,
	Token_Colon,
	Token_SemiColon,
	Token_PercentSign,
	Token_QuestionMark,
	Token_EqualSign,	/*  = */
	Token_Carat,
	Token_Comma,		/*  , */
	Token_Cross,		/*  + */
	Token_Dash,		/*  - */
	Token_Slash,		/*  / */
	Token_Dot,		/*  . */
	Token_Bang,
	Token_Pipe,
	Token_LessThan,
	Token_GreaterThan,
	Token_Tilde,

	Token_NotEqual,
	Token_GreaterThanEqual,
	Token_LessThanEqual,
	Token_LogicalOr,
	Token_LogicalAnd,
	Token_BitShiftLeft,
	Token_BitShiftRight,
	Token_Arrow,		/* -> */
	Token_PlusPlus,
	Token_MinusMinus,
	Token_MultiplyEquals,
	Token_DivideEquals,
	Token_ModuloEquals,
	Token_PlusEquals,
	Token_MinusEquals,
	Token_DoubleLessThanEquals,
	Token_DoubleGreaterThanEquals,
	Token_AmpersandEquals,
	Token_CaratEquals,
	Token_PipeEquals,

	Token_Character,
	Token_String,
	Token_Identifier,
	Token_Keyword,
	Token_PreprocessorCommand,
	Token_Comment,
	Token_Integer,
	Token_PrecisionNumber,

	Token_EndOfStream,
};

char *
TokenName(enum token_type Type)
{
	switch(Type)
	{
		case Token_Unknown: { return "Unknown"; } break;

		case Token_Asterisk: { return "Asterisk"; } break;
		case Token_Ampersand: { return "Ampersand"; } break;
		case Token_OpenParen: { return "OpenParen"; } break;
		case Token_CloseParen: { return "CloseParen"; } break;
		case Token_OpenBracket: { return "OpenBracket"; } break;
		case Token_CloseBracket: { return "CloseBracket"; } break;
		case Token_OpenBrace: { return "OpenBrace"; } break;
		case Token_CloseBrace: { return "CloseBrace"; } break;
		case Token_Colon: { return "Colon"; } break;
		case Token_SemiColon: { return "SemiColon"; } break;
		case Token_PercentSign: { return "PercentSign"; } break;
		case Token_QuestionMark: { return "QuestionMark"; } break;
		case Token_EqualSign: { return "EqualSign"; } break;
		case Token_Carat: { return "Carat"; } break;
		case Token_Comma: { return "Comma"; } break;
		case Token_Cross: { return "Cross"; } break;
		case Token_Dash: { return "Dash"; } break;
		case Token_Slash: { return "Slash"; } break;
		case Token_Dot: { return "Dot"; } break;
		case Token_Bang: { return "Bang"; } break;
		case Token_Pipe: { return "Pipe"; } break;
		case Token_LessThan: { return "LessThan"; } break;
		case Token_GreaterThan: { return "GreaterThan"; } break;
		case Token_Tilde: { return "Tilde"; } break;

		case Token_NotEqual: { return "NotEqual"; } break;
		case Token_GreaterThanEqual: { return "GreaterThanEqual"; } break;
		case Token_LessThanEqual: { return "LessThanEqual"; } break;
		case Token_LogicalOr: { return "LogicalOr"; } break;
		case Token_LogicalAnd: { return "LogicalAnd"; } break;
		case Token_BitShiftLeft: { return "BitShiftLeft"; } break;
		case Token_BitShiftRight: { return "BitShiftRight"; } break;
		case Token_Arrow: { return "Arrow"; } break;
		case Token_PlusPlus: { return "PlusPlus"; } break;
		case Token_MinusMinus: { return "MinusMinus"; } break;

		case Token_MultiplyEquals: { return "MultiplyEquals"; } break;
		case Token_DivideEquals: { return "DivideEquals"; } break;
		case Token_ModuloEquals: { return "ModuloEquals"; } break;
		case Token_PlusEquals: { return "PlusEquals"; } break;
		case Token_MinusEquals: { return "MinusEquals"; } break;
		case Token_DoubleLessThanEquals: { return "DoubleLessThanEquals"; } break;
		case Token_DoubleGreaterThanEquals: { return "DoubleGreaterThanEquals"; } break;
		case Token_AmpersandEquals: { return "AmpersandEquals"; } break;
		case Token_CaratEquals: { return "CaratEquals"; } break;
		case Token_PipeEquals: { return "PipeEquals"; } break;

		case Token_Character: { return "Character"; } break;
		case Token_String: { return "String"; } break;
		case Token_Identifier: { return "Identifier"; } break;
		case Token_Keyword: { return "Keyword"; } break;
		case Token_PreprocessorCommand: { return "PreprocessorCommand"; } break;
		case Token_Comment: { return "Comment"; } break;
		case Token_Integer: { return "Integer"; } break;
		case Token_PrecisionNumber: { return "PrecisionNumber"; } break;

		case Token_EndOfStream: { return "EndOfStream"; } break;
	}
}

struct token
{
	char *Text;
	size_t TextLength;
	enum token_type Type;
	int Line;
	int Column;
};

struct tokenizer
{
	char *Beginning;
	char *At;
	int Line;
	int Column;
};

void
AdvanceTokenizer(struct tokenizer *Tokenizer)
{
	if(IsEndOfLine(Tokenizer->At[0]))
	{
		++Tokenizer->Line;
		Tokenizer->Column = 0;
	}
	else
	{
		++Tokenizer->Column;
	}
	++Tokenizer->At;
}

void
CopyTokenizer(struct tokenizer *Source, struct tokenizer *Dest)
{
	Dest->Beginning = Source->Beginning;
	Dest->At = Source->At;
	Dest->Line = Source->Line;
	Dest->Column = Source->Column;
}

void
AdvanceTokenizerToChar(struct tokenizer *Tokenizer, char Char)
{
	while(!IsEndOfStream(Tokenizer->At[0]))
	{
		if(Tokenizer->At[0] == Char) break;
		AdvanceTokenizer(Tokenizer);
	}
}

bool
CopyToTokenAndAdvance(struct tokenizer *Tokenizer, struct token *Token, int Length, enum token_type Type)
{
	Token->Text = Tokenizer->At;
	Token->TextLength = Length;
	Token->Type = Type;
	Token->Line = Tokenizer->Line;
	Token->Column = Tokenizer->Column;

	for(int I=0; I<Length; ++I) AdvanceTokenizer(Tokenizer);
}

void
EatAllWhitespace(struct tokenizer *Tokenizer)
{
	for(; IsWhitespace(Tokenizer->At[0]); AdvanceTokenizer(Tokenizer));
}

bool
GetCharacter(struct tokenizer *Tokenizer, struct token *Token)
{
	char *Cursor = Tokenizer->At;

	/* First character must be a single quote. */
	if(*Cursor != '\'') return(false);
	++Cursor; /* Skip past the first single quote. */

	/* Read until closing single quote. */
	for(; *Cursor != '\''; ++Cursor);

	/* If previous character is an escape, then closing quote is next char. */
	if(*(Cursor-1) == '\\' && *(Cursor -2) != '\\')
	{
		++Cursor;
	}
	++Cursor; /* Point to character after literal. */

	/* Longest char literal is: '\''. */
	if(Cursor - Tokenizer->At > 4) return(false);

	CopyToTokenAndAdvance(Tokenizer, Token, Cursor - Tokenizer->At, Token_Character);

	return(true);
}

bool
GetString(struct tokenizer *Tokenizer, struct token *Token)
{
	char *Cursor = Tokenizer->At;
	if(*Cursor != '"') return(false);

	while(true)
	{
		++Cursor;
		if(*Cursor == '\0') return(false);
		if(*Cursor == '"' && *(Cursor - 1) != '\\') break;
	}
	++Cursor; /* Swallow the last double quote. */

	CopyToTokenAndAdvance(Tokenizer, Token, Cursor - Tokenizer->At, Token_String);

	return(true);
}

bool
IsOctalString(char *Text, int Length)
{
	if(Length < 2) return(false);

	char Last = Text[Length-1];

	/* Leading 0 required for octal integers. */
	if('0' != Text[0]) return(false);

	if(!(IsOctal(Last) || IsIntegerSuffix(Last))) return(false);

	/* Loop from character after leading '0' to second-last. */
	for(int i=1; i<Length-1; ++i)
	{
		if(!IsOctal(Text[i])) return(false);
	}

	return(true);
}

bool
IsHexadecimalString(char *Text, int Length)
{
	if(Length < 3) return(false);

	char Last = Text[Length-1];

	/* Hex numbers must start with: '0x' or '0X'. */
	if(!(Text[0] == '0' && (Text[1] == 'x' || Text[1] == 'X'))) return(false);

	if(!(IsHexadecimal(Last) || IsIntegerSuffix(Last))) return(false);

	/* Loop from character after leading '0x' to second-last. */
	for(int i=2; i<Length-1; ++i)
	{
		if(!IsHexadecimal(Text[i])) return(false);
	}

	return(true);
}

bool
GetPrecisionNumber(struct tokenizer *Tokenizer, struct token *Token)
{
	/*
	  A floating constant consists of an integer part, a decimal point, a
	  fraction part, an e or E, an optionally signed integer exponent and an
	  optional type suffix, one of f, F, l, or L.
	  The integer and fraction parts both consist of a sequence of digits.
	  Either the integer part or the fraction part (not both) may be
	  missing; either the decimal point or the e and the exponent (not both)
	  may be missing.
	  The type is determined by the suffix; F or f makes it float, L or l
	  makes it long double; otherwise it is double.
	 */

	char *Cursor = Tokenizer->At;

	bool HasIntegerPart = false;
	bool HasDecimalPoint = false;
	bool HasFractionalPart = false;
	bool HasExponentPart = false;

	if(IsDecimal(*Cursor))
	{
		for(; IsDecimal(*Cursor); ++Cursor);
		HasIntegerPart = true;
	}

	if('.' == *Cursor)
	{
		++Cursor;
		HasDecimalPoint = true;
		if(IsDecimal(*Cursor))
		{
			for(; IsDecimal(*Cursor); ++Cursor);
			HasFractionalPart = true;
		}
	}

	if('e' == *Cursor || 'E' == *Cursor)
	{
		++Cursor;
		HasExponentPart = true;

		/* Optional negative sign for exponent is allowed. */
		if('-' == *Cursor)
		{
			++Cursor;
		}

		/* Exponent must contain an exponent part. */
		if(!IsDecimal(*Cursor))
		{
			return(false);
		}

		for(; IsDecimal(*Cursor); ++Cursor);
	}

	/* IsFloatSuffix(C) */
	char C = *Cursor;
	if('f' == C || 'F' == C || 'l' == C || 'L' == C)
	{
		++Cursor;
	}

	if((HasIntegerPart || HasFractionalPart) &&
	   (HasDecimalPoint || HasExponentPart))
	{
		CopyToTokenAndAdvance(Tokenizer, Token, Cursor - Tokenizer->At, Token_PrecisionNumber);
		return(true);
	}

	return(false);
}

bool
GetInteger(struct tokenizer *Tokenizer, struct token *Token)
{
	char *LastChar = Tokenizer->At;
	for(; *LastChar && (IsDecimal(*LastChar) || IsAlphabetical(*LastChar)); ++LastChar);

	int Length = LastChar - Tokenizer->At;
	--LastChar;

	/* Token->Type = Token_Unknown; */
	/* Token->Text = Tokenizer->At; */
	/* Token->TextLength = Length; */

	if(Length < 1) return(false);

	char *Cursor = Tokenizer->At;

	if ((IsOctalString(Cursor, Length) || IsHexadecimalString(Cursor, Length)) ||
	    (1 == Length && '0' == *Cursor))
	{
		CopyToTokenAndAdvance(Tokenizer, Token, Length, Token_Integer);
		return(true);
	}

	/* Can't have a multi-digit integer starting with zero unless it's Octal. */
	if('0' == *Cursor)
	{
		return(false);
	}

	for(int I=0; I<Length-1; ++I)
	{
		if(!IsDecimal(Cursor[I]))
		{
			return(false);
		}
	}

	if(IsDecimal(*LastChar) || IsIntegerSuffix(*LastChar))
	{
		CopyToTokenAndAdvance(Tokenizer, Token, Length, Token_Integer);
		return(true);
	}

	return(false);
}

bool
GetIdentifier(struct tokenizer *Tokenizer, struct token *Token)
{
	if(!IsAlphabetical(Tokenizer->At[0])) return(false);

	char *Cursor = Tokenizer->At;

	for(; IsIdentifierCharacter(*Cursor); ++Cursor);

	CopyToTokenAndAdvance(Tokenizer, Token, Cursor - Tokenizer->At, Token_Identifier);

	return(true);
}

bool
GetComment(struct tokenizer *Tokenizer, struct token *Token)
{
	if(Tokenizer->At[0] != '/' || Tokenizer->At[1] != '*') return(false);

	char *Cursor = Tokenizer->At;

	while(true)
	{
		if('\0' == *Cursor)
		{
			return(false);
		}
		if('*' == Cursor[0] && '/' == Cursor[1])
		{
			break;
		}
		++Cursor;
	}

	Cursor += 2; /* Swallow last two characters: asterisk, slash */

	CopyToTokenAndAdvance(Tokenizer, Token, Cursor - Tokenizer->At, Token_Comment);

	return(true);
}

bool
GetKeyword(struct tokenizer *Tokenizer, struct token *Token)
{
	static char *Keywords[] = {
		"auto", "break", "case", "char", "const", "continue", "default",
		"do", "double", "else", "enum", "extern", "float", "for",
		"goto", "if", "int", "long", "register", "return", "short",
		"signed", "sizeof", "static", "struct", "switch", "typedef",
		"union", "unsigned", "void", "volatile", "while"
	};

	for(int i = 0; i < ARRAY_SIZE(Keywords); ++i)
	{
		if(IsStringEqual(Tokenizer->At, Keywords[i], StringLength(Keywords[i])))
		{
			CopyToTokenAndAdvance(Tokenizer, Token, StringLength(Keywords[i]), Token_Keyword);

			return(true);
		}
	}

	return(false);
}

bool
GetPreprocessorCommand(struct tokenizer *Tokenizer, struct token *Token)
{
	if(Tokenizer->At[0] != '#') return(false);

	char *Cursor = Tokenizer->At;

	/* Preprocessor commands must start a line on their own. */
	for(--Cursor; Cursor > Tokenizer->Beginning && IsWhitespace(*Cursor); --Cursor);

	if(*(++Cursor) != '\n' && Cursor != Tokenizer->Beginning)
	{
		return(false);
	}
	Cursor = Tokenizer->At + 1; /* Skip the starting '#' for macros. */

	while(true)
	{
		if(*Cursor == '\n' && *(Cursor - 1) != '\\') break;
		if(*Cursor == '\0') break;
		++Cursor;
	}

	CopyToTokenAndAdvance(Tokenizer, Token, Cursor - Tokenizer->At, Token_PreprocessorCommand);

	return(true);
}

bool
GetSymbol(struct tokenizer *Tokenizer, struct token *Token, char *Symbol, enum token_type Type)
{
	int Length = StringLength(Symbol);
	if(!IsStringEqual(Tokenizer->At, Symbol, Length)) return(false);

	CopyToTokenAndAdvance(Tokenizer, Token, Length, Type);

	return(true);
}

struct token
GetToken(struct tokenizer *Tokenizer)
{
	EatAllWhitespace(Tokenizer);

	struct token Token;
	Token.Text = Tokenizer->At;
	Token.TextLength = 0;
	Token.Type = Token_Unknown;

	{
		GetSymbol(Tokenizer, &Token, "!=", Token_NotEqual) ||
		GetSymbol(Tokenizer, &Token, ">=", Token_GreaterThanEqual) ||
		GetSymbol(Tokenizer, &Token, "<=", Token_LessThanEqual) ||
		GetSymbol(Tokenizer, &Token, "->", Token_Arrow) ||
		GetSymbol(Tokenizer, &Token, "||", Token_LogicalOr) ||
		GetSymbol(Tokenizer, &Token, "&&", Token_LogicalAnd) ||
		GetSymbol(Tokenizer, &Token, "<<", Token_BitShiftLeft) ||
		GetSymbol(Tokenizer, &Token, ">>", Token_BitShiftRight) ||
		GetSymbol(Tokenizer, &Token, "++", Token_PlusPlus) ||
		GetSymbol(Tokenizer, &Token, "--", Token_MinusMinus) ||

		GetSymbol(Tokenizer, &Token, "*=", Token_MultiplyEquals) ||
		GetSymbol(Tokenizer, &Token, "/=", Token_DivideEquals) ||
		GetSymbol(Tokenizer, &Token, "%=", Token_ModuloEquals) ||
		GetSymbol(Tokenizer, &Token, "+=", Token_PlusEquals) ||
		GetSymbol(Tokenizer, &Token, "-=", Token_MinusEquals) ||
		GetSymbol(Tokenizer, &Token, "<<=", Token_DoubleLessThanEquals) ||
		GetSymbol(Tokenizer, &Token, ">>=", Token_DoubleGreaterThanEquals) ||
		GetSymbol(Tokenizer, &Token, "&=", Token_AmpersandEquals) ||
		GetSymbol(Tokenizer, &Token, "^=", Token_CaratEquals) ||
		GetSymbol(Tokenizer, &Token, "|=", Token_PipeEquals);

	}
	if(Token.Type != Token_Unknown) return(Token);

	{
		GetKeyword(Tokenizer, &Token) ||
		GetCharacter(Tokenizer, &Token) ||
		GetPreprocessorCommand(Tokenizer, &Token) ||
		GetComment(Tokenizer, &Token) ||
		GetString(Tokenizer, &Token) ||
		GetPrecisionNumber(Tokenizer, &Token) ||
		GetInteger(Tokenizer, &Token) ||
		GetIdentifier(Tokenizer, &Token);

	}
	if(Token.Type != Token_Unknown) return(Token);

	char C = Tokenizer->At[0];
	CopyToTokenAndAdvance(Tokenizer, &Token, 1, Token_Unknown);

	switch(C)
	{
		case '\0':{ Token.Type = Token_EndOfStream; } break;
		case '(': { Token.Type = Token_OpenParen; } break;
		case ')': { Token.Type = Token_CloseParen; } break;
		case ':': { Token.Type = Token_Colon; } break;
		case ';': { Token.Type = Token_SemiColon; } break;
		case '*': { Token.Type = Token_Asterisk; } break;
		case '[': { Token.Type = Token_OpenBracket; } break;
		case ']': { Token.Type = Token_CloseBracket; } break;
		case '{': { Token.Type = Token_OpenBrace; } break;
		case '}': { Token.Type = Token_CloseBrace; } break;
		case ',': { Token.Type = Token_Comma; } break;
		case '-': { Token.Type = Token_Dash; } break;
		case '+': { Token.Type = Token_Cross; } break;
		case '=': { Token.Type = Token_EqualSign; } break;
		case '^': { Token.Type = Token_Carat; } break;
		case '&': { Token.Type = Token_Ampersand; } break;
		case '%': { Token.Type = Token_PercentSign; } break;
		case '?': { Token.Type = Token_QuestionMark; } break;
		case '!': { Token.Type = Token_Bang; } break;
		case '/': { Token.Type = Token_Slash; } break;
		case '|': { Token.Type = Token_Pipe; } break;
		case '<': { Token.Type = Token_LessThan; } break;
		case '>': { Token.Type = Token_GreaterThan; } break;
		case '~': { Token.Type = Token_Tilde; } break;
		case '.': { Token.Type = Token_Dot; } break;
	}

	return(Token);
}

/*******************************************************************************
 * Recursive Descent Parsing Routines.
 *******************************************************************************/

bool
ParseConstant(struct tokenizer *Tokenizer)
{
	struct tokenizer PrevState = *Tokenizer;
	struct token Token = GetToken(Tokenizer);

	switch(Token.Type)
	{
		case Token_Integer:
		case Token_Character:
		case Token_PrecisionNumber:
/*	TODO:	case Token_Enumeration:*/
		{
			return(true);
		} break;
		default:
		{
			*Tokenizer = PrevState;
			return(false);
		}
	}
}

bool ParseAssignmentExpression(struct tokenizer *Tokenizer);


/* https://en.wikipedia.org/wiki/Left_recursion */
bool
ParseExpressionI(struct tokenizer *Tokenizer)
{
	struct tokenizer PrevState = *Tokenizer;

	if(ParseAssignmentExpression(Tokenizer) &&
	   ParseExpressionI(Tokenizer))
	{
		return(true);
	}

	*Tokenizer = PrevState;
	return(false);
}

/* https://en.wikipedia.org/wiki/Left_recursion */
/*
  expression:
          assignment-expression
	  expression , assignment-expression
*/
bool
ParseExpression(struct tokenizer *Tokenizer)
{
	struct tokenizer PrevState = *Tokenizer;
	if(ParseAssignmentExpression(Tokenizer) &&
	   ParseExpressionI(Tokenizer))
	{
		return(true);
	}

	*Tokenizer = PrevState;
	return(false);
}

bool
ParsePrimaryExpression(struct tokenizer *Tokenizer)
{
	struct tokenizer PrevState = *Tokenizer;
	struct token Token = GetToken(Tokenizer);

	switch(Token.Type)
	{
		case Token_Identifier:
		case Token_String:
		{
			return(true);
		} break;

		default:
		{
			/* Do nothing. Logic continues outside of Switch
			   statement below. */
			;
		} break;
	}

	/* We haven't had a match yet, so we need to reset Tokenizer->At in
	   order to see what we've got. */
	*Tokenizer = PrevState;
	if(ParseConstant(Tokenizer))
	{
		return(true);
	}

	*Tokenizer = PrevState;
	if(Token_OpenParen == GetToken(Tokenizer).Type &&
	   ParseExpression(Tokenizer) &&
	   Token_CloseParen == GetToken(Tokenizer).Type)
	{
		return true;
	}

	*Tokenizer = PrevState;
	return(false);
}

bool
ParseArgumentExpressionList(struct tokenizer *Tokenizer)
{
	struct tokenizer PrevState = *Tokenizer;
	if(ParseAssignmentExpression(Tokenizer))
	{
		return(true);
	}

	*Tokenizer = PrevState;
	if(ParseArgumentExpressionList(Tokenizer) &&
	   Token_Comma == GetToken(Tokenizer).Type &&
	   ParseAssignmentExpression(Tokenizer))
	{
		return(true);
	}

	*Tokenizer = PrevState;
	return(false);
}

bool
ParsePostfixExpressionI(struct tokenizer *Tokenizer)
{
	struct tokenizer PrevState = *Tokenizer;

	if(Token_OpenBracket == GetToken(Tokenizer).Type &&
	   ParseExpression(Tokenizer) &&
	   Token_CloseBracket == GetToken(Tokenizer).Type &&
	   ParsePostfixExpressionI(Tokenizer))
	{
		return(true);
	}

	*Tokenizer = PrevState;
	if(Token_OpenParen == GetToken(Tokenizer).Type &&
	   ParseArgumentExpressionList(Tokenizer) &&
	   Token_CloseParen == GetToken(Tokenizer).Type &&
	   ParsePostfixExpressionI(Tokenizer))
	{
		return(true);
	}

	*Tokenizer = PrevState;
	if(Token_OpenParen == GetToken(Tokenizer).Type &&
	   Token_CloseParen == GetToken(Tokenizer).Type &&
	   ParsePostfixExpressionI(Tokenizer))
	{
		return(true);
	}

	*Tokenizer = PrevState;
	if(Token_Dot == GetToken(Tokenizer).Type &&
	   Token_Identifier == GetToken(Tokenizer).Type &&
	   ParsePostfixExpressionI(Tokenizer))
	{
		return(true);
	}

	*Tokenizer = PrevState;
	if(Token_Arrow == GetToken(Tokenizer).Type &&
	   Token_Identifier == GetToken(Tokenizer).Type &&
	   ParsePostfixExpressionI(Tokenizer))
	{
		return(true);
	}

	*Tokenizer = PrevState;
	if(Token_PlusPlus == GetToken(Tokenizer).Type &&
	   ParsePostfixExpressionI(Tokenizer))
	{
		return(true);
	}

	*Tokenizer = PrevState;
	if(Token_MinusMinus == GetToken(Tokenizer).Type &&
	   ParsePostfixExpressionI(Tokenizer))
	{
		return(true);
	}

	*Tokenizer = PrevState;
	return(false);
}

/*
  postfix-expression:
          primary-expression
	  postfix-expression [ expression ]
	  postfix-expression . identifier
	  postfix-expression -> identifier
	  postfix-expression ++
	  postfix-expression --
*/
bool
ParsePostfixExpression(struct tokenizer *Tokenizer)
{
	struct tokenizer PrevState = *Tokenizer;

	if(ParsePrimaryExpression(Tokenizer) &&
	   ParsePostfixExpressionI(Tokenizer))
	{
		return(true);
	}

	*Tokenizer = PrevState;
	return(false);
}

bool
ParseUnaryOperator(struct tokenizer *Tokenizer)
{
	struct tokenizer PrevState = *Tokenizer;
	struct token Token = GetToken(Tokenizer);
	switch(Token.Type)
	{
		case Token_Ampersand:
		case Token_Asterisk:
		case Token_Cross:
		case Token_Dash:
		case Token_Tilde:
		case Token_Bang:
		{
			return(true);
		}
		default:
		{
			*Tokenizer = PrevState;
			return(false);
		}
	}
}

bool ParseUnaryExpression(struct tokenizer *Tokenizer);

bool
ParseTypeName(struct tokenizer *Tokenizer)
{
	/* TODO: Implement ParseTypeName */
	return(false);
}

bool
ParseCastExpression(struct tokenizer *Tokenizer)
{
	struct tokenizer PrevState = *Tokenizer;

	if(ParseUnaryExpression)
	{
		return(true);
	}

	*Tokenizer = PrevState;
	if(Token_OpenParen == GetToken(Tokenizer).Type &&
	   ParseTypeName(Tokenizer) &&
	   Token_CloseParen == GetToken(Tokenizer).Type)
	{
		return(true);
	}

	*Tokenizer = PrevState;
	return(false);
}

bool
ParseConditionalExpression(struct tokenizer *Tokenizer)
{
	/* TODO: Implement ParseConditionalExpression */
	return(false);
}

bool
ParseUnaryExpression(struct tokenizer *Tokenizer)
{
	struct tokenizer PrevState = *Tokenizer;
	struct token Token;

	if(ParsePostfixExpression(Tokenizer))
	{
		return(true);
	}

	*Tokenizer = PrevState;
	if(Token_PlusPlus == GetToken(Tokenizer).Type &&
	   ParseUnaryExpression(Tokenizer))
	{
		return(true);
	}

	*Tokenizer = PrevState;
	if(Token_MinusMinus == GetToken(Tokenizer).Type &&
	   ParseUnaryExpression(Tokenizer))
	{
		return(true);
	}

	*Tokenizer = PrevState;
	if(ParseUnaryOperator(Tokenizer) &&
	   ParseCastExpression(Tokenizer))
	{
		return(true);
	}

	*Tokenizer = PrevState;
	Token = GetToken(Tokenizer);
	if(Token_Keyword == Token.Type)
	{
		if(IsStringEqual("sizeof", Token.Text, StringLength("sizeof")))
		{
			struct tokenizer Previous = *Tokenizer;
			if(ParseUnaryExpression(Tokenizer))
			{
				return(true);
			}

			*Tokenizer = Previous;
			if(Token_OpenParen == GetToken(Tokenizer).Type &&
			   ParseTypeName(Tokenizer) &&
			   Token_CloseParen == GetToken(Tokenizer).Type)
			{
				return(true);
			}
		}
	}

	*Tokenizer = PrevState;
	return(false);
}

bool
ParseAssignmentOperator(struct tokenizer *Tokenizer)
{
	struct tokenizer PrevState = *Tokenizer;
	struct token Token = GetToken(Tokenizer);

	switch(Token.Type)
	{
		case Token_EqualSign:
		case Token_MultiplyEquals:
		case Token_DivideEquals:
		case Token_ModuloEquals:
		case Token_PlusEquals:
		case Token_MinusEquals:
		case Token_DoubleLessThanEquals:
		case Token_DoubleGreaterThanEquals:
		case Token_AmpersandEquals:
		case Token_CaratEquals:
		case Token_PipeEquals:
		{
			return(true);
		}

		default:
		{
			*Tokenizer = PrevState;
			return(false);
		}
	}
}

bool
ParseAssignmentExpression(struct tokenizer *Tokenizer)
{
	struct tokenizer PrevState = *Tokenizer;

	if(ParseConditionalExpression(Tokenizer))
	{
		return(true);
	}

	*Tokenizer = PrevState;
	if(ParseUnaryExpression(Tokenizer) &&
		ParseAssignmentOperator(Tokenizer) &&
		ParseAssignmentExpression(Tokenizer))
	{
		return(true);
	}

	*Tokenizer = PrevState;
	return(false);
}

/* TODO(AARON): HERE!

/*
  struct-declaration:
          specifier-qualifier-list struct-declarator-list ;
*/
bool
ParseStructDeclaration(struct tokenizer *Tokenizer)
{
	struct tokenizer Start = *Tokenizer;

	if(ParseSpecifierQualifierList(Tokenizer) &&
	   ParseStructDeclaratorList(Tokenizer) &&
	   Token_SemiColon == GetToken(Tokenizer).Type)
	{
		return(true);
	}

	*Tokenizer = Start;
	return(false);
}

/*
  init-declarator:
          declarator
	  declarator = initializer
*/
bool
ParseInitDeclarator(struct tokenizer *Tokenizer)
{
	struct tokenizer Start = *Tokenizer;

	if(ParseDeclarator(Tokenizer) &&
	   Token_EqualSign == GetToken(Tokenizer).Type &&
	   ParseInitialize(Tokenizer))
	{
		return(true);
	}

	*Tokenizer = Start;
	return(false);
}

bool
ParseInitDeclatorListI(struct tokenizer *Tokenizer)
{
	struct tokenizer Start = *Tokenizer;

	if(Token_Comma == GetToken(Tokenizer).Type &&
	   ParseInitDeclarator(Tokenizer))
	{
		return(true);
	}

	*Tokenizer = Start;
	return(false);
}

/*
  init-declarator-list:
          init-declarator
	  init-declarator-list , init-declarator
*/
bool
ParseInitDeclaratorList(struct tokenizer *Tokenizer)
{
	struct tokenizer Start = *Tokenizer;

	if(ParseInitDeclarator(Tokenizer) &&
	   ParseInitDeclaratorListI(Tokenizer))
	{
		return(true);
	}

	*Tokenizer = Start;
	return(false);
}

bool
ParseStructDeclarationListI(struct tokenizer *Tokenizer)
{
	struct tokenizer Start = *Tokenizer;

	if(ParseStructDeclaration(Tokenizer) &&
	   ParseStructDeclarationListI(Tokenizer))
	{
		return(false);
	}

	*Tokenizer = Start;
	return(false);
}

/*
  struct-declaration-list:
          struct-declaration
	  struct-declaration-list struct-declaration
*/
bool
ParseStructDeclarationList(struct tokenizer *Tokenizer)
{
	struct tokenizer Start = *Tokenizer;

	if(ParseStructDeclaration(Tokenizer) &&
	   ParseStructDeclarationListI(Tokenizer))
	{
		return(true);
	}

	*Tokenizer = Start;
	return(false);
}

/*
  One of: struct union
*/
bool
ParseStructOrUnion(struct tokenizer *Tokenizer)
{
	struct tokenizer Start = *Tokenizer;
	struct token Token = GetToken(Tokenizer);

	if(Token_Keyword == Token.Type &&
	   (IsStringEqual(Token.Text, "struct", Token.TextLength) ||
	    IsStringEqual(Token.Text, "union", Token.TextLength)))
	{
		return(true);
	}

	*Tokenizer = Start;
	return(false);
}

/*
  struct-or-union-specifier:
          struct-or-union identifier(opt) { struct-declaratio-list }
	  struct-or-union identifier
*/
bool
ParseStructOrUnionSpecifier(struct tokenizer *Tokenizer)
{
	struct tokenizer Start = *Tokenizer;

	if(ParseStructOrUnion(Tokenizer))
	{
		struct tokenizer Previous = *Tokenizer;
		if(Token_Identifier != GetToken(Tokenizer).Type)
		{
			*Tokenizer = Previous;
		}

		if(Token_OpenBrace == GetToken(Tokenizer).Type &&
		   ParseStructDeclarationList(Tokenizer) &&
		   Token_CloseBrace == GetToken(Tokenizer).Type)
		{
			return(true);
		}
	}

	*Tokenizer = Start;
	if(ParseStructOrUnion(Tokenizer) &&
	   Token_Identifier == GetToken(Tokenizer).Type)
	{
		return(true);
	}

	*Tokenizer = Start;
	return(false);
}

/*
  One of: const union
*/
bool
ParseTypeQualifier(struct tokenizer *Tokenizer)
{
	struct tokenizer Start = *Tokenizer;
	struct token Token = GetToken(Tokenizer);

	if(Token.Type != Token_Keyword)
	{
		*Tokenizer = Start;
		return(false);
	}

	if(IsStringEqual(Token.Text, "const", Token.TextLength) ||
	   IsStringEqual(Token.Text, "union", Token.TextLength))
	{
		return(true);
	}

	*Tokenizer = Start;
	return(false);
}

/*
  One of: void char short int long float double signed unsigned
          struct-or-union-specifier enum-specifier typedef-name
*/
bool
ParseTypeSpecifier(struct tokenizer *Tokenizer)
{
	struct tokenizer Start = *Tokenizer;
	char *Keywords[] = { "void", "char", "short", "int", "long", "float",
			     "double", "signed", "unsigned" };

	if(ParseStructOrUnionSpecifier(Tokenizer))
	{
		return(true);
	}

	*Tokenizer = Start;
	if(ParseEnumSpecifier(Tokenizer))
	{
		return(true);
	}

	*Tokenizer = Start;
	if(ParseTypedefName(Tokenizer))
	{
		return(true);
	}

	struct token Token = GetToken(Tokenizer);
	if(Token.Type != Token_Keyword)
	{
		*Tokenizer = Start;
		return(false);
	}

	for(int Index = 0; Index < ARRAY_SIZE(Keywords); Index++)
	{
		if(IsStringEqual(Token.Text, Keywords[Index], Token.TextLength))
		{
			return(true);
		}
	}

	*Tokenizer = Start;
	return(false);
}

/*
  One of: auto register static extern typedef
*/
bool
ParseStorageClassSpecifier(struct tokenizer *Tokenizer)
{
	struct tokenizer Start = *Tokenizer;
	struct token Token = GetToken(Tokenizer);

	if(!Token.Type == Token_Keyword)
	{
		*Tokenizer = Start;
		return(false);
	}

	char *Keywords[] = { "auto", "register", "static", "extern", "typedef" };
	for(int Index = 0; Index < ARRAY_SIZE(Keywords); Index++)
	{
		if(IsStringEqual(Token.Text, Keywords[Index], Token.TextLength))
		{
			return(true);
		}
	}

	*Tokenizer = Start;
	return(false);
}

/*
  declaration-specifiers:
          storage-class-specifier declaration-specifiers(opt)
	  type-specifier declaration-specifiers(opt)
	  type-qualifier declaration-specifiers(opt)
*/
bool
ParseDeclarationSpecifiers(struct tokenizer *Tokenizer)
{
	struct tokenizer Start = *Tokenizer;
	struct tokenizer Previous = *Tokenizer;

	if(ParseStorageClassSpecifier(Tokenizer))
	{
		Previous = *Tokenizer;
		if(!ParseDeclarationSpecifiers(Tokenizer))
		{
			*Tokenizer = Previous;
		}
		return(true);
	}
	*Tokenizer = Start;

	if(ParseTypeSpecifier(Tokenizer))
	{
		Previous = *Tokenizer;
		if(!ParseDeclarationSpecifiers(Tokenizer))
		{
			*Tokenizer = Previous;
		}
		return(true);
	}
	*Tokenizer = Start;

	if(ParseTypeQualifier(Tokenizer))
	{
		Previous = *Tokenizer;
		if(!ParseDeclarationSpecifiers(Tokenizer))
		{
			*Tokenizer = Previous;
		}
		return(true);
	}

	*Tokenizer = Start;
	return(false);
}

bool
ParseDeclarationListI(struct tokenizer *Tokenizer)
{
	struct tokenizer Start = *Tokenizer;

	if(ParseDeclaration(Tokenizer) &&
	   ParseDeclarationListI(Tokenizer))
	{
		return(true);
	}

	*Tokenizer = Start;
	return(false);
}

/*
  declaration-list:
          declaration
	  declaration-list declaration
*/
bool
ParseDeclarationList(struct tokenizer *Tokenizer)
{
	struct tokenizer Start = *Tokenizer;

	if(ParseDeclaration(Tokenizer) &&
	   ParseDeclarationListI(Tokenizer))
	{
		return(true);
	}

	*Tokenizer = Start;
	return(false);
}

/*
  declaration:
          declaration-specifiers init-declarator-list(opt) ;
*/
bool
ParseDeclaration(struct tokenizer *Tokenizer)
{
	struct tokenizer Start = *Tokenizer;
	if(!ParseDeclarationSpecifiers(Tokenizer))
	{
		*Tokenizer = Start;
		return(false);
	}

	struct tokenizer Previous = *Tokenizer;
	if(!ParseInitDeclaratorList(Tokenizer))
	{
		*Tokenizer = Previous;
	}

	struct token Token = GetToken(Tokenizer);
	if(!Token.Type == Token_SemiColon)
	{
		*Tokenizer = Previous;
		return(false);
	}

	return(true);
}

/*
  function-definition:
          declaration-specifiers(opt) declarator declaration-list(opt) compound-statement
*/
bool
ParseFunctionDefinition(struct tokenizer *Tokenizer)
{
	struct tokenizer StartState = *Tokenizer;
	struct tokenizer Previous = *Tokenizer;

	if(!ParseDeclarationSpecifiers(Tokenizer))
	{
		*Tokenizer = Previous;
	}
	Previous = *Tokenizer;

	if(!ParseDeclarator(Tokenizer))
	{
		*Tokenizer = StartState;
		return(false);
	}
	Previous = *Tokenizer;

	if(!ParseDeclarationList(Tokenizer))
	{
		*Tokenizer = Previous;
	}

	if(!ParseCompoundStatement(Tokenizer))
	{
		*Tokenizer = StartState;
		return(false);
	}

	return(true);
}

/*
  external-declaration:
          function-definition
	  declaration
*/
bool
ParseExternalDeclaration(struct tokenizer *Tokenizer)
{
	struct tokenizer PrevState = *Tokenizer;

	if(ParseFunctionDefinition(Tokenizer) ||
	   ParseDeclaration(Tokenizer))
	{
		return(true);
	}

	*Tokenizer = PrevState;
	return(false);
}

bool
ParseTranslationUnitI(struct tokenizer *Tokenizer)
{
	struct tokenizer PrevState = *Tokenizer;

	if(ParseExternalDeclaration(Tokenizer) &&
	   ParseTranslationUnitI(Tokenizer))
	{
		return(true);
	}

	*Tokenizer = PrevState;
	return(false);
}

/*
  translation-unit:
          external-declaration
	  translation-unit external-declaration
*/
bool
ParseTranslationUnit(struct tokenizer *Tokenizer)
{
	struct tokenizer PrevState = *Tokenizer;

	if(ParseExternalDeclaration(Tokenizer) &&
	   ParseTranslationUnitI(Tokenizer))
	{
		return(true);
	}

	*Tokenizer = PrevState;
	return(false);
}

/*******************************************************************************
 * Main entrypoints.
 *******************************************************************************/

void
Lex(struct buffer *FileContents)
{
	struct tokenizer Tokenizer;
	Tokenizer.Beginning = FileContents->Data;
	Tokenizer.At = FileContents->Data;

	bool Parsing = true;
	while(Parsing)
	{
		struct token Token = GetToken(&Tokenizer);
		switch(Token.Type)
		{
			case Token_EndOfStream: { Parsing = false; } break;
			case Token_Unknown:
			{
				printf("[%d,%d] Token Name: %20s, Token Text: %.*s (%.*s)\n",
				       Token.Line + 1,
				       Token.Column,
				       TokenName(Token.Type),
				       CastSizeIntTo32Bits(Token.TextLength), Token.Text,
				       CastSizeIntTo32Bits(Token.TextLength + 4), Token.Text - 2);
			} break;
			default:
			{
				printf("[%d,%d] Token Name: %20s, Token Text: %.*s\n",
				       Token.Line + 1,
				       Token.Column,
				       TokenName(Token.Type),
				       CastSizeIntTo32Bits(Token.TextLength),
				       Token.Text);
			} break;
		}
	}
}

void
Parse(struct buffer *FileContents)
{
	struct tokenizer Tokenizer, PrevState;
	Tokenizer.Beginning = FileContents->Data;
	Tokenizer.At = FileContents->Data;

	bool Parsing = true;
	while(Parsing)
	{
		PrevState = Tokenizer;
		struct token Token = GetToken(&Tokenizer);
		switch(Token.Type)
		{
			case Token_EndOfStream: { Parsing = false; } break;
			case Token_Unknown: { Parsing = false; } break;
			case Token_Comment: { } break;
			case Token_PreprocessorCommand: { } break;
			default:
			{
				Tokenizer = PrevState;
				ParseTranslationUnit(&Tokenizer);
			} break;
		}
	}
}

/******************************************************************************
 * Main entrypoint and related functions.
 ******************************************************************************/

void
AbortWithMessage(const char *msg)
{
	fprintf(stderr, "%s\n", msg);
	exit(EXIT_FAILURE);
}

void
Usage()
{
	printf("Usage: program operation file\n");
	printf("  operation: One of: [parse, lex].\n");
	printf("  file: Must be a file in this directory.\n");
	printf("  Specify '-h' or '--help' for this help text.\n");
	exit(EXIT_SUCCESS);
}

int
main(int ArgCount, char **Args)
{
	for(int Index = 0; Index < ArgCount; ++Index)
	{
		if(IsStringEqual(Args[Index], "-h", StringLength("-h")) ||
		   IsStringEqual(Args[Index], "--help", StringLength("--help")))
		{
			Usage();
		}
	}
	if(ArgCount != 3) Usage();

	if(!IsStringEqual(Args[1], "parse", StringLength("parse")) &&
	   !IsStringEqual(Args[1], "lex", StringLength("lex")))
	{
		Usage();
	}

	size_t AllocSize = FileSize(Args[2]);
	struct buffer FileContents;

	/* Allocate space on the stack. */
	BufferSet(&FileContents, (char *)alloca(AllocSize), 0, AllocSize);

	if(!CopyFileIntoBuffer(Args[2], &FileContents))
	{
		AbortWithMessage("Couldn't copy entire file to buffer");
	}

	if(IsStringEqual(Args[1], "parse", StringLength("parse")))
	{
		Parse(&FileContents);
	}
	else
	{
		Lex(&FileContents);
	}

	return(EXIT_SUCCESS);
}
