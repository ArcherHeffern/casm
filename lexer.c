#include <math.h>
#include <string.h>
#include <strings.h>
#include <stdio.h>
#include <stdlib.h>

#include "lexer.h"
#include "util.h"

typedef struct Scanner Scanner;
struct Scanner {
       char* s;
       int start;
       int cur;
};

char* lexer_error = NULL;

char* TokenTypeToString[] = {
	"TOKEN_EQUAL",
	"TOKEN_R_BRACKET",
	"TOKEN_L_BRACKET",
	"TOKEN_AT",
	"TOKEN_DOLLAR",
	"TOKEN_COMMA",

	"TOKEN_LOAD",
	"TOKEN_STORE",
	"TOKEN_READ",
	"TOKEN_WRITE",

	"TOKEN_ADD",
	"TOKEN_SUB",
	"TOKEN_MUL",
	"TOKEN_DIV",
	"TOKEN_INC",

	"TOKEN_BR",
	"TOKEN_BLT",
	"TOKEN_BGT",
	"TOKEN_BLEQ",
	"TOKEN_BGEQ",
	"TOKEN_BEQ",
	"TOKEN_BNEQ",

	"TOKEN_LABEL_REF",
	"TOKEN_REGISTER",
	"TOKEN_NUMBER",

	"TOKEN_HALT",
	"TOKEN_NONE"
};

void ScannerInit(char* s);
void ScannerFree();
char ScannerAdvance();
char ScannerPeek();
void ScannerDbg();
void ScannerAddToken(TokenType token_type);
int ScannerTokenLength();
char ScannerGetTokenCharAt(int i);
bool ScannerAtEnd();
Token* TokenInit(TokenType type, char* literal, int length);
void TokenFree(Token* token);
void ScannerScanNumber();
bool ScannerContainsRegister();
TokenType ScannerCheckRest(int pos, char* s, int len, TokenType token);
TokenType ScannerParseIdentifier();
void ScannerScanIdentifier();
void ScannerSkipWhitespace();

Token* tokens[MAX_TOKENS];
size_t num_tokens;
Scanner* scanner = NULL;

void ScannerInit(char* s) {
	scanner = (Scanner*)malloc(sizeof(Scanner));
	memset(scanner, 0, sizeof(Scanner));
	scanner->s = s;
}

char ScannerAdvance() {
	scanner->cur++;
	return scanner->s[scanner->cur-1];
}

char ScannerPeek() {
	return scanner->s[scanner->cur];
}

void ScannerDbg() {
	printf("{ .start=%d, .cur=%d, %.*s }\n", 
		scanner->start,
		scanner->cur,
		scanner->cur-scanner->start,
		&scanner->s[scanner->start]
	);
}

void ScannerAddToken(TokenType token_type) {
	Token* token = TokenInit(token_type,&scanner->s[scanner->start], scanner->cur-scanner->start);
	scanner->start = scanner->cur;
	scanner->cur = scanner->start;
	tokens[num_tokens++] = token;
}


int ScannerTokenLength() {
	return scanner->cur - scanner->start;
}

char ScannerGetTokenCharAt(int i) {
	return scanner->s[scanner->start+i];
}

void ScannerFree() {
	free(scanner);
};

bool ScannerAtEnd() {
	char cur_char = scanner->s[scanner->cur];
	return cur_char == '\0' || cur_char == '\n' || cur_char == ';'; // ; is comment
}

void TokenDbg(Token* token) {
	printf("{ .type=%d, .literal=%.*s }\n", 
		token->type,
		token->length, 
		token->literal
	);
}

Token* TokenInit(TokenType type, char* literal, int length) {
	Token* token = (Token*)malloc(sizeof(Token));
	token->type = type;
	token->literal = literal;
	token->length = length;
	return token;
}

void TokenFree(Token* token) {
	free(token);
}

void TokenListPrint(TokenList* token_list) {
	for (int i = 0; i < token_list->size; i++) {
		TokenDbg(token_list->tokens[i]);
	}
}

void ScannerScanNumber() {
	while (!ScannerAtEnd() && IsDigit(ScannerPeek())) {
		ScannerAdvance();
	};
	ScannerAddToken(TOKEN_NUMBER);
}

bool ScannerContainsRegister() {
	return scanner->cur - scanner->start == 2 
		&& scanner->s[scanner->start] == 'R'
		&& IsDigit(scanner->s[scanner->start+1]);
}

TokenType ScannerCheckRest(int pos, char* s, int len, TokenType token) {
	if (ScannerTokenLength() - pos != len) {
		return TOKEN_LABEL_REF;
	}
	if (strncasecmp(&scanner->s[scanner->start+pos], s, len) == 0) {
		return token;
	}
	return TOKEN_LABEL_REF;
}

TokenType ScannerParseIdentifier() {
	// Very fast! Conceptually a hard coded trie
	switch (ScannerGetTokenCharAt(0)) {
		case 'A':
			return ScannerCheckRest(1, "DD", 2, TOKEN_ADD);
		case 'B':
			if (ScannerTokenLength() < 2) {
				return TOKEN_LABEL_REF;
			}
			switch (ScannerGetTokenCharAt(1)) {
				case 'E':
					return ScannerCheckRest(2, "Q", 1, TOKEN_BEQ);
				case 'G':
					if (ScannerTokenLength() < 3) {
						return TOKEN_LABEL_REF;
					}
					switch (ScannerGetTokenCharAt(2)) {
						case 'T':
							return ScannerCheckRest(3, "", 0, TOKEN_BGT);
						case 'E':
							return ScannerCheckRest(3, "Q", 1, TOKEN_BGEQ);
					}
					break;
				case 'L':
					if (ScannerTokenLength() < 3) {
						return TOKEN_LABEL_REF;
					}
					switch (ScannerGetTokenCharAt(2)) {
						case 'E':
							return ScannerCheckRest(3, "Q", 1, TOKEN_BLEQ);
						case 'T':
							return ScannerCheckRest(3, "", 0, TOKEN_BLT);
					}
					break;
				case 'N':
					return ScannerCheckRest(2, "EQ", 2, TOKEN_BNEQ);
				case 'R':
					return ScannerCheckRest(2, "", 0, TOKEN_BR);
			}
			break;
		case 'D':
			return ScannerCheckRest(1, "IV", 2, TOKEN_DIV);
		case 'H':
			return ScannerCheckRest(1, "ALT", 3, TOKEN_HALT);
		case 'I':
			return ScannerCheckRest(1, "NC", 2, TOKEN_INC);
		case 'L':
			return ScannerCheckRest(1, "OAD", 3, TOKEN_LOAD);
		case 'M':
			return ScannerCheckRest(1, "UL", 2, TOKEN_MUL);
		case 'R':
			return ScannerCheckRest(1, "EAD", 3, TOKEN_READ);
		case 'S':
			if (ScannerTokenLength() < 2) {
				return TOKEN_LABEL_REF;
			}
			switch (ScannerGetTokenCharAt(1)) {
				case 'T':
					return ScannerCheckRest(2, "ORE", 3, TOKEN_STORE);
				case 'U':
					return ScannerCheckRest(2, "B", 1, TOKEN_SUB);
			}
			break;
		case 'W':
			return ScannerCheckRest(1, "RITE", 4, TOKEN_WRITE);
	}
	return TOKEN_LABEL_REF;
}

void ScannerScanIdentifier() {
	while (!ScannerAtEnd()) {
		char c = ScannerPeek();
		if (!IsDigit(c) && !IsAlpha(c) && c != '_') {
			break;
		}
		ScannerAdvance();
	}
	if (ScannerContainsRegister()) {
		ScannerAddToken(TOKEN_REGISTER);
	} else {
		ScannerAddToken(ScannerParseIdentifier());
	}
}

void ScannerSkipWhitespace() {
	while (!ScannerAtEnd()) {
		char c = ScannerPeek();
		switch (c) {
			case ' ':
			case '\n':
			case '\r':
			case '\t':
				scanner->start++;
				scanner->cur++;
			default:
				return;
		}
	}
}

TokenList* TokenizeLine(char* s) {
	ScannerInit(s);
	if (lexer_error) {
		free(lexer_error);
		lexer_error = NULL;
	}
	for (int i = 0; i < num_tokens; i++) {
		tokens[i] = NULL;
	}
	num_tokens = 0;

	ScannerSkipWhitespace();
	while (!ScannerAtEnd()) {
		char c = ScannerAdvance();
		switch (c) {
			case '=':
				ScannerAddToken(TOKEN_EQUAL);
				break;
			case ']':
				ScannerAddToken(TOKEN_R_BRACKET);
				break;
			case '[':
					ScannerAddToken(TOKEN_L_BRACKET);
					break;
			case '@':
					ScannerAddToken(TOKEN_AT);
					break;
			case '$':
					ScannerAddToken(TOKEN_DOLLAR);
					break;
			case ',':
					ScannerAddToken(TOKEN_COMMA);
					break;
			default:
				if (IsDigit(c)) {
					ScannerScanNumber();
				}
				else if (IsAlpha(c)) {
					ScannerScanIdentifier();
				} else {
					asprintf(&lexer_error, "Unexpected Token '%c'\n", c);
					return NULL;
				}
		}
	ScannerSkipWhitespace();
	}
	
	ScannerFree();
	Token** internal_token_list = (Token**)malloc(sizeof(Token*)*num_tokens);
	for (int i = 0; i < num_tokens; i++) {
		internal_token_list[i] = tokens[i];
	}
	TokenList* token_list = (TokenList*)malloc(sizeof(TokenList));
	token_list->tokens = internal_token_list;
	token_list->size = num_tokens;
	return token_list;
}

void TokenListFree(TokenList* token_list) {
	for (int i = 0; i < token_list->size; i++) {
		TokenFree(token_list->tokens[i]);
	}
	free(token_list);
}


/*
int main() {
	char* lines[] = {
		"BR HELLO",
		"WRITE WRRITE STORE SUB STTORE SUBB BLEQ BLT BR BLEQQ BLTT BRR BGT BGEQ BGEQQ R1 ADD DIV INC MUL flub MULflub R2 R3 23048 hi HALT",
		"LABEL: LOAD R1, R2",
		"BLT LABEL",
	};
	char* s = lines[0];
	TokenList* token_list = TokenizeLine(s);
	TokenListPrint(token_list);
	TokenListFree(token_list);
}
*/

