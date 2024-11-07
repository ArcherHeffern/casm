#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#include "lexer.h"
#include "util.h"
#include "casm.h"
#include "ui.h"
#include "casm_internal.h"


// ============
// Entry Points
// ============
bool RunProgram() {
	while (StepProgram()) {}
	return !HasError();
}


bool StepProgram() {
	char* line = UIGetMemory(GetProgramCounter()*4);
	if (!line || line[0] == '\0') {
		char* error_msg;
		asprintf(&error_msg, "Expected instruction but found garbage");
		SetErrorMsg(error_msg);
		return false;
	}
	TokenList* token_list = TokenizeLine(line);
	if (token_list == NULL) {
		char* error_msg;
		asprintf(&error_msg, "Lexer Error: %s", lexer_error);
		SetErrorMsg(error_msg);
		return false;
	}
	Scanner scanner = { token_list, 0 };

	int new_pc = ExecuteInstruction(&scanner);

	TokenListFree(token_list);

	bool can_continue = !HasError() && !GetHaltflag();
	if (can_continue) {
		SetProgramCounter(new_pc);
	}
	return can_continue;
}

// ============
// Scanner
// ============
Token* Peek(Scanner* scanner) {
	if (IsAtEnd(scanner)) {
		return NULL;
	}
	return scanner->token_list->tokens[scanner->cur];
}


Token* Advance(Scanner* scanner) {
	Token* token = Peek(scanner);
	if (token == NULL) {
		return NULL;
	}
	scanner->cur++;
	return token;
}


Token* Check(Scanner* scanner, TokenType token_type) {
	if (HasError()) {
		return NULL;
	}
	Token* token = Peek(scanner);
	if (!token || token->type != token_type) {
		char* error_msg;
		asprintf(&error_msg, "Expected %s but found %s", 
			TokenTypeToString[token_type], 
			TokenTypeToString[token?token->type:TOKEN_NONE]
		);
		SetErrorMsg(error_msg);
		return NULL;
	}
	return token;
}


Token* Consume(Scanner* scanner, TokenType token_type) {
	if (HasError()) {
		return NULL;
	}
	Token* token = Advance(scanner);
	if (!token || token->type != token_type) {
		char* error_msg;
		asprintf(&error_msg, "Expected %s but found %s", 
			TokenTypeToString[token_type], 
			TokenTypeToString[token?token->type: TOKEN_NONE]
		);
		SetErrorMsg(error_msg);
		
		return NULL;
	}
	return token;
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
int ExecuteInstruction(Scanner* scanner) {
	TokenType instruction = Advance(scanner)->type;
	int new_pc = GetProgramCounter() + 1;
	switch (instruction) {
		case TOKEN_LOAD:
			ExecuteLoad(scanner);
			break;
		case TOKEN_STORE:
			ExecuteStore(scanner);
			break;
		case TOKEN_READ:
			ExecuteRead(scanner);
			break;
		case TOKEN_WRITE:
			ExecuteWrite(scanner);
			break;
		case TOKEN_HALT:
			SetHaltflag(true);
			break;
		case TOKEN_ADD:
		case TOKEN_SUB:
		case TOKEN_MUL:
		case TOKEN_DIV: 
			ExecuteMath(instruction, scanner);
			break;
		case TOKEN_INC:
			ExecuteInc(scanner);
			break;
		case TOKEN_BR:
			new_pc = ExecuteBr(scanner);
			break;
		case TOKEN_BLT:
		case TOKEN_BGT:
		case TOKEN_BLEQ:
		case TOKEN_BGEQ:
		case TOKEN_BEQ:
		case TOKEN_BNEQ:
			new_pc = ExecuteConditionalBranch(instruction, scanner);
			break;
		default: {
			char* error_msg;
			asprintf(&error_msg, "Unexpected token while resolving instruction: %s", TokenTypeToString[instruction]);
			SetErrorMsg(error_msg);
		}
	}
	if (!IsAtEnd(scanner)) {
		char* error_msg;
		asprintf(&error_msg, "Too many tokens on this line");
		SetErrorMsg(error_msg);
	}
	return new_pc;
}

void ExecuteMath(TokenType instruction, Scanner* scanner) {
	Register r1 = ScanRegister(scanner);
	Consume(scanner, TOKEN_COMMA);
	Register r2 = ScanRegister(scanner);
	if (HasError()) {
		return;
	}
	int op1 = GetRegister(r1.index);
	int op2 = GetRegister(r2.index);
	int result = 0;
	if (instruction == TOKEN_ADD) {
		result = op1 + op2;
	} else if (instruction == TOKEN_SUB) {
		result = op1 - op2;
	} else if (instruction == TOKEN_MUL) {
		result = op1 * op2;
	} else { // TOKEN_DIV
		result = op1 / op2;
		int second_result = op1%op2;
		SetRegister(r2.index, MaxInt(second_result, 0));
	}
	SetRegister(r1.index, MaxInt(result, 0));
}

void ExecuteInc(Scanner* scanner) {
	Register r1 = ScanRegister(scanner);
	SetRegister(r1.index, r1.value+1);
}

void ExecuteLoad(Scanner* scanner) {
	Register r1 = ScanRegister(scanner);
	Consume(scanner, TOKEN_COMMA);
	int value = ScanLoadValue(scanner);
	if (HasError()) {
		return;
	}
	SetRegister(r1.index, value);
}

void ExecuteStore(Scanner* scanner) {
	Register r1 = ScanRegister(scanner);
	Consume(scanner, TOKEN_COMMA);
	int address = ScanStoreAddress(scanner); 
	if (HasError()) {
		return;
	}
	char* str_value = IntToString(r1.value);
	SetMemory(address, str_value);
}


void ExecuteRead(Scanner* scanner) {
	Register r1 = ScanRegister(scanner);
	Consume(scanner, TOKEN_COMMA);
	int value = ScanReadValue(scanner); 
	if (HasError()) {
		return;
	}
	SetRegister(r1.index, value);
}


void ExecuteWrite(Scanner* scanner) {
	Register r1 = ScanRegister(scanner);
	Consume(scanner, TOKEN_COMMA);
	int address = ScanWriteAddress(scanner); 
	if (HasError()) {
		return;
	}
	char* str_value = IntToString(r1.value);
	SetStorage(address, str_value);
}

int ExecuteBr(Scanner* scanner) {
	return ScanLabelIndex(scanner, true);
}

int ExecuteConditionalBranch(TokenType jump_type, Scanner* scanner) {
	Register r1 = ScanRegister(scanner);
	Consume(scanner, TOKEN_COMMA);
	Register r2 = ScanRegister(scanner);
	Consume(scanner, TOKEN_COMMA);

	bool jump = false;
	int op1 = r1.value;
	int op2 = r2.value;
	switch (jump_type) {
		case TOKEN_BLT:
			jump = op1 < op2;
			break;
		case TOKEN_BGT:
			jump = op1 > op2;
			break;
		case TOKEN_BLEQ:
			jump = op1 <= op2;
			break;
		case TOKEN_BGEQ:
			jump = op1 >= op2;
			break;
		case TOKEN_BEQ:
			jump = op1 == op2;
			break;
		case TOKEN_BNEQ:
			jump = op1 != op2;
			break;
	}
	int index = ScanLabelIndex(scanner, jump);

	if (jump) {
		return index;
	}
	return GetProgramCounter()+1;
}
// ============
// Jump Helper
// ============
int ScanLabelIndex(Scanner* scanner, bool increment_count) {
	Token* token = Consume(scanner, TOKEN_LABEL_REF);
	if (token == NULL || !increment_count) {
		return 0;
	}

	char* label_ref = NULL;
	asprintf(&label_ref, "%.*s", token->length, token->literal);
	int address = GetLabelAddress(label_ref);
	if (address != -1) {
		return address;
	}
	char* error_msg;
	asprintf(&error_msg, "Failed to resolve label '%s'", label_ref);
	SetErrorMsg(error_msg);
	return -1;
}


// ============
// Addressing Combinations
// ============
int ScanLoadValue(Scanner* scanner) {
	Token* token = Peek(scanner);
	if (token == NULL) goto err;
	switch (token->type) {
		case TOKEN_REGISTER:
			return ScanDirectAddress(scanner);
		case TOKEN_EQUAL:
			return ScanImmediateAddressValue(scanner);
		case TOKEN_L_BRACKET:
			return GetMemory(ScanIndexAddress(scanner));
		case TOKEN_AT:
			return GetMemory(ScanIndirectAddress(scanner));
		case TOKEN_DOLLAR:
			return GetMemory(ScanRelativeAddress(scanner));
	}
	err: {
		char* error_msg;
		asprintf(&error_msg, "Unexpected token %s while resolving load value", 
			TokenTypeToString[token?token->type: TOKEN_NONE]
		);
		SetErrorMsg(error_msg);
		return 0;
	}
}


int ScanStoreAddress(Scanner* scanner) {
	Token* token = Peek(scanner);
	if (token == NULL) goto err;
	switch (token->type) {
		case TOKEN_REGISTER:
			return ScanDirectAddress(scanner);
		case TOKEN_L_BRACKET:
			return ScanIndexAddress(scanner);
		case TOKEN_DOLLAR:
			return ScanRelativeAddress(scanner);
	}
	err: {
		char* error_msg;
		asprintf(&error_msg, "Unexpected token %s while resolving store value", 
			TokenTypeToString[token?token->type: TOKEN_NONE]
		);
		SetErrorMsg(error_msg);
		return 0;
	}
}

int ScanReadValue(Scanner* scanner) {
	Token* token = Peek(scanner);
	if (token == NULL) goto err;
	switch (token->type) {
		case TOKEN_REGISTER:
			return GetStorage(ScanDirectAddress(scanner));
		case TOKEN_L_BRACKET:
			return GetStorage(ScanIndexAddress(scanner));
	}
	err: {
		char* error_msg;
		asprintf(&error_msg, "Unexpected token %s while resolving read value", 
			TokenTypeToString[token?token->type: TOKEN_NONE]
		);
		SetErrorMsg(error_msg);
		return 0;
	}
}


int ScanWriteAddress(Scanner* scanner) {
	Token* token = Peek(scanner);
	if (token == NULL) goto err;
	switch (token->type) {
		case TOKEN_REGISTER:
			return ScanDirectAddress(scanner);
		case TOKEN_L_BRACKET:
			return ScanIndexAddress(scanner);
	}
	err: {
		char* error_msg;
		asprintf(&error_msg, "Unexpected token %s while resolving write value", 
			TokenTypeToString[token?token->type: TOKEN_NONE]
		);
		SetErrorMsg(error_msg);
		return 0;
	}
}


// ============
// Addressing Primatives
// ============
int ScanDirectAddress(Scanner* scanner) {
	return ScanRegister(scanner).value;
}


int ScanImmediateAddressValue(Scanner* scanner) {
	Advance(scanner);
	return ScanNumberValue(scanner);
}


int ScanIndexAddress(Scanner* scanner) {
	Advance(scanner);
	int addr = ScanNumberValue(scanner);
	Consume(scanner, TOKEN_COMMA);
	Register r = ScanRegister(scanner);
	Consume(scanner, TOKEN_R_BRACKET);
	
	if (HasError()) {
		return 0;
	}

	return addr+r.value;
}


int ScanIndirectAddress(Scanner* scanner) {
	Advance(scanner);
	int address = ScanRegister(scanner).value;

	if (HasError()) {
		return 0;
	}

	return GetMemory(address);
}

int ScanRelativeAddress(Scanner* scanner) {
	Advance(scanner);
	int offset = ScanRegister(scanner).value;
	int pc = 4*(GetProgramCounter());
	return offset+pc;
}

Register ScanRegister(Scanner* scanner) {
	// From R1->5 returns { .index=1, .value=.5 }
	Token* register_token = Consume(scanner, TOKEN_REGISTER);
	Register r = { 0 };
	if (!register_token) {
		return r;
	}
	r.index = register_token->literal[1] - '0';
	r.value = GetRegister(r.index);
	return r;
}

int ScanNumberValue(Scanner* scanner) {
	// From Token {8} -> 8
	Token* number_token = Consume(scanner, TOKEN_NUMBER);
	if (HasError()) {
		return 0;
	}
	return atoi(number_token->literal);
}

// ============
// Getters
// ============
int GetRegister(int reg_num) {
	if (reg_num > MAX_REGISTERS || reg_num < 1) {
		char* error_msg;
		asprintf(&error_msg, "General purpose registers range from 1-%d. Getting nonexistant register %d",
			MAX_REGISTERS,
			reg_num);
		SetErrorMsg(error_msg);
		return 0;
	}
	return UIGetRegister(reg_num);
}

int GetMemory(int address) {
	// M:[0, 1, 2, 3, 4]
	// GetMemory(4) -> 1
	if (address % 4 >= MEMORY_SIZE) {
		char* error_msg;
		asprintf(&error_msg, "Memory address '%d' greater than max memory size '%d'", address, MEMORY_SIZE);
		SetErrorMsg(error_msg);
		return 0;
	}
	if (address % 4 != 0) {
		char* error_msg;
		asprintf(&error_msg, "Expected address to be multiple of 4: 0x%d", address);
		SetErrorMsg(error_msg);
		return 0;
	}
	char* line = UIGetMemory(address);
	int contents = 0;
	if (line == NULL || !ToInteger(line, &contents)) {
		char* error_msg;
		asprintf(&error_msg, "Cannot read memory address %d since it contains garbage or a non positive integer: '%s'\nWhile this is *Technically* valid, since every memory address is actually just numbers being interpreted as instructions and whatnot, I'm assuming this is not what you were intending.", address, line);
		SetErrorMsg(error_msg);
		return 0;
	}
	return contents;
}


int GetStorage(int address) {
	// S:[0, 1, 2, 3, 4]
	// GetMemory(4) -> 1
	if (address % 4 >= STORAGE_SIZE) {
		char* error_msg;
		asprintf(&error_msg, "Storage address '%d' greater than max storage size '%d'", address, STORAGE_SIZE);
		SetErrorMsg(error_msg);
		return 0;
	}
	if (address % 4 != 0) {
		char* error_msg;
		asprintf(&error_msg, "Expected address to be multiple of 4: 0x%d", address);
		SetErrorMsg(error_msg);
		return 0;
	}
	char* line = UIGetStorage(address);
	int contents = 0;
	if (line == NULL || !ToInteger(line, &contents)) {
		char* error_msg;
		asprintf(&error_msg, "Cannot read storage address %d since it contains garbage or a non positive integer: '%s'\nWhile this is *Technically* valid, since every storage address is actually just numbers being interpreted as instructions and whatnot, I'm assuming this is not what you were intending.", address, line);
		SetErrorMsg(error_msg);
		return 0;
	}
	return contents;
}

// ============
// Setters
// ============
// TODO: Add animations and enforce validations
void SetProgramCounter(int pc) {
	if (pc < 0 || pc >= MEMORY_SIZE) {
		char* error_msg;
		asprintf(&error_msg, "Program Counter exceeded max memory size %d", MEMORY_SIZE);
		return;
	}
	UISetRegister(0, pc);
}

void SetRegister(int reg_num, int value) {
	if (reg_num > MAX_REGISTERS || reg_num < 1) {
		char* error_msg;
		asprintf(&error_msg, "General purpose registers range from 1-%d. Used nonexistant register %d",
			MAX_REGISTERS,
			reg_num);
		SetErrorMsg(error_msg);
		return;
	}
	UISetRegister(reg_num, value);
}


void SetMemory(int address, char* value) {
	if (address % 4 >= MEMORY_SIZE) {
		char* error_msg;
		asprintf(&error_msg, "Memory address '%d' greater than max memory size '%d'", address, MEMORY_SIZE);
		SetErrorMsg(error_msg);
		return;
	}
	if (address % 4 != 0) {
		char* error_msg;
		asprintf(&error_msg, "Expected address to be multiple of 4: 0x%d", address);
		SetErrorMsg(error_msg);
		return;
	}
	UISetMemory(address, value);
}


void SetStorage(int address, char* value) {
	if (address % 4 >= STORAGE_SIZE) {
		char* error_msg;
		asprintf(&error_msg, "Storage address '%d' greater than max storage size '%d'", address, MEMORY_SIZE);
		SetErrorMsg(error_msg);
		return;
	}
	if (address % 4 != 0) {
		char* error_msg;
		asprintf(&error_msg, "Expected address to be multiple of 4: 0x%d", address);
		SetErrorMsg(error_msg);
		return;
	}
	UISetStorage(address, value);
} 
