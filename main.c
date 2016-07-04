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
	Token_Hash,		/*  # */
	Token_Bang,
	Token_Pipe,
	Token_LessThan,
	Token_GreaterThan,
	Token_Tilde,

	Token_NotEqual,
	Token_GreaterThanEqual,
	Token_LessThanEqual,
	Token_LogicalEqual,
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
	Token_Ellipsis,		/* ... */

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
		case Token_Hash: { return "Hash"; } break;
		case Token_Bang: { return "Bang"; } break;
		case Token_Pipe: { return "Pipe"; } break;
		case Token_LessThan: { return "LessThan"; } break;
		case Token_GreaterThan: { return "GreaterThan"; } break;
		case Token_Tilde: { return "Tilde"; } break;

		case Token_NotEqual: { return "NotEqual"; } break;
		case Token_GreaterThanEqual: { return "GreaterThanEqual"; } break;
		case Token_LessThanEqual: { return "LessThanEqual"; } break;
		case Token_LogicalEqual: { return "LogicalEqual"; } break;
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
		case Token_Ellipsis: { return "Ellipsis"; } break;

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
		GetSymbol(Tokenizer, &Token, "==", Token_LogicalEqual) ||
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
		GetSymbol(Tokenizer, &Token, "|=", Token_PipeEquals) ||
		GetSymbol(Tokenizer, &Token, "...", Token_Ellipsis);

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
		case '#': { Token.Type = Token_Hash; } break;
	}

	return(Token);
}

/*******************************************************************************
 * Recursive Descent Parsing Routines.
 *******************************************************************************/

/*
  constant:
          integer-constant
          character-constant
          floating-constant
          enumeration-constant
*/
bool
ParseConstant(struct tokenizer *Tokenizer)
{
	struct tokenizer Start = *Tokenizer;
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
			*Tokenizer = Start;
			return(false);
		}
	}
}

bool ParseAssignmentExpression(struct tokenizer *Tokenizer);

bool
ParseArgumentExpressionListI(struct tokenizer *Tokenizer)
{
        struct tokenizer Start = *Tokenizer;

        if(Token_Comma == GetToken(Tokenizer).Type &&
           ParseAssignmentExpression(Tokenizer) &&
           ParseArgumentExpressionListI(Tokenizer))
        {
                return(true);
        }

        *Tokenizer = Start;
        return(true);
}

/*
  argument-expression-list:
          assignment-expression
          argument-expression-list , assignment-expression
*/
bool
ParseArgumentExpressionList(struct tokenizer *Tokenizer)
{
	struct tokenizer Start = *Tokenizer;
	if(ParseAssignmentExpression(Tokenizer) &&
           ParseArgumentExpressionListI(Tokenizer))
	{
		return(true);
	}

	*Tokenizer = Start;
	return(false);
}

bool ParseExpression(struct tokenizer *Tokenizer);

/*
  primary-expression:
          identifier
          constant
          string
          ( expression )
*/
bool
ParsePrimaryExpression(struct tokenizer *Tokenizer)
{
	struct tokenizer Start = *Tokenizer;

	if(Token_Identifier == GetToken(Tokenizer).Type) return(true);

	*Tokenizer = Start;
	if(ParseConstant(Tokenizer)) return(true);

	*Tokenizer = Start;
	if(Token_String == GetToken(Tokenizer).Type) return(true);

	*Tokenizer = Start;
	if(Token_OpenParen == GetToken(Tokenizer).Type &&
	   ParseExpression(Tokenizer) &&
	   Token_CloseParen == GetToken(Tokenizer).Type)
	{
		return true;
	}

	*Tokenizer = Start;
	return(false);
}

bool
ParsePostfixExpressionI(struct tokenizer *Tokenizer)
{
	struct tokenizer Start = *Tokenizer;

	if(Token_OpenBracket == GetToken(Tokenizer).Type &&
	   ParseExpression(Tokenizer) &&
	   Token_CloseBracket == GetToken(Tokenizer).Type &&
	   ParsePostfixExpressionI(Tokenizer))
	{
		return(true);
	}

	*Tokenizer = Start;
	if(Token_OpenParen == GetToken(Tokenizer).Type &&
	   ParseArgumentExpressionList(Tokenizer) &&
	   Token_CloseParen == GetToken(Tokenizer).Type &&
	   ParsePostfixExpressionI(Tokenizer))
	{
		return(true);
	}

	*Tokenizer = Start;
	if(Token_OpenParen == GetToken(Tokenizer).Type &&
	   Token_CloseParen == GetToken(Tokenizer).Type &&
	   ParsePostfixExpressionI(Tokenizer))
	{
		return(true);
	}

	*Tokenizer = Start;
	if(Token_Dot == GetToken(Tokenizer).Type &&
	   Token_Identifier == GetToken(Tokenizer).Type &&
	   ParsePostfixExpressionI(Tokenizer))
	{
		return(true);
	}

	*Tokenizer = Start;
	if(Token_Arrow == GetToken(Tokenizer).Type &&
	   Token_Identifier == GetToken(Tokenizer).Type &&
	   ParsePostfixExpressionI(Tokenizer))
	{
		return(true);
	}

	*Tokenizer = Start;
	if(Token_PlusPlus == GetToken(Tokenizer).Type &&
	   ParsePostfixExpressionI(Tokenizer))
	{
		return(true);
	}

	*Tokenizer = Start;
	if(Token_MinusMinus == GetToken(Tokenizer).Type &&
	   ParsePostfixExpressionI(Tokenizer))
	{
		return(true);
	}

	*Tokenizer = Start;
	return(true);
}

/*
  postfix-expression:
          primary-expression
          postfix-expression [ expression ]
          postfix-expression ( argument-expression-list(opt) )
          postfix-expression . identifier
          postfix-expression -> identifier
          postfix-expression ++
          postfix-expression --
*/
bool
ParsePostfixExpression(struct tokenizer *Tokenizer)
{
	struct tokenizer Start = *Tokenizer;

	if(ParsePrimaryExpression(Tokenizer) &&
	   ParsePostfixExpressionI(Tokenizer))
	{
		return(true);
	}

	*Tokenizer = Start;
	return(false);
}

bool
ParseUnaryOperator(struct tokenizer *Tokenizer)
{
	struct tokenizer Start = *Tokenizer;
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
			*Tokenizer = Start;
			return(false);
		}
	}
}

bool ParseTypeName(struct tokenizer *Tokenizer);
bool ParseCastExpression(struct tokenizer *Tokenizer);

/*
  unary-expression:
          postfix-expression
          ++ unary-expression
          -- unary-expression
          unary-operator cast-expression
          sizeof unary-expression
          sizeof ( type-name )
*/
bool
ParseUnaryExpression(struct tokenizer *Tokenizer)
{
	struct tokenizer Start = *Tokenizer;
	struct token Token;

	if(ParsePostfixExpression(Tokenizer))
	{
		return(true);
	}

	*Tokenizer = Start;
	if(Token_PlusPlus == GetToken(Tokenizer).Type &&
	   ParseUnaryExpression(Tokenizer))
	{
		return(true);
	}

	*Tokenizer = Start;
	if(Token_MinusMinus == GetToken(Tokenizer).Type &&
	   ParseUnaryExpression(Tokenizer))
	{
		return(true);
	}

	*Tokenizer = Start;
	if(ParseUnaryOperator(Tokenizer) &&
	   ParseCastExpression(Tokenizer))
	{
		return(true);
	}

	*Tokenizer = Start;
	Token = GetToken(Tokenizer);
	if(Token_Keyword == Token.Type &&
           IsStringEqual("sizeof", Token.Text, StringLength("sizeof")))
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

	*Tokenizer = Start;
	return(false);
}

/*
  cast-expression:
          unary-expression
          ( type-name ) cast-expression
*/
bool
ParseCastExpression(struct tokenizer *Tokenizer)
{
	struct tokenizer Start = *Tokenizer;

	if(ParseUnaryExpression)
	{
		return(true);
	}

	*Tokenizer = Start;
	if(Token_OpenParen == GetToken(Tokenizer).Type &&
	   ParseTypeName(Tokenizer) &&
	   Token_CloseParen == GetToken(Tokenizer).Type)
	{
		return(true);
	}

	*Tokenizer = Start;
	return(false);
}

bool
ParseMultiplicativeExpressionI(struct tokenizer *Tokenizer)
{
        struct tokenizer Start = *Tokenizer;

        if(Token_Asterisk == GetToken(Tokenizer).Type &&
           ParseCastExpression(Tokenizer) &&
           ParseMultiplicativeExpressionI(Tokenizer))
        {
                return(true);
        }

        *Tokenizer = Start;
        if(Token_Slash == GetToken(Tokenizer).Type &&
           ParseCastExpression(Tokenizer) &&
           ParseMultiplicativeExpressionI(Tokenizer))
        {
                return(true);
        }

        *Tokenizer = Start;
        if(Token_PercentSign == GetToken(Tokenizer).Type &&
           ParseCastExpression(Tokenizer) &&
           ParseMultiplicativeExpressionI(Tokenizer))
        {
                return(true);
        }

        *Tokenizer = Start;
        return(true);
}

/*
  multiplicative-expression:
          cast-expression
          multiplicative-expression * cast-expression
          multiplicative-expression / cast-expression
          multiplicative-expression % cast-expression
*/
bool
ParseMultiplicativeExpression(struct tokenizer *Tokenizer)
{
        struct tokenizer Start = *Tokenizer;

        if(ParseCastExpression(Tokenizer) &&
           ParseMultiplicativeExpressionI(Tokenizer))
        {
                return(true);
        }

        *Tokenizer = Start;
        return(false);
}

bool
ParseAdditiveExpressionI(struct tokenizer *Tokenizer)
{
        struct tokenizer Start = *Tokenizer;

        if(Token_Cross == GetToken(Tokenizer).Type &&
           ParseMultiplicativeExpression(Tokenizer) &&
           ParseAdditiveExpressionI(Tokenizer))
        {
                return(true);
        }

        *Tokenizer = Start;
        if(Token_Dash == GetToken(Tokenizer).Type &&
           ParseMultiplicativeExpression(Tokenizer) &&
           ParseAdditiveExpressionI(Tokenizer))
        {
                return(true);
        }

        *Tokenizer = Start;
        return(true);
}

/*
  additive-expression:
          multiplicative-expression
          additive-expression + multiplicative-expression
          additive-expression - multiplicative-expression
*/
bool
ParseAdditiveExpression(struct tokenizer *Tokenizer)
{
        struct tokenizer Start = *Tokenizer;

        if(ParseMultiplicativeExpression(Tokenizer) &&
           ParseAdditiveExpressionI(Tokenizer))
        {
                return(true);
        }

        *Tokenizer = Start;
        return(false);
}

bool
ParseShiftExpressionI(struct tokenizer *Tokenizer)
{
        struct tokenizer Start = *Tokenizer;

        if(Token_BitShiftLeft == GetToken(Tokenizer).Type &&
           ParseAdditiveExpression(Tokenizer) &&
           ParseShiftExpressionI(Tokenizer))
        {
                return(true);
        }

        *Tokenizer = Start;
        if(Token_BitShiftRight == GetToken(Tokenizer).Type &&
           ParseAdditiveExpression(Tokenizer) &&
           ParseShiftExpressionI(Tokenizer))
        {
                return(true);
        }

        *Tokenizer = Start;
        return(true);
}

/*
  shift-expression:
          additive-expression
          shift-expression << additive-expression
          shift-expression >> additive-expression
*/
bool
ParseShiftExpression(struct tokenizer *Tokenizer)
{
        struct tokenizer Start = *Tokenizer;

        if(ParseAdditiveExpression(Tokenizer) &&
           ParseShiftExpressionI(Tokenizer))
        {
                return(true);
        }

        *Tokenizer = Start;
        return(false);
}

bool
ParseRelationalExpressionI(struct tokenizer *Tokenizer)
{
        struct tokenizer Start = *Tokenizer;

        if(Token_LessThan == GetToken(Tokenizer).Type &&
           ParseShiftExpression(Tokenizer) &&
           ParseRelationalExpressionI(Tokenizer))
        {
                return(true);
        }

        *Tokenizer = Start;
        if(Token_GreaterThan == GetToken(Tokenizer).Type &&
           ParseShiftExpression(Tokenizer) &&
           ParseRelationalExpressionI(Tokenizer))
        {
                return(true);
        }

        *Tokenizer = Start;
        if(Token_LessThanEqual == GetToken(Tokenizer).Type &&
           ParseShiftExpression(Tokenizer) &&
           ParseRelationalExpressionI(Tokenizer))
        {
                return(true);
        }

        *Tokenizer = Start;
        if(Token_GreaterThanEqual == GetToken(Tokenizer).Type &&
           ParseShiftExpression(Tokenizer) &&
           ParseRelationalExpressionI(Tokenizer))
        {
                return(true);
        }

        *Tokenizer = Start;
        return(true);
}

/*
  relational-expression:
          shift-expression
          relational-expression < shift-expression
          relational-expression > shift-expression
          relational-expression <= shift-exression
          relational-expression >= shift-expression
*/
bool
ParseRelationalExpression(struct tokenizer *Tokenizer)
{
        struct tokenizer Start = *Tokenizer;

        if(ParseShiftExpression(Tokenizer) &&
           ParseRelationalExpressionI(Tokenizer))
        {
                return(true);
        }

        *Tokenizer = Start;
        return(false);
}

bool
ParseEqualityExpressionI(struct tokenizer *Tokenizer)
{
        struct tokenizer Start = *Tokenizer;

        if(Token_LogicalEqual == GetToken(Tokenizer).Type &&
           ParseRelationalExpression(Tokenizer) &&
           ParseEqualityExpressionI(Tokenizer))
        {
                return(true);
        }

        *Tokenizer = Start;
        if(Token_NotEqual == GetToken(Tokenizer).Type &&
           ParseRelationalExpression(Tokenizer) &&
           ParseEqualityExpressionI(Tokenizer))
        {
                return(true);
        }

        *Tokenizer = Start;
        return(true);
}

/*
  equality-expression:
          relational-expression
          equality-expression == relational-expression
          equality-expression != relational-expression
*/
bool
ParseEqualityExpression(struct tokenizer *Tokenizer)
{
        struct tokenizer Start = *Tokenizer;

        if(ParseRelationalExpression(Tokenizer) &&
           ParseRelationalExpressionI(Tokenizer))
        {
                return(true);
        }

        *Tokenizer = Start;
        return(false);
}

bool
ParseAndExpressionI(struct tokenizer *Tokenizer)
{
        struct tokenizer Start = *Tokenizer;

        if(Token_Ampersand == GetToken(Tokenizer).Type &&
           ParseEqualityExpression(Tokenizer) &&
           ParseAndExpressionI(Tokenizer))
        {
                return(true);
        }

        *Tokenizer = Start;
        return(true);
}

/*
  AND-expression:
          equality-expression
          AND-expression & equality-expression
*/
bool
ParseAndExpression(struct tokenizer *Tokenizer)
{
        struct tokenizer Start = *Tokenizer;

        if(ParseEqualityExpression(Tokenizer) &&
           ParseAndExpressionI(Tokenizer))
        {
                return(true);
        }

        *Tokenizer = Start;
        return(false);
}

bool
ParseExclusiveOrExpressionI(struct tokenizer *Tokenizer)
{
        struct tokenizer Start = *Tokenizer;

        if(Token_Carat == GetToken(Tokenizer).Type &&
           ParseAndExpression(Tokenizer) &&
           ParseExclusiveOrExpressionI(Tokenizer))
        {
                return(true);
        }

        *Tokenizer = Start;
        return(true);
}

/*
  exclusive-OR-expression
          AND-expression
          exclusive-OR-expression ^ AND-expression
 */
bool
ParseExclusiveOrExpression(struct tokenizer *Tokenizer)
{
        struct tokenizer Start = *Tokenizer;

        if(ParseAndExpression(Tokenizer) &&
           ParseExclusiveOrExpressionI(Tokenizer))
        {
                return(true);
        }

        *Tokenizer = Start;
        return(false);
}

bool
ParseInclusiveOrExpressionI(struct tokenizer *Tokenizer)
{
        struct tokenizer Start = *Tokenizer;

        if(Token_Pipe == GetToken(Tokenizer).Type &&
           ParseExclusiveOrExpression(Tokenizer) &&
           ParseInclusiveOrExpressionI(Tokenizer))
        {
                return(true);
        }

        *Tokenizer = Start;
        return(true);
}

/*
  inclusive-OR-expression:
          exclusive-OR-expression
          inclusive-OR-expression | exclusive-OR-expression
*/
bool
ParseInclusiveOrExpression(struct tokenizer *Tokenizer)
{
        struct tokenizer Start = *Tokenizer;

        if(ParseExclusiveOrExpression(Tokenizer) &&
           ParseInclusiveOrExpressionI(Tokenizer))
        {
                return(true);
        }

        *Tokenizer = Start;
        return(false);
}

bool
ParseLogicalAndExpressionI(struct tokenizer *Tokenizer)
{
        struct tokenizer Start = *Tokenizer;

        if(Token_LogicalAnd == GetToken(Tokenizer).Type &&
           ParseInclusiveOrExpression(Tokenizer) &&
           ParseLogicalAndExpressionI(Tokenizer))
        {
                return(true);
        }

        *Tokenizer = Start;
        return(true);
}

/*
  logical-AND-expression:
          inclusive-OR-expression
          logical-AND-expression && inclusive-OR-expression
*/
bool
ParseLogicalAndExpression(struct tokenizer *Tokenizer)
{
        struct tokenizer Start = *Tokenizer;

        if(ParseInclusiveOrExpression(Tokenizer) &&
           ParseLogicalAndExpressionI(Tokenizer))
        {
                return(true);
        }

        *Tokenizer = Start;
        return(false);
}

bool
ParseLogicalOrExpressionI(struct tokenizer *Tokenizer)
{
	struct tokenizer Start = *Tokenizer;

	if(Token_LogicalOr == GetToken(Tokenizer).Type &&
	   ParseLogicalAndExpression(Tokenizer) &&
	   ParseLogicalOrExpressionI(Tokenizer))
	{
		return(true);
	}

	*Tokenizer = Start;
	return(true);
}

/*
  logical-OR-expression:
          logical-AND-expression
          logical-OR-expression || logical-AND-expression
*/
bool
ParseLogicalOrExpression(struct tokenizer *Tokenizer)
{
	struct tokenizer Start = *Tokenizer;

	if(ParseLogicalAndExpression(Tokenizer) &&
	   ParseLogicalOrExpressionI(Tokenizer))
	{
		return(true);
	}

	*Tokenizer = Start;
	return(false);
}

bool ParseConditionalExpression(struct tokenizer *Tokenizer);

/*
  constant-expression:
          conditional-expression
*/
bool
ParseConstantExpression(struct tokenizer *Tokenizer)
{
	struct tokenizer Start = *Tokenizer;
	if(ParseConditionalExpression(Tokenizer))
	{
		return(true);
	}

	*Tokenizer = Start;
	return(false);
}

/*
  conditional-expression:
          logical-OR-expression
          logical-OR-expression ? expression : conditional-expression
*/
bool
ParseConditionalExpression(struct tokenizer *Tokenizer)
{
	struct tokenizer Start = *Tokenizer;

	if(ParseLogicalOrExpression(Tokenizer) &&
	   Token_QuestionMark == GetToken(Tokenizer).Type &&
	   ParseExpression(Tokenizer) &&
	   Token_Colon == GetToken(Tokenizer).Type &&
	   ParseConditionalExpression(Tokenizer))
	{
		return(true);
	}

	*Tokenizer = Start;
	if(ParseLogicalOrExpression(Tokenizer))
	{
		return(true);
	}

	*Tokenizer = Start;
	return(false);
}

/*
  assignment-operator:
  one of: = *= /= %= += -= <<= >>= &= ^= |=
*/
bool
ParseAssignmentOperator(struct tokenizer *Tokenizer)
{
	struct tokenizer Start = *Tokenizer;
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
			*Tokenizer = Start;
			return(false);
		}
	}
}

/*
  assignment-expression:
          conditional-expression
          unary-expression assignment-operator assignment-expression
*/
bool
ParseAssignmentExpression(struct tokenizer *Tokenizer)
{
	struct tokenizer Start = *Tokenizer;

	if(ParseConditionalExpression(Tokenizer))
	{
		return(true);
	}

	*Tokenizer = Start;
	if(ParseUnaryExpression(Tokenizer) &&
	   ParseAssignmentOperator(Tokenizer) &&
	   ParseAssignmentExpression(Tokenizer))
	{
		return(true);
	}

	*Tokenizer = Start;
	return(false);
}

bool
ParseExpressionI(struct tokenizer *Tokenizer)
{
	struct tokenizer Start = *Tokenizer;

	if(Token_Comma == GetToken(Tokenizer).Type &&
	   ParseAssignmentExpression(Tokenizer) &&
	   ParseExpressionI(Tokenizer))
	{
		return(true);
	}

	*Tokenizer = Start;
	return(true);
}

/*
  expression:
          assignment-expression
          expression , assignment-expression
*/
bool
ParseExpression(struct tokenizer *Tokenizer)
{
	struct tokenizer Start = *Tokenizer;

	if(ParseAssignmentExpression(Tokenizer) &&
	   ParseExpressionI(Tokenizer))
	{
		return(true);
	}

	*Tokenizer = Start;
	return(false);
}

bool
ParseIdentifier(struct tokenizer *Tokenizer)
{
        struct tokenizer Start = *Tokenizer;

        if(Token_Identifier == GetToken(Tokenizer).Type)
        {
                return(true);
        }

        *Tokenizer = Start;
        return(false);
}

/*
  jump-statement:
          goto identifier ;
          continue ;
          break ;
          return expression(opt) ;
*/
bool
ParseJumpStatement(struct tokenizer *Tokenizer)
{
	struct tokenizer Start = *Tokenizer;
	struct token Token = GetToken(Tokenizer);
	struct tokenizer AtToken = *Tokenizer;

	if(Token_Keyword == Token.Type &&
	   IsStringEqual("goto", Token.Text, Token.TextLength) &&
	   ParseIdentifier(Tokenizer) &&
	   Token_SemiColon == GetToken(Tokenizer).Type)
	{
		return(true);
	}

	*Tokenizer = AtToken;
	if(Token_Keyword == Token.Type &&
	   IsStringEqual("continue", Token.Text, Token.TextLength) &&
	   Token_SemiColon == GetToken(Tokenizer).Type)
	{
		return(true);
	}

	*Tokenizer = AtToken;
	if(Token_Keyword == Token.Type &&
	   IsStringEqual("break", Token.Text, Token.TextLength) &&
	   Token_SemiColon == GetToken(Tokenizer).Type)
	{
		return(true);
	}

	*Tokenizer = AtToken;
	if(Token_Keyword == Token.Type &&
	   IsStringEqual("return", Token.Text, Token.TextLength))
	{
		struct tokenizer Previous = *Tokenizer;
		if(!ParseExpression(Tokenizer))
			*Tokenizer = Previous;

		if(Token_SemiColon == GetToken(Tokenizer).Type)
		{
			return(true);
		}
	}

	*Tokenizer = Start;
	return(false);
}

bool ParseStatement(struct tokenizer *Tokenizer);

/*
  iteration-statement:
          while ( expression ) statement
          do statement while ( expression) ;
          for ( expression(opt) ; expression(opt) ; expression(opt) ) statement
*/
bool
ParseIterationStatement(struct tokenizer *Tokenizer)
{
	struct tokenizer Start = *Tokenizer;
	struct token Token = GetToken(Tokenizer);
	struct tokenizer AtToken = *Tokenizer;

	if(Token_Keyword == Token.Type &&
	   IsStringEqual("while", Token.Text, Token.TextLength) &&
	   Token_OpenParen == GetToken(Tokenizer).Type &&
	   ParseExpression(Tokenizer) &&
	   Token_CloseParen == GetToken(Tokenizer).Type &&
	   ParseStatement(Tokenizer))
	{
		return(true);
	}

	*Tokenizer = AtToken;
	if(Token_Keyword == Token.Type &&
	   IsStringEqual("do", Token.Text, Token.TextLength) &&
	   ParseStatement(Tokenizer))
	{
		struct token Token = GetToken(Tokenizer);
		if(Token_Keyword == Token.Type &&
		   IsStringEqual("while", Token.Text, Token.TextLength) &&
		   Token_OpenParen == GetToken(Tokenizer).Type &&
		   ParseExpression(Tokenizer) &&
		   Token_CloseParen == GetToken(Tokenizer).Type &&
		   Token_SemiColon == GetToken(Tokenizer).Type)
		{
			return(true);
		}
	}

	*Tokenizer = AtToken;
	if(Token_Keyword == Token.Type &&
	   IsStringEqual("for", Token.Text, Token.TextLength) &&
	   Token_OpenParen == GetToken(Tokenizer).Type)
	{
		for(int I = 0; I < 3; I++)
		{
			struct tokenizer Previous = *Tokenizer;

			if(!ParseExpression(Tokenizer))
				*Tokenizer = Previous;

			if(Token_SemiColon != GetToken(Tokenizer).Type)
			{
				*Tokenizer = Start;
				return(false);
			}

		}

		if(Token_CloseParen == GetToken(Tokenizer).Type &&
		   ParseStatement(Tokenizer))
		{
			return(true);
		}
	}

	*Tokenizer = Start;
	return(false);
}

/*
  selection-statement:
          if ( expression ) statement
          if ( expression ) statement else statement
          switch ( expression ) statement
*/
bool
ParseSelectionStatement(struct tokenizer *Tokenizer)
{
	struct tokenizer Start = *Tokenizer;
	struct token Token = GetToken(Tokenizer);
	struct tokenizer AtToken = *Tokenizer;

	if(Token_Keyword == Token.Type &&
	   IsStringEqual("if", Token.Text, Token.TextLength) &&
	   Token_OpenParen == GetToken(Tokenizer).Type &&
	   ParseExpression(Tokenizer) &&
	   Token_CloseParen == GetToken(Tokenizer).Type &&
	   ParseStatement(Tokenizer))
	{
		struct tokenizer AtElse = *Tokenizer;
		struct token Token = GetToken(Tokenizer);

		if(Token_Keyword == Token.Type &&
		   IsStringEqual("else", Token.Text, Token.TextLength) &&
		   ParseStatement(Tokenizer))
		{
			return(true);
		}

		*Tokenizer = AtElse;
		return(true);
	}

	*Tokenizer = AtToken;
	if(Token_Keyword == Token.Type &&
	   IsStringEqual("switch", Token.Text, Token.TextLength) &&
	   Token_OpenParen == GetToken(Tokenizer).Type &&
	   ParseExpression(Tokenizer) &&
	   Token_CloseParen == GetToken(Tokenizer).Type &&
	   ParseStatement(Tokenizer))
	{
		return(true);
	}

	*Tokenizer = Start;
	return(false);
}

bool
ParseStatementListI(struct tokenizer *Tokenizer)
{
	struct tokenizer Start = *Tokenizer;

	if(ParseStatement(Tokenizer) &&
	   ParseStatementListI(Tokenizer))
	{
		return(true);
	}

	*Tokenizer = Start;
	return(true);
}

/*
  statement-list:
          statement
          statement-list statement
*/
bool
ParseStatementList(struct tokenizer *Tokenizer)
{
	struct tokenizer Start = *Tokenizer;

	if(ParseStatement(Tokenizer) &&
	   ParseStatementListI(Tokenizer))
	{
		return(true);
	}

	*Tokenizer = Start;
	return(false);
}

bool ParseDeclarationList(struct tokenizer *Tokenizer);

/*
  compound-statement:
          { declaration-list(opt) statement-list(opt) }
*/
bool
ParseCompoundStatement(struct tokenizer *Tokenizer)
{
	struct tokenizer Start = *Tokenizer;

	if(Token_OpenBrace == GetToken(Tokenizer).Type)
	{
		struct tokenizer Previous = *Tokenizer;
		if(!ParseDeclarationList(Tokenizer))
		{
			*Tokenizer = Previous;
		}

		Previous = *Tokenizer;
		if(!ParseStatementList(Tokenizer))
		{
			*Tokenizer = Previous;
		}

		if(Token_CloseBrace == GetToken(Tokenizer).Type)
		{
			return(true);
		}
	}

	*Tokenizer = Start;
	return(false);
}

/*
  expression-statement:
          expression(opt) ;
*/
bool
ParseExpressionStatement(struct tokenizer *Tokenizer)
{
	struct tokenizer Start = *Tokenizer;

	if(ParseExpression(Tokenizer) &&
	   Token_SemiColon == GetToken(Tokenizer).Type)
	{
		return(true);
	}

	*Tokenizer = Start;
	if(Token_SemiColon == GetToken(Tokenizer).Type)
	{
		return(true);
	}

	*Tokenizer = Start;
	return(false);
}

/*
  labeled-statement:
          identifier : statement
          case constant-expression : statement
          default : statement
*/
bool
ParseLabeledStatement(struct tokenizer *Tokenizer)
{
	struct tokenizer Start = *Tokenizer;
	struct token Token = GetToken(Tokenizer);
	struct tokenizer AtToken = *Tokenizer;

	if(Token_Keyword == Token.Type &&
	   IsStringEqual("identifier", Token.Text, Token.TextLength) &&
	   Token_Colon == GetToken(Tokenizer).Type &&
	   ParseStatement(Tokenizer))
	{
		return(true);
	}

	*Tokenizer = AtToken;
	if(Token_Keyword == Token.Type &&
	   IsStringEqual("case", Token.Text, Token.TextLength) &&
	   ParseConstantExpression(Tokenizer) &&
	   Token_Colon == GetToken(Tokenizer).Type &&
	   ParseStatement(Tokenizer))
	{
		return(true);
	}

	*Tokenizer = AtToken;
	if(Token_Keyword == Token.Type &&
	   IsStringEqual("default", Token.Text, Token.TextLength) &&
	   Token_Colon == GetToken(Tokenizer).Type &&
	   ParseStatement(Tokenizer))
	{
		return(true);
	}

	*Tokenizer = Start;
	return(false);
}

/*
  statement:
  labeled-statement
  expression-statement
  compound-statement
  selection-statement
  iteration-statement
  jump-statement
*/
bool
ParseStatement(struct tokenizer *Tokenizer)
{
	struct tokenizer Start = *Tokenizer;

	if(ParseLabeledStatement(Tokenizer)) return(true);

	*Tokenizer = Start;
	if(ParseExpressionStatement(Tokenizer)) return(true);

	*Tokenizer = Start;
	if(ParseCompoundStatement(Tokenizer)) return(true);

	*Tokenizer = Start;
	if(ParseSelectionStatement(Tokenizer)) return(true);

	*Tokenizer = Start;
	if(ParseIterationStatement(Tokenizer)) return(true);

	*Tokenizer = Start;
	if(ParseJumpStatement(Tokenizer)) return(true);

	*Tokenizer = Start;
	return(false);
}

/*
  typedef-name:
  identifier
*/
bool
ParseTypedefName(struct tokenizer *Tokenizer)
{
	struct tokenizer Start = *Tokenizer;

	if(ParseIdentifier(Tokenizer))
	{
		return(true);
	}

	*Tokenizer = Start;
	return(false);
}

bool ParseParameterTypeList(struct tokenizer *Tokenizer);

bool
ParseDirectAbstractDeclaratorI(struct tokenizer *Tokenizer)
{
	struct tokenizer Start = *Tokenizer;

	if(Token_OpenBracket == GetToken(Tokenizer).Type)
	{
		struct tokenizer AtOpenBracket = *Tokenizer;
		if(ParseConstantExpression(Tokenizer) &&
		   Token_CloseBracket == GetToken(Tokenizer).Type)
		{
			struct tokenizer Previous = *Tokenizer;
			if(ParseDirectAbstractDeclaratorI(Tokenizer))
				return(true);

			*Tokenizer = Previous;
			return(true);
		}

		*Tokenizer = AtOpenBracket;
		if(Token_CloseBracket == GetToken(Tokenizer).Type)
		{
			struct tokenizer Previous = *Tokenizer;
			if(ParseDirectAbstractDeclaratorI(Tokenizer))
				return(true);

			*Tokenizer = Previous;
			return(true);
		}
	}

	if(Token_OpenParen == GetToken(Tokenizer).Type)
	{
		struct tokenizer AtOpenParen = *Tokenizer;
		if(ParseParameterTypeList(Tokenizer) &&
		   Token_CloseParen == GetToken(Tokenizer).Type)
		{
			struct tokenizer Previous = *Tokenizer;
			if(ParseDirectAbstractDeclaratorI(Tokenizer))
				return(true);

			*Tokenizer = Previous;
			return(true);
		}

		*Tokenizer = AtOpenParen;
		if(Token_CloseParen == GetToken(Tokenizer).Type)
		{
			struct tokenizer Previous = *Tokenizer;
			if(ParseDirectAbstractDeclaratorI(Tokenizer))
				return(true);

			*Tokenizer = Previous;
			return(true);
		}
	}

	*Tokenizer = Start;
	return(true);
}

bool ParseAbstractDeclarator(struct tokenizer *Tokenizer);

/*
  direct-abstract-declarator:
  ( abstract-declarator )
  direct-abstract-declarator(opt) [ constant-expression(opt) ]
  direct-abstract-declarator(opt) ( parameter-type-list(opt) )
*/
bool
ParseDirectAbstractDeclarator(struct tokenizer *Tokenizer)
{
	struct tokenizer Start = *Tokenizer;

	if(Token_OpenParen == GetToken(Tokenizer).Type &&
	   ParseAbstractDeclarator(Tokenizer) &&
	   Token_CloseParen == GetToken(Tokenizer).Type &&
	   ParseDirectAbstractDeclaratorI(Tokenizer))
	{
		return(true);
	}

	*Tokenizer = Start;
	return(false);
}

bool ParsePointer(struct tokenizer *Tokenizer);

/*
  abstract-declarator:
  pointer
  pointer(opt) direct-abstract-declarator
*/
bool
ParseAbstractDeclarator(struct tokenizer *Tokenizer)
{
	struct tokenizer Start = *Tokenizer;

	if(ParsePointer(Tokenizer) &&
	   ParseDirectAbstractDeclarator(Tokenizer))
	{
		return(true);
	}

	*Tokenizer = Start;
	if(ParseDirectAbstractDeclarator(Tokenizer))
	{
		return(true);
	}

	*Tokenizer = Start;
	if(ParsePointer(Tokenizer))
	{
		return(true);
	}

	*Tokenizer = Start;
	return(false);
}

bool ParseSpecifierQualifierList(struct tokenizer *Tokenizer);

/*
  type-name:
  specifier-qualifier-list abstract-declarator(opt)
*/
bool
ParseTypeName(struct tokenizer *Tokenizer)
{
	struct tokenizer Start = *Tokenizer;

	if(ParseSpecifierQualifierList(Tokenizer))
	{
		struct tokenizer Previous = *Tokenizer;

		if(ParseAbstractDeclarator(Tokenizer))
		{
			return(true);
		}

		*Tokenizer = Previous;
		return(true);
	}

	*Tokenizer = Start;
	return(false);
}

bool ParseInitializer(struct tokenizer *Tokenizer);

bool
ParseInitializerListI(struct tokenizer *Tokenizer)
{
	struct tokenizer Start = *Tokenizer;

	if(Token_Comma == GetToken(Tokenizer).Type &&
	   ParseInitializer(Tokenizer) &&
	   ParseInitializerListI(Tokenizer))
	{
		return(true);
	}

	*Tokenizer = Start;
	return(true);
}

/*
  initializer-list:
  initializer
  initializer-list , initializer
*/
bool
ParseInitializerList(struct tokenizer *Tokenizer)
{
	struct tokenizer Start = *Tokenizer;

	if(ParseInitializer(Tokenizer) &&
	   ParseInitializerListI(Tokenizer))
	{
		return(true);
	}

	*Tokenizer = Start;
	return(false);
}

/*
  initializer:
  assignment-expression
  { initializer-list }
  { initializer-list , }
*/
bool
ParseInitializer(struct tokenizer *Tokenizer)
{
	struct tokenizer Start = *Tokenizer;

	if(ParseAssignmentExpression(Tokenizer))
	{
		return(true);
	}

	*Tokenizer = Start;
	if(Token_OpenBrace == GetToken(Tokenizer).Type &&
	   ParseInitializerList(Tokenizer) &&
	   Token_CloseBrace == GetToken(Tokenizer).Type)
	{
		return(true);
	}

	*Tokenizer = Start;
	if(Token_OpenBrace == GetToken(Tokenizer).Type &&
	   ParseInitializerList(Tokenizer) &&
	   Token_Comma == GetToken(Tokenizer).Type &&
	   Token_CloseBrace == GetToken(Tokenizer).Type)
	{
		return(true);
	}

	*Tokenizer = Start;
	return(false);
}

bool
ParseIdentifierListI(struct tokenizer *Tokenizer)
{
	struct tokenizer Start = *Tokenizer;

	if(Token_Comma == GetToken(Tokenizer).Type &&
	   ParseIdentifier(Tokenizer) &&
	   ParseIdentifierListI(Tokenizer))
	{
		return(true);
	}

	*Tokenizer = Start;
	return(true);
}

/*
  identifier-list:
  identifier
  identifier-list , identifier
*/
bool
ParseIdentifierList(struct tokenizer *Tokenizer)
{
	struct tokenizer Start = *Tokenizer;

	if(ParseIdentifier(Tokenizer) &&
	   ParseIdentifierListI(Tokenizer))
	{
		return(true);
	}

	*Tokenizer = Start;
	return(false);
}

bool ParseDeclarationSpecifiers(struct tokenizer *Tokenizer);
bool ParseDeclarator(struct tokenizer *Tokenizer);

/*
  parameter-declaration:
  declaration-specifiers declarator
  declaration-specifiers abstract-declarator(opt)
*/
bool
ParseParameterDeclaration(struct tokenizer *Tokenizer)
{
	struct tokenizer Start = *Tokenizer;

	if(ParseDeclarationSpecifiers(Tokenizer) &&
	   ParseDeclarator(Tokenizer))
	{
		return(true);
	}

	*Tokenizer = Start;
	if(ParseDeclarationSpecifiers(Tokenizer))
	{
		struct tokenizer Previous = *Tokenizer;
		if(ParseAbstractDeclarator(Tokenizer))
		{
			return(true);
		}
		*Tokenizer = Previous;
		return(true);
	}

	*Tokenizer = Start;
	return(false);
}

bool
ParseParameterListI(struct tokenizer *Tokenizer)
{
	struct tokenizer Start = *Tokenizer;

	if(Token_Comma == GetToken(Tokenizer).Type &&
	   ParseParameterDeclaration(Tokenizer) &&
	   ParseParameterListI(Tokenizer))
	{
		return(true);
	}

	*Tokenizer = Start;
	return(true);
}

/*
  parameter-list:
  parameter-declaration
  parameter-list , parameter-declaration
*/
bool
ParseParameterList(struct tokenizer *Tokenizer)
{
	struct tokenizer Start = *Tokenizer;

	if(ParseParameterDeclaration(Tokenizer) &&
	   ParseParameterListI(Tokenizer))
	{
		return(true);
	}

	*Tokenizer = Start;
	return(false);
}

/*
  parameter-type-list:
  parameter-list
  parameter-list , ...
*/
bool
ParseParameterTypeList(struct tokenizer *Tokenizer)
{
	struct tokenizer Start = *Tokenizer;

	if(ParseParameterList(Tokenizer))
	{
		struct tokenizer Previous = *Tokenizer;
		if(Token_Comma == GetToken(Tokenizer).Type &&
		   Token_Ellipsis == GetToken(Tokenizer).Type)
		{
			return(true);
		}

		*Tokenizer = Previous;
		return(true);
	}

	*Tokenizer = Start;
	return(false);
}

bool ParseTypeQualifier(struct tokenizer *Tokneizer);

bool
ParseTypeQualifierListI(struct tokenizer *Tokenizer)
{
	struct tokenizer Start = *Tokenizer;

	if(ParseTypeQualifier(Tokenizer) &&
	   ParseTypeQualifierListI(Tokenizer))
	{
		return(true);
	}

	*Tokenizer = Start;
	return(true);
}

/*
  type-qualifier-list:
  type-qualifier
  type-qualifier-list type-qualifier
*/
bool
ParseTypeQualifierList(struct tokenizer *Tokenizer)
{
	struct tokenizer Start = *Tokenizer;

	if(ParseTypeQualifier(Tokenizer) &&
	   ParseTypeQualifierListI(Tokenizer))
	{
		return(true);
	}

	*Tokenizer = Start;
	return(false);
}

/*
  pointer:
  * type-qualifier-list(opt)
  * type-qualifier-list(opt) pointer
  */
bool
ParsePointer(struct tokenizer *Tokenizer)
{
	struct tokenizer Start = *Tokenizer;

	if(Token_Asterisk == GetToken(Tokenizer).Type)
	{
		struct tokenizer AtAsterisk = *Tokenizer;
		if(ParseTypeQualifierList(Tokenizer) &&
		   ParsePointer(Tokenizer))
		{
			return(true);
		}

		*Tokenizer = AtAsterisk;
		if(ParsePointer(Tokenizer))
		{
			return(true);
		}
	}

	*Tokenizer = Start;
	return(false);
}

bool
ParseDirectDeclaratorI(struct tokenizer *Tokenizer)
{
	struct tokenizer Start = *Tokenizer;

	if(Token_OpenBracket == GetToken(Tokenizer).Type)
	{
		struct tokenizer Previous = *Tokenizer;
		if(ParseConstantExpression(Tokenizer) &&
		   Token_CloseBracket == GetToken(Tokenizer).Type &&
		   ParseDirectDeclaratorI(Tokenizer))
		{
			return(true);
		}

		*Tokenizer = Previous;
		if(Token_CloseBracket == GetToken(Tokenizer).Type &&
		   ParseDirectDeclaratorI(Tokenizer))
		{
			return(true);
		}
	}

	*Tokenizer = Start;
	if(Token_OpenParen == GetToken(Tokenizer).Type &&
	   ParseParameterTypeList(Tokenizer) &&
	   Token_CloseParen == GetToken(Tokenizer).Type &&
	   ParseDirectDeclaratorI(Tokenizer))
	{
		return(true);
	}

	*Tokenizer = Start;
	if(Token_OpenParen == GetToken(Tokenizer).Type)
	{
		struct tokenizer Previous = *Tokenizer;
		if(ParseIdentifierList(Tokenizer) &&
		   Token_CloseParen == GetToken(Tokenizer).Type &&
		   ParseDirectDeclaratorI(Tokenizer))
		{
			return(true);
		}

		*Tokenizer = Previous;
		if(Token_CloseParen == GetToken(Tokenizer).Type &&
		   ParseDirectDeclaratorI(Tokenizer))
		{
			return(true);
		}
	}

	*Tokenizer = Start;
	return(true);
}

/*
  direct-declarator:
  identifier
  ( declarator )
  direct-declarator [ constant-expression(opt) ]
  direct-declarator ( parameter-type-list )
  direct-declarator ( identifier-list(opt) )
*/
bool
ParseDirectDeclarator(struct tokenizer *Tokenizer)
{
	struct tokenizer Start = *Tokenizer;

	if(ParseIdentifier(Tokenizer) &&
	   ParseDirectDeclaratorI(Tokenizer))
	{
		return(true);
	}

	*Tokenizer = Start;
	if(Token_OpenParen == GetToken(Tokenizer).Type &&
	   ParseDeclarator(Tokenizer) &&
	   Token_CloseParen == GetToken(Tokenizer).Type &&
	   ParseDirectDeclaratorI(Tokenizer))
	{
		return(true);
	}

	*Tokenizer = Start;
	return(false);
}

/*
  declarator:
  pointer(opt) direct-declarator
*/
bool
ParseDeclarator(struct tokenizer *Tokenizer)
{
	struct tokenizer Start = *Tokenizer;

	if(ParsePointer(Tokenizer) &&
	   ParseDirectDeclarator(Tokenizer))
	{
		return(true);
	}

	*Tokenizer = Start;
	if(ParseDirectDeclarator(Tokenizer))
	{
		return(true);
	}

	*Tokenizer = Start;
	return(false);
}

/*
  enumerator:
  identifier
  identifier = constant-expression
*/
bool
ParseEnumerator(struct tokenizer *Tokenizer)
{
	struct tokenizer Start = *Tokenizer;

	if(ParseIdentifier(Tokenizer))
	{
		struct tokenizer Previous = *Tokenizer;

		if(Token_EqualSign == GetToken(Tokenizer).Type &&
		   ParseConstantExpression(Tokenizer))
		{
			return(true);
		}

		*Tokenizer = Previous;
		return(true);
	}

	*Tokenizer = Start;
	return(false);
}


bool
ParseEnumeratorListI(struct tokenizer *Tokenizer)
{
	struct tokenizer Start = *Tokenizer;

	if(Token_Comma == GetToken(Tokenizer).Type &&
	   ParseEnumerator(Tokenizer) &&
	   ParseEnumeratorListI(Tokenizer))
	{
		return(true);
	}

	*Tokenizer = Start;
	return(true);
}

/*
  enumerator-list:
  enumerator
  enumerator-list , enumerator
*/
bool
ParseEnumeratorList(struct tokenizer *Tokenizer)
{
	struct tokenizer Start = *Tokenizer;

	if(ParseEnumerator(Tokenizer) &&
	   ParseEnumeratorListI(Tokenizer))
	{
		return(true);
	}

	*Tokenizer = Start;
	return(false);
}

/*
  enum-specifier:
  enum identifier(opt) { enumerator-list }
  enum identifier
*/
bool
ParseEnumSpecifier(struct tokenizer *Tokenizer)
{
	struct tokenizer Start = *Tokenizer;

	struct token Token = GetToken(Tokenizer);
	if((Token_Keyword == Token.Type) &&
	   IsStringEqual("enum", Token.Text, Token.TextLength))
	{
		struct tokenizer AtEnum = *Tokenizer;
		if(ParseIdentifier(Tokenizer))
		{
			struct tokenizer AtIdentifier = *Tokenizer;
			if(Token_OpenBrace == GetToken(Tokenizer).Type &&
			   ParseEnumeratorList(Tokenizer) &&
			   Token_CloseBrace == GetToken(Tokenizer).Type)
			{
				return(true);
			}

			*Tokenizer = AtIdentifier;
			return(true);
		}

		*Tokenizer = AtEnum;
		if(Token_OpenBrace == GetToken(Tokenizer).Type &&
		   ParseEnumeratorList(Tokenizer) &&
		   Token_CloseBrace == GetToken(Tokenizer).Type)
		{
			return(true);
		}

		*Tokenizer = AtEnum;
		return(false);
	}

	*Tokenizer = Start;
	return(false);
}

/*
  struct-declarator:
  declarator
  declarator(opt) : constant-expression
*/
bool
ParseStructDeclarator(struct tokenizer *Tokenizer)
{
	struct tokenizer Start = *Tokenizer;

	if(ParseDeclarator(Tokenizer))
	{
		struct tokenizer Previous = *Tokenizer;

		if(Token_Colon == GetToken(Tokenizer).Type &&
		   ParseConstantExpression(Tokenizer))
		{
			return(true);
		}

		*Tokenizer = Previous;
		return(true);
	}

	*Tokenizer = Start;
	if(Token_Colon == GetToken(Tokenizer).Type &&
	   ParseConstantExpression(Tokenizer))
	{
		return(true);
	}

	*Tokenizer = Start;
	return(false);
}

bool
ParseStructDeclaratorListI(struct tokenizer *Tokenizer)
{
	struct tokenizer Start = *Tokenizer;

	if(Token_Comma == GetToken(Tokenizer).Type &&
	   ParseStructDeclarator(Tokenizer) &&
	   ParseStructDeclaratorListI(Tokenizer))
	{
		return(true);
	}

	*Tokenizer = Start;
	return(true);
}

/*
  struct-declarator-list:
  struct-declarator
  struct-declarator-list , struct-declarator
*/
bool
ParseStructDeclaratorList(struct tokenizer *Tokenizer)
{
	struct tokenizer Start = *Tokenizer;

	if(ParseStructDeclarator(Tokenizer) &&
	   ParseStructDeclaratorListI(Tokenizer))
	{
		return(true);
	}

	*Tokenizer = Start;
	return(false);
}

bool ParseTypeSpecifier(struct tokenizer *Tokenizer);

/*
  specifier-qualifier-list:
  type-specifier specifier-qualifier-list(opt)
  type-qualifier specifier-qualifier-list(opt)
*/
bool
ParseSpecifierQualifierList(struct tokenizer *Tokenizer)
{
	struct tokenizer Start = *Tokenizer;

	if(ParseTypeSpecifier(Tokenizer))
	{
		struct tokenizer Previous = *Tokenizer;
		if(ParseSpecifierQualifierList(Tokenizer))
		{
			return(true);
		}
		*Tokenizer = Previous;

		return(true);
	}

	if(ParseTypeQualifier(Tokenizer))
	{
		struct tokenizer Previous = *Tokenizer;
		if(ParseSpecifierQualifierList(Tokenizer))
		{
			return(true);
		}
		*Tokenizer = Previous;

		return(true);
	}

	*Tokenizer = Start;
	return(false);
}

/*
  TODO(AARON): Make sure optional matches are correctly handled below!
  I may have done it incorrectly... ie., should return true in outer if(...)
*/

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
	   ParseInitializer(Tokenizer))
	{
		return(true);
	}

	*Tokenizer = Start;
	return(false);
}

bool
ParseInitDeclaratorListI(struct tokenizer *Tokenizer)
{
	struct tokenizer Start = *Tokenizer;

	if(Token_Comma == GetToken(Tokenizer).Type &&
	   ParseInitDeclarator(Tokenizer))
	{
		return(true);
	}

	*Tokenizer = Start;
	return(true);
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
	return(true);
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

bool ParseDeclaration(struct tokenizer *Tokenizer);

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
	return(true);
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
	return(true);
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
