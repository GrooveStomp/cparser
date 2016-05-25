#include <stdio.h>
#include <alloca.h>
#include <string.h>
#include <stdlib.h> /* EXIT_SUCCESS, EXIT_FAILURE */

#include "file_buffer.c"

typedef int bool;

#define PARSE_NODE_DEFAULT_CAPACITY 1000
#define ARRAY_SIZE(Array) (sizeof((Array)) / sizeof((Array)[0]))
#define false 0
#define true !false

#define Min(a,b) ((a) < (b) ? (a) : (b))
#define Bytes(n) (n)
#define Kilobytes(n) (Bytes(n) * 1000)

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
};

struct parse_node
{
	struct token Token;
	struct parse_node *Children;
	struct parse_node *Parent;
	int NumChildren;
	int Capacity;
};

struct tokenizer
{
	char *Beginning;
	char *At;
};

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

static void
EatAllWhitespace(struct tokenizer *Tokenizer)
{
	for(; IsWhitespace(Tokenizer->At[0]); ++Tokenizer->At);
}

bool
GetCharacter(struct tokenizer *Tokenizer, struct token *Token)
{
	if(Tokenizer->At[0] != '\'') return(false);

	char *Close = Tokenizer->At + 0x1l;
	if(*Close != '\'') Close += 1;
	if(*Close != '\'') return(false);
	if(*Close == '\'' && *(Close-1) == '\\') Close += 1;
	if(*Close != '\'') return(false);

	Token->Text = Tokenizer->At;
	Token->TextLength = Close - Tokenizer->At + 1;
	Token->Type = Token_Character;
	Tokenizer->At += Token->TextLength;

	return(true);
}

bool
GetString(struct tokenizer *Tokenizer, struct token *Token)
{
	char *Text = Tokenizer->At;
	if(Tokenizer->At[0] != '"') return(false);

	while(true)
	{
		++Tokenizer->At;
		if(Tokenizer->At[0] == '\0') return(false);
		if(Tokenizer->At[0] == '"' && *(Tokenizer->At - 1) != '\\') break;
	}
	++Tokenizer->At; /* Swallow the last double quote. */

	Token->Text = Text;
	Token->TextLength = Tokenizer->At - Token->Text;
	Token->Type = Token_String;

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

	char *ReadCursor = Tokenizer->At;

	bool HasIntegerPart = false;
	bool HasDecimalPoint = false;
	bool HasFractionalPart = false;
	bool HasExponentPart = false;

	if(IsDecimal(Tokenizer->At[0]))
	{
		for(; IsDecimal(Tokenizer->At[0]); ++Tokenizer->At);
		HasIntegerPart = true;
	}

	if('.' == Tokenizer->At[0])
	{
		++Tokenizer->At;
		HasDecimalPoint = true;
		if(IsDecimal(Tokenizer->At[0]))
		{
			for(; IsDecimal(Tokenizer->At[0]); ++Tokenizer->At);
			HasFractionalPart = true;
		}
	}

	if('e' == Tokenizer->At[0] || 'E' == Tokenizer->At[0])
	{
		++Tokenizer->At;
		HasExponentPart = true;

		/* Optional negative sign for exponent is allowed. */
		if('-' == Tokenizer->At[0])
		{
			++Tokenizer->At;
		}

		/* Exponent must contain an exponent part. */
		if(!IsDecimal(Tokenizer->At[0]))
		{
			Tokenizer->At = ReadCursor;
			return(false);
		}

		for(; IsDecimal(Tokenizer->At[0]); ++Tokenizer->At);
	}

	/* IsFloatSuffix(C) */
	char C = Tokenizer->At[0];
	if('f' == C || 'F' == C || 'l' == C || 'L' == C)
	{
		++Tokenizer->At;
	}

	if((HasIntegerPart || HasFractionalPart) &&
	   (HasDecimalPoint || HasExponentPart))
	{
		Token->Type = Token_PrecisionNumber;
		Token->Text = ReadCursor;
		Token->TextLength = Tokenizer->At - ReadCursor;
		return(true);
	}

	Tokenizer->At = ReadCursor;
	return(false);
}

bool
GetInteger(struct tokenizer *Tokenizer, struct token *Token)
{
	char *LastChar = Tokenizer->At;
	for(; LastChar && (IsDecimal(*LastChar) || IsAlphabetical(*LastChar)); ++LastChar);

	int Length = LastChar - Tokenizer->At;
	--LastChar;

	Token->Type = Token_Unknown;
	Token->Text = Tokenizer->At;
	Token->TextLength = Length;

	if(Length < 1) return(false);

	if ((IsOctalString(Tokenizer->At, Length) || IsHexadecimalString(Tokenizer->At, Length)) ||
	   (1 == Length && '0' == Tokenizer->At[0]))
	{
		Tokenizer->At += Length;
		Token->Type = Token_Integer;
		return(true);
	}

	/* Can't have a multi-digit integer starting with zero unless it's Octal. */
	if(Tokenizer->At[0] == '0')
	{
		Tokenizer->At = Token->Text;
		return(false);
	}

	for(int i=0; i<Length-1; ++i)
	{
		if(!IsDecimal(Tokenizer->At[i]))
		{
			Tokenizer->At = Token->Text;
			return(false);
		}
	}

	if(IsDecimal(*LastChar) || IsIntegerSuffix(*LastChar))
	{
		Tokenizer->At += Length;
		Token->Type = Token_Integer;
		return(true);
	}

	Tokenizer->At = Token->Text;
	return(false);
}

bool
GetIdentifier(struct tokenizer *Tokenizer, struct token *Token)
{
	if(!IsAlphabetical(Tokenizer->At[0])) return(false);

	char *Text = Tokenizer->At;

	while(true)
	{
		if(!IsIdentifierCharacter(Tokenizer->At[0])) break;
		++Tokenizer->At;
	}

	Token->Text = Text;
	Token->TextLength = Tokenizer->At - Token->Text;
	Token->Type = Token_Identifier;

	return(true);
}

bool
GetComment(struct tokenizer *Tokenizer, struct token *Token)
{
	if(Tokenizer->At[0] != '/' || Tokenizer->At[1] != '*') return(false);

	char *Text = Tokenizer->At;

	while(true)
	{
		if(Tokenizer->At[0] == '\0')
		{
			Tokenizer->At = Text;
			return(false);
		}
		if(Tokenizer->At[0] == '*' && Tokenizer->At[1] == '/')
		{
			break;
		}
		++Tokenizer->At;
	}

	Tokenizer->At += 2; /* Swallow last two characters: asterisk, slash */

	Token->Text = Text;
	Token->TextLength = Tokenizer->At - Token->Text;
	Token->Type = Token_Comment;
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

	Token->Text = Tokenizer->At;
	Token->TextLength = 0;
	Token->Type = Token_Unknown;

	for(int i = 0; i < ARRAY_SIZE(Keywords); ++i)
	{
		if(strstr(Tokenizer->At, Keywords[i]) == Tokenizer->At)
		{
			Token->TextLength = strlen(Keywords[i]);
			Token->Type = Token_Keyword;
			Tokenizer->At += Token->TextLength;
			return(true);
		}
	}

	return(false);
}

bool
GetPreprocessorCommand(struct tokenizer *Tokenizer, struct token *Token)
{
	if(Tokenizer->At[0] != '#') return(false);

	char *Text, *Marker;
	Text = Marker = Tokenizer->At++;

	/* Preprocessor commands must start a line on their own. */
	for(--Marker; Marker > Tokenizer->Beginning && IsWhitespace(*Marker); --Marker);
	if(*(++Marker) != '\n' && Marker != Tokenizer->Beginning)
	{
		Tokenizer->At = Text;
		return(false);
	}

	while(true)
	{
		if(Tokenizer->At[0] == '\n' && *(Tokenizer->At - 1) != '\\') break;
		if(Tokenizer->At[0] == '\0') break;
		++Tokenizer->At;
	}

	Token->Text = Text;
	Token->TextLength = Tokenizer->At - Token->Text;
	Token->Type = Token_PreprocessorCommand;
	return(true);
}

bool
GetSymbol(struct tokenizer *Tokenizer, struct token *Token, char *Symbol, enum token_type Type)
{
	int Length = strlen(Symbol);
	int Match = strncmp(Tokenizer->At, Symbol, Length);
	if(Match != 0) return(false);

	Token->Text = Tokenizer->At;
	Token->TextLength = Length;
	Token->Type = Type;

	Tokenizer->At += Length;

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
	++Tokenizer->At;
	Token.TextLength = 1;

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
	char *ReadCursor = Tokenizer->At;
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
			Tokenizer->At = ReadCursor;
			return(false);
		}
	}
}

bool ParseExpression(struct tokenizer *Tokenizer);

bool
ParsePrimaryExpression(struct tokenizer *Tokenizer)
{
	char *ReadCursor = Tokenizer->At;
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
	Tokenizer->At = ReadCursor;
	if(ParseConstant(Tokenizer))
	{
		return(true);
	}

	Tokenizer->At = ReadCursor;
	if(Token_OpenParen == GetToken(Tokenizer).Type &&
	   ParseExpression(Tokenizer) &&
	   Token_CloseParen == GetToken(Tokenizer).Type)
	{
		return true;
	}

	/* There were no matches. Parsing for PrimaryExpression failed.
	   Reset the Tokenizer and return false. */
	Tokenizer->At = ReadCursor;
	return(false);
}

bool ParseAssignmentExpression(struct tokenizer *Tokenizer);

bool
ParseExpression(struct tokenizer *Tokenizer)
{
	char *ReadCursor = Tokenizer->At;

	if(ParseAssignmentExpression(Tokenizer))
	{
		return(true);
	}

	Tokenizer->At = ReadCursor;
	if(ParseExpression(Tokenizer) &&
	   Token_Comma == GetToken(Tokenizer).Type &&
	   ParseAssignmentExpression(Tokenizer))
	{
		return(true);
	}

	Tokenizer->At = ReadCursor;
	return(false);
}

bool
ParseArgumentExpressionList(struct tokenizer *Tokenizer)
{
	char *ReadCursor = Tokenizer->At;

	if(ParseAssignmentExpression(Tokenizer))
	{
		return(true);
	}

	Tokenizer->At = ReadCursor;
	if(ParseArgumentExpressionList(Tokenizer) &&
	   Token_Comma == GetToken(Tokenizer).Type &&
	   ParseAssignmentExpression(Tokenizer))
	{
		return(true);
	}


	Tokenizer->At = ReadCursor;
	return(false);
}

bool
ParsePostfixExpression(struct tokenizer *Tokenizer)
{
	char *ReadCursor = Tokenizer->At;

	if(ParsePrimaryExpression(Tokenizer))
	{
		return(true);
	}

	Tokenizer->At = ReadCursor;
	if(ParsePostfixExpression(Tokenizer) &&
	   Token_OpenBracket == GetToken(Tokenizer).Type &&
	   ParseExpression(Tokenizer) &&
	   Token_CloseBracket == GetToken(Tokenizer).Type)
	{
		return(true);
	}

	Tokenizer->At = ReadCursor;
	if(ParsePostfixExpression(Tokenizer) &&
	   Token_OpenParen == GetToken(Tokenizer).Type &&
	   ParseArgumentExpressionList(Tokenizer) &&
	   Token_CloseParen == GetToken(Tokenizer).Type)
	{
		return(true);
	}

	Tokenizer->At = ReadCursor;
	if(ParsePostfixExpression(Tokenizer) &&
	   Token_OpenParen == GetToken(Tokenizer).Type &&
	   Token_CloseParen == GetToken(Tokenizer).Type)
	{
		return(true);
	}

	Tokenizer->At = ReadCursor;
	if(ParsePostfixExpression(Tokenizer) &&
	   Token_Dot == GetToken(Tokenizer).Type &&
	   Token_Identifier == GetToken(Tokenizer).Type)
	{
		return(true);
	}

	Tokenizer->At = ReadCursor;
	if(ParsePostfixExpression(Tokenizer) &&
	   Token_Arrow == GetToken(Tokenizer).Type &&
	   Token_Identifier == GetToken(Tokenizer).Type)
	{
		return(true);
	}

	Tokenizer->At = ReadCursor;
	if(ParsePostfixExpression(Tokenizer) &&
	   Token_PlusPlus == GetToken(Tokenizer).Type)
	{
		return(true);
	}

	Tokenizer->At = ReadCursor;
	if(ParsePostfixExpression(Tokenizer) &&
	   Token_MinusMinus == GetToken(Tokenizer).Type)
	{
		return(true);
	}

	Tokenizer->At = ReadCursor;
	return(false);
}

bool
ParseUnaryOperator(struct tokenizer *Tokenizer)
{
	char *ReadCursor = Tokenizer->At;
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
			Tokenizer->At = ReadCursor;
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
	char *ReadCursor = Tokenizer->At;

	if(ParseUnaryExpression)
	{
		return(true);
	}

	Tokenizer->At = ReadCursor;
	if(Token_OpenParen == GetToken(Tokenizer).Type &&
	   ParseTypeName(Tokenizer) &&
	   Token_CloseParen == GetToken(Tokenizer).Type)
	{
		return(true);
	}

	Tokenizer->At = ReadCursor;
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
	struct token Token;
	char *ReadCursor = Tokenizer->At;

	if(ParsePostfixExpression(Tokenizer))
	{
		return(true);
	}

	Tokenizer->At = ReadCursor;
	if(Token_PlusPlus == GetToken(Tokenizer).Type &&
	   ParseUnaryExpression(Tokenizer))
	{
		return(true);
	}

	Tokenizer->At = ReadCursor;
	if(Token_MinusMinus == GetToken(Tokenizer).Type &&
	   ParseUnaryExpression(Tokenizer))
	{
		return(true);
	}

	Tokenizer->At = ReadCursor;
	if(ParseUnaryOperator(Tokenizer) &&
	   ParseCastExpression(Tokenizer))
	{
		return(true);
	}

	Tokenizer->At = ReadCursor;
	Token = GetToken(Tokenizer);
	if(Token_Keyword == Token.Type)
	{
		if(strncmp("sizeof", Token.Text, strlen("sizeof")) == 0)
		{
			char *CurrentCursor = Tokenizer->At;
			if(ParseUnaryExpression(Tokenizer))
			{
				return(true);
			}

			Tokenizer->At = CurrentCursor;
			if(Token_OpenParen == GetToken(Tokenizer).Type &&
			   ParseTypeName(Tokenizer) &&
			   Token_CloseParen == GetToken(Tokenizer).Type)
			{
				return(true);
			}
		}
	}

	Tokenizer->At = ReadCursor;
	return(false);
}

bool
ParseAssignmentOperator(struct tokenizer *Tokenizer)
{
	char *ReadCursor = Tokenizer->At;
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
			Tokenizer->At = ReadCursor;
			return(false);
		}
	}
}

bool
ParseAssignmentExpression(struct tokenizer *Tokenizer)
{
	char *ReadCursor = Tokenizer->At;

	if(ParseConditionalExpression(Tokenizer))
	{
		return(true);
	}

	Tokenizer->At = ReadCursor;
	if(ParseUnaryExpression(Tokenizer) &&
		ParseAssignmentOperator(Tokenizer) &&
		ParseAssignmentExpression(Tokenizer))
	{
		return(true);
	}

	Tokenizer->At = ReadCursor;
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
			default:
			{
				printf("Token Name: %20s, Token Text: %.*s\n",
				       TokenName(Token.Type),
				       Token.TextLength, Token.Text);
			} break;
		}
	}
}

void
Parse(struct buffer *FileContents)
{
	struct tokenizer Tokenizer;
	Tokenizer.Beginning = FileContents->Data;
	Tokenizer.At = FileContents->Data;

/* TODO: Call Recursive Descent Entrypoint. */
}

int main(int ArgCount, char **Args)
{
	for(int Index = 0; Index < ArgCount; ++Index)
	{
		if(strncmp(Args[Index], "-h", strlen("-h")) == 0 ||
		   strncmp(Args[Index], "--help", strlen("--help")) == 0)
		{
			Usage();
		}
	}
	if(ArgCount != 3) Usage();

	if(strncmp(Args[1], "parse", strlen("parse")) != 0 &&
	   strncmp(Args[1], "lex", strlen("lex")) != 0)
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

	if(strncmp(Args[1], "parse", strlen("parse")) == 0)
	{
		Parse(&FileContents);
	}
	else
	{
		Lex(&FileContents);
	}

	return(EXIT_SUCCESS);
}
