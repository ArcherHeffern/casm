
#ifndef TEST_H_
#define TEST_H_

#include <stdbool.h>
#define MAX_TOKENS 16

extern char* lexer_error;

typedef struct Token Token;
typedef enum TokenType TokenType;
typedef struct TokenList TokenList;

enum TokenType {
	TOKEN_EQUAL,
	TOKEN_R_BRACKET,
	TOKEN_L_BRACKET,
	TOKEN_AT,
	TOKEN_DOLLAR,
	TOKEN_COMMA,

	TOKEN_LOAD,
	TOKEN_STORE,
	TOKEN_READ,
	TOKEN_WRITE,

	TOKEN_ADD,
	TOKEN_SUB,
	TOKEN_MUL,
	TOKEN_DIV,
	TOKEN_INC,

	TOKEN_BR,
	TOKEN_BLT,
	TOKEN_BGT,
	TOKEN_BLEQ,
	TOKEN_BGEQ,
	TOKEN_BEQ,
	TOKEN_BNEQ,

	TOKEN_LABEL_REF,
	TOKEN_REGISTER,
	TOKEN_NUMBER,

	TOKEN_HALT,
	TOKEN_NONE
};

extern char* TokenTypeToString[];

struct Token {
	TokenType type;
	char* literal; // Pointer to original text - Aka not malloced
	int length; // Length of actual string
};

struct TokenList {
	Token** tokens;
	int size;
};

TokenList* TokenizeLine(char* s);
void TokenListFree(TokenList* token_list);
void TokenDbg(Token* token);
void TokenListPrint(TokenList* token_list);

#endif // TEST_H_
