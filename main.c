/*
 * TODO(AARON):
 * - Build a parse tree consisting of all tokens.
 * - Octal support for integers
 * - Floating point numbers (decimal point, suffix, scientific notation)
 * - Remove dependency on string.h
 */
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
		case Token_PercentSign: { return ""; } break;
		case Token_QuestionMark: { return ""; } break;
		case Token_EqualSign: { return ""; } break;
		case Token_Carat: { return ""; } break;
		case Token_Comma: { return ""; } break;
		case Token_Cross: { return ""; } break;
		case Token_Dash: { return ""; } break;
		case Token_Slash: { return ""; } break;
		case Token_Dot: { return ""; } break;
		case Token_Bang: { return ""; } break;
		case Token_Pipe: { return ""; } break;
		case Token_LessThan: { return ""; } break;
		case Token_GreaterThan: { return ""; } break;
		case Token_Tilde: { return ""; } break;

		case Token_NotEqual: { return ""; } break;
		case Token_GreaterThanEqual: { return ""; } break;
		case Token_LessThanEqual: { return ""; } break;
		case Token_LogicalOr: { return ""; } break;
		case Token_LogicalAnd: { return ""; } break;
		case Token_BitShiftLeft: { return ""; } break;
		case Token_BitShiftRight: { return ""; } break;
		case Token_Arrow: { return ""; } break;

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
	printf("Usage: program file\n");
	printf("  file: must be a file in this directory\n");
	printf("  Specify '-h' or '--help' for this help text\n");
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
	while(true)
	{
		if(IsWhitespace(Tokenizer->At[0]))
		{
			++Tokenizer->At;
		}
		else
		{
			break;
		}
	}
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
		return true;
	}

	/* Can't have a multi-digit integer starting with zero unless it's Octal. */
	if(Tokenizer->At[0] == '0') return(false);

	for(int i=0; i<Length-1; ++i)
	{
		if(!IsDecimal(Tokenizer->At[i])) return(false);
	}

	if(IsDecimal(*LastChar) || IsIntegerSuffix(*LastChar))
	{
		Tokenizer->At += Length;
		Token->Type = Token_Integer;
		return(true);
	}

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
		if(Tokenizer->At[0] == '\0') return(false);
		if(Tokenizer->At[0] == '*' && Tokenizer->At[1] == '/') break;
		++Tokenizer->At;
	}

	Tokenizer->At += 2; /* Swallow last two characters. */

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
	if(*(++Marker) != '\n') return(false);

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
		GetSymbol(Tokenizer, &Token, ">>", Token_BitShiftRight);
	}
	if(Token.Type != Token_Unknown) return(Token);

	{
		GetKeyword(Tokenizer, &Token) ||
		GetCharacter(Tokenizer, &Token) ||
		GetPreprocessorCommand(Tokenizer, &Token) ||
		GetComment(Tokenizer, &Token) ||
		GetString(Tokenizer, &Token) ||
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

void
CopyToken(struct token *Destination, struct token *Source)
{
	Destination->Type = Source->Type;
	Destination->Text = Source->Text;
	Destination->TextLength = Source->TextLength;
}

struct token
UnknownToken()
{
	struct token Token;
	Token.Type = Token_Unknown;
	Token.Text = NULL;
	Token.TextLength = 0;
	return(Token);
}

struct parse_node
NewParseNode(struct token Token)
{
	struct parse_node Node;
	CopyToken(&Node.Token, &Token);
	Node.Children = (struct parse_node *)malloc(sizeof(struct parse_node) * PARSE_NODE_DEFAULT_CAPACITY);
	Node.Parent = NULL;
	Node.NumChildren = 0;
	Node.Capacity = PARSE_NODE_DEFAULT_CAPACITY;
	return(Node);
}

void
PrintParseTree(struct parse_node *Node, int Indent)
{
	char *RootText = "Root";
	char *Text = Node->Token.Text;
	int TextLength = Node->Token.TextLength;
	if(!Node->Parent)
	{
		Text = RootText;
		TextLength = strlen(RootText);
	}

	for(int i=0; i<Indent; ++i) printf(" ");
	printf("%s: %.*s\n", TokenName(Node->Token.Type), TextLength, Text);
	if(Node->NumChildren != 0)
	{
		for (int i = 0; i < Node->NumChildren; ++i)
		{
			PrintParseTree(&(Node->Children[i]), Indent + 4);
		}
	}
}

bool
AddParseNode(struct parse_node *Parent, struct parse_node Node)
{
	/* Allocate new space and copy existing contents */
	if(!Parent || Parent->NumChildren >= Parent->Capacity)
	{
		PrintParseTree(Parent, 0);
		AbortWithMessage("Parse Node out of space.");
	}

	struct parse_node *New = &(Parent->Children[Parent->NumChildren++]);
	Node.Parent = Parent;
	memcpy(New, &Node, sizeof(Node));

	return true;
}

/* argv[1] is the input file name. */
int
main(int argc, char *argv[])
{
	if(argc != 2 || strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0) Usage();

	size_t AllocSize = FileSize(argv[1]);
	struct buffer FileContents;

	/* Allocate space on the stack. */
	BufferSet(&FileContents, (char *)alloca(AllocSize), 0, AllocSize);

	if(!CopyFileIntoBuffer(argv[1], &FileContents))
	{
		AbortWithMessage("Couldn't copy entire file to buffer");
	}

	char print_buffer[Kilobytes(1)] = { 0 };
	int pos = 0;

	struct tokenizer Tokenizer;
	Tokenizer.Beginning = FileContents.Data;
	Tokenizer.At = FileContents.Data;

	struct parse_node RootParseNode = NewParseNode(UnknownToken());
	struct parse_node *ParseNode = &RootParseNode;

	bool Parsing = true;
	while(Parsing)
	{
		struct token Token = GetToken(&Tokenizer);
		switch(Token.Type)
		{
			case Token_EndOfStream: {
				Parsing = false;
			} break;

			/* case Token_PreprocessorCommand: { printf("Preprocessor: "); } break; */
			/* case Token_Comment:		{ printf("Comment: "); } break; */
			case Token_Keyword:
			case Token_Character:
			case Token_String:
			case Token_Integer:
			case Token_PrecisionNumber:
			case Token_Identifier: {
				AddParseNode(ParseNode, NewParseNode(Token));
			} break;

			case Token_OpenBracket:
			case Token_OpenBrace:
			case Token_OpenParen: {
				AddParseNode(ParseNode, NewParseNode(Token));
				ParseNode = &(ParseNode->Children[ParseNode->NumChildren-1]);
			} break;

			case Token_CloseBracket:
			case Token_CloseBrace:
			case Token_CloseParen: {
				ParseNode = ParseNode->Parent;
				AddParseNode(ParseNode, NewParseNode(Token));
			} break;

			case Token_Asterisk:
			case Token_Ampersand:
			case Token_Colon:
			case Token_SemiColon:
			case Token_PercentSign:
			case Token_QuestionMark:
			case Token_EqualSign:
			case Token_Carat:
			case Token_Comma:
			case Token_Cross:
			case Token_Dash:
			case Token_Slash:
			case Token_Dot:
			case Token_Bang:
			case Token_Pipe:
			case Token_LessThan:
			case Token_GreaterThan:
			case Token_Tilde:

			case Token_NotEqual:
			case Token_GreaterThanEqual:
			case Token_LessThanEqual:
			case Token_LogicalOr:
			case Token_LogicalAnd:
			case Token_BitShiftLeft:
			case Token_BitShiftRight:
			/* case Token_Arrow:		{ printf("Symbol: "); } break; */

			case Token_Unknown:		{} break;

			default:			{} break;
		}
	}

	PrintParseTree(&RootParseNode, 0);

	return(EXIT_SUCCESS);
}
