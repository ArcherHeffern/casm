#include <stdlib.h>
#include <stdio.h>
#include "preprocess.h"
#include "lexer.h"

#define MAX_LABELS 16

int registers[10] = { 0 };
char* memory[64] = { NULL };
char* storage[64] = { NULL };

char* casm_error = NULL;

typedef struct RegisterPair {
	int first;
	int second;
} RegisterPair;

typedef struct Scanner {
	TokenList* token_list;
	int cur;
} Scanner;

// Entry Points
bool StepProgram();
bool ExecuteInstruction(Scanner* scanner);
// Scanner
Token* Advance(Scanner* scanner);
Token* Check(Scanner* scanner, TokenType token_type);
Token* Consume(Scanner* scanner, TokenType token_type);
Token* Peek(Scanner* scanner);
Token* Prev(Scanner* scanner);
bool IsAtEnd(Scanner* scanner);
// Executors
RegisterPair* ParseRegisterPair(Scanner* scanner);
bool ExecuteLoad(Scanner* scanner);
int ResolveDirectAddress();
int ResolveImmediateAddress();
int ResolveIndexAddress();
int ResolveIndirectAddress();
int ResolveLoadAddress();
int ResolveRelativeAddress();
// Setters
void SetRegister(int num, int value);
void SetMemory(int num, char* value, bool set_focused);
void SetStorage(int num, char* value);
// Debug Info
void PrintRegisters();
// Main
int main();


// ============
// Entry Points
// ============
bool StepProgram() {
	TokenList* token_list = TokenizeLine(memory[registers[0]++]);
	TokenListPrint(token_list);
	Scanner scanner;
	scanner.token_list = token_list;
	scanner.cur = 0;

	bool can_continue = ExecuteInstruction(&scanner);

	TokenListFree(token_list);
	return can_continue;
}

bool ExecuteInstruction(Scanner* scanner) {
	TokenType instruction = Advance(scanner)->type;
	switch (instruction) {
		case TOKEN_LOAD:
			if (!ExecuteLoad(scanner)) {
				return false;
			}
		case TOKEN_ADD:
		case TOKEN_SUB:
		case TOKEN_MUL:
		case TOKEN_DIV: {
			RegisterPair* rp = ParseRegisterPair(scanner);
			if (rp == NULL) {
				return false;
			}
			int first_val = registers[rp->first];
			int second_val = registers[rp->second];
			int result;
			if (instruction == TOKEN_ADD) {
				result = first_val + second_val;
			} else if (instruction == TOKEN_SUB) {
				result = first_val - second_val;
			} else if (instruction == TOKEN_MUL) {
				result = first_val * second_val;
			} else if (instruction == TOKEN_DIV) {
				result = first_val / second_val;
				SetRegister(rp->second, first_val % second_val);
			}
			printf("Result: %d\n", result);
			SetRegister(rp->first, result);
			free(rp);
			break;
		}
	}
	return IsAtEnd(scanner);
}
// ============
// Scanner
// ============
Token* Advance(Scanner* scanner) {
	Token* token = Peek(scanner);
	if (token == NULL) {
		return NULL;
	}
	scanner->cur++;
	return token;
}


Token* Check(Scanner* scanner, TokenType token_type) {
	if (casm_error) {
		return NULL;
	}
	Token* token = Peek(scanner);
	if (!token || token->type != token_type) {
		casm_error = malloc(64);
		asprintf(&casm_error, "Expected %s but found %s\n", TokenTypeToString[token_type], TokenTypeToString[token||TOKEN_NONE]);
		return NULL;
	}
	return token;
}


Token* Consume(Scanner* scanner, TokenType token_type) {
	if (casm_error) {
		return NULL;
	}
	Token* token = Advance(scanner);
	if (!token || token->type != token_type) {
		casm_error = malloc(64);
		asprintf(&casm_error, "Expected %s but found %s\n", TokenTypeToString[token_type], TokenTypeToString[token||TOKEN_NONE]);
		
		return NULL;
	}
	return token;
}


Token* Peek(Scanner* scanner) {
	if (IsAtEnd(scanner)) {
		return NULL;
	}
	return scanner->token_list->tokens[scanner->cur];
}


Token* Prev(Scanner* scanner) {
	return scanner->token_list->tokens[scanner->cur-1];
}


bool IsAtEnd(Scanner* scanner) {
	return scanner->cur == scanner->token_list->size;
}


// ============
// Executors
// ============
RegisterPair* ParseRegisterPair(Scanner* scanner) {
	Token* register_1 = Consume(scanner, TOKEN_REGISTER);
	Token* comma = Consume(scanner, TOKEN_COMMA);
	Token* register_2 = Consume(scanner, TOKEN_REGISTER);

	if (casm_error) {
		return NULL;
	}
	RegisterPair* rp = (RegisterPair*)malloc(sizeof(RegisterPair));
	rp->first = register_1->literal[1] - '0';
	rp->second = register_2->literal[1] - '0';
	return rp;
}


bool ExecuteLoad(Scanner* scanner) {
	return false;
}


int ResolveDirectAddress() {return 0;}


int ResolveImmediateAddress() {return 0;}


int ResolveIndexAddress() {return 0;}


int ResolveIndirectAddress() {return 0;}


// All return address+1 OR 0 on failure
int ResolveLoadAddress() {
	 return ResolveDirectAddress()
		|| ResolveImmediateAddress()
		|| ResolveIndexAddress()
		|| ResolveIndirectAddress()
		|| ResolveRelativeAddress();
}


int ResolveRelativeAddress() {return 0;}


// ============
// Setters
// ============
// TODO: Add animations
void SetRegister(int num, int value) {
	registers[num] = value;
}


void SetMemory(int num, char* value, bool set_focused) {
	memory[num] = value;
}


void SetStorage(int num, char* value){
	memory[num] = value;
} 

// ============
// Debug Info
// ============
void PrintRegisters() {
	printf("PC: %d\n", registers[0]);
	for (int i = 1; i < 10; i++) {
		printf("R%d: %d\n", i, registers[i]);
	}
}


// ============
// Main
// ============
int main() {
	int num_lines = 1;
	/*
	char* lines[] = {
		"			LOAD R1 =0",
		"			LOAD R2, =10",
		"Label: 	BGEQ R1, R2, Label2", 
		"			ADD R2, R1",
		"			INC R1 ",
		"Label2:	BR Label",
		"HALT"
	};
	*/
	char* lines[] = {
		"ADD R1, R2"
	};
	char* label_names[MAX_LABELS];
	int label_locations[MAX_LABELS];
	int num_labels = Preprocess(lines, num_lines, label_names, label_locations);
	if (num_labels < 0) {
		fprintf(stderr, "ERROR: %s\n", preprocess_error_msg);
		return 0;
	}
	// Load memory
	for (int i = 0; i < num_lines; i++) {
		memory[i] = lines[i];
	}
	registers[2] = 1;

	if (!StepProgram()) {
		printf("%s\n", casm_error);
	}
	PrintRegisters();
	// while (StepProgram()) {}
	return 0;
}
