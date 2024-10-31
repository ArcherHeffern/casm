#include <math.h>
#include <string.h>
#include <strings.h>
#include <stdio.h>
#include <stdlib.h>

#include "interpreter.h"

Token tokens[MAX_TOKENS];
size_t tokens_added;
Scanner* scanner = NULL;


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
	Token* token = &tokens[tokens_added];
	token->type = token_type;
	token->literal = &scanner->s[scanner->start];
	token->length = scanner->cur-scanner->start;
	scanner->start = scanner->cur;
	scanner->cur = scanner->start;
	tokens_added++;
}

void ScannerInit(char* s) {
	scanner = (Scanner*)malloc(sizeof(Scanner));
	memset(scanner, 0, sizeof(Scanner));
	scanner->s = s;
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

void TokensPrint() {
	for (int i = 0; i < tokens_added; i++) {
		TokenDbg(&tokens[i]);
	}
}

bool IsDigit(char c) {
	return c <='9' && c >= '0';
}

bool IsAlpha(char c) {
	return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
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
	if (scanner->cur - scanner->start - pos != len) {
		return TOKEN_LABEL_REF;
	}
	if (strncasecmp(&scanner->s[scanner->start+pos], s, len) == 0) {
		return token;
	}
	return TOKEN_LABEL_REF;
}

TokenType ScannerParseIdentifier() {
	switch (scanner->s[scanner->start]) {
		case 'A':
			return ScannerCheckRest(1, "DD", 2, TOKEN_ADD);
		case 'D':
			return ScannerCheckRest(1, "IV", 2, TOKEN_DIV);
		case 'I':
			return ScannerCheckRest(1, "NC", 2, TOKEN_INC);
		case 'M':
			return ScannerCheckRest(1, "UL", 2, TOKEN_MUL);
		case 'W':
			return ScannerCheckRest(1, "RITE", 4, TOKEN_WRITE);
		default:
			return TOKEN_LABEL_REF;
			
	}
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

void Tokenize() {
	while (!ScannerAtEnd()) {
		ScannerSkipWhitespace();
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
					printf("Unexpected Token");
					return;
				}
		}
	}
}

void Preprocess(char** lines) {
	// Handle Labels
}

#define MAX_LABELS 8
char* label_names[MAX_LABELS];
int label_lines[MAX_LABELS];

int main() {
	char* lines[] = {
		"R1 ADD DIV INC MUL flub MULflub R2 R3 23048 hi",
		"LABEL: LOAD R1, R2",
		"BLT LABEL",
	};
	Preprocess(lines);
	char* s = lines[0];
	// s = "5[]$=100=,10";
	ScannerInit(s);
	Tokenize();
	printf("Tokens added: %zu\n", tokens_added);
	TokensPrint();

	// Cleanup
	ScannerFree();
	/*
		FILE* f = fopen("test.a", "r");
		size_t s = 64;
		char* token = NULL;
		char* line = malloc(s);
		getline(&line, &s, f);
		token = strsep(&line, " ");
		printf("%s\n", token);

		if (eq(token, "ADD")) {
		} else if (eq(token, "SUB")) {
		}
	*/	
}
