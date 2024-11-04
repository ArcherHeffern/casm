#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#include "preprocess.h"
#include "lexer.h"
#include "util.h"
#include "casm.h"
#include "casm_internal.h"


bool haltflag = false;

int num_labels = 0;
int num_label_jumps = 0; // Checks for possible infinite loops
char* label_names[MAX_LABELS] = { NULL };
int label_locations[MAX_LABELS] = { 0 };
int label_jump_counts[MAX_LABELS] = { 0 };

char* casm_error = NULL;

// Getters and Setters of 
int 	(*UIGetRegister)(int reg_num) = NULL;
char* 	(*UIGetMemory)(int address) = NULL;
char* 	(*UIGetStorage)(int address) = NULL;
void 	(*UISetRegister)(int reg_num, int value) = NULL;
void 	(*UISetMemory)(int address, char* value) = NULL;
void 	(*UISetStorage)(int address, char* value) = NULL;

// ============
// Entry Points
// ============
void PassUIGettersAndSetters(
	int 	(*_UIGetRegister)(int),
	char* 	(*_UIGetMemory)(int),
	char* 	(*_UIGetStorage)(int),
	void 	(*_UISetRegister)(int, int),
	void 	(*_UISetMemory)(int, char*),
	void 	(*_UISetStorage)(int, char*)
) {
	UIGetRegister = _UIGetRegister;
	UISetRegister = _UISetRegister;
	UIGetMemory = _UIGetMemory;
	UISetMemory = _UISetMemory;
	UIGetStorage = _UIGetStorage;
	UISetStorage = _UISetStorage;
}

bool LoadProgram(
	char** 	program, 
	int 	num_lines
	) {
	if (casm_error) {
		free(casm_error);
		casm_error = NULL;
	}
	for (int i = 0; i < num_labels; i++) {
		free(label_names[i]);
		label_names[i] = NULL;
		label_locations[i] = 0; 
		label_jump_counts[i] = 0;
	}
	num_labels = 0;
	num_label_jumps = 0;
	haltflag = false;

	num_labels = Preprocess(program, num_lines, label_names, label_locations);
	if (num_labels < 0) {
		char* error_msg;
		asprintf(&error_msg, "Preprocess error: %s", preprocess_error_msg);
		SetErrorMsg(error_msg);
		return false;
	}
	// Load memory
	for (int i = 0; i < num_lines; i++) {
		// SetMemory(i*4, program[i]);
		SetMemoryCellValue(i, program[i], RESET_DURATION, i*RESET_DELAY);
	}
	
	return true;
}

bool RunProgram() {
	while (StepProgram()) {}
	return casm_error == NULL;
}

void PrintErrorMsg() {
	if (!casm_error) {
		printf("Attempted to print error msg when there was no error\n");
		return;
	}
	int pc = GetProgramCounter()-1;
	printf("Error at address %d executing '%s'\n", pc*4, UIGetMemory(pc*4));
	printf("%s\n", casm_error);
}

bool StepProgram() {
	char* line = UIGetMemory(GetProgramCounter()*4);
	SetProgramCounter(GetProgramCounter()+1);
	if (!line) {
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

	ExecuteInstruction(&scanner);

	TokenListFree(token_list);

	if (num_label_jumps >= MAX_LABEL_JUMPS) {
		char* error_msg;
		asprintf(&error_msg, "%d jumps performed - Possible infinite loop\n\n%s", MAX_LABEL_JUMPS, PrintJumpLabelBreakdown());
		SetErrorMsg(error_msg);
	}

	bool can_continue = !casm_error && !haltflag;
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
	if (casm_error) {
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
	if (casm_error) {
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
void ExecuteInstruction(Scanner* scanner) {
	TokenType instruction = Advance(scanner)->type;
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
			haltflag = true;
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
			ExecuteBr(scanner);
			break;
		case TOKEN_BLT:
		case TOKEN_BGT:
		case TOKEN_BLEQ:
		case TOKEN_BGEQ:
		case TOKEN_BEQ:
		case TOKEN_BNEQ:
			ExecuteConditionalBranch(instruction, scanner);
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
}

void ExecuteMath(TokenType instruction, Scanner* scanner) {
	Register r1 = ScanRegister(scanner);
	Consume(scanner, TOKEN_COMMA);
	Register r2 = ScanRegister(scanner);
	if (casm_error) {
		return;
	}
	int op1 = GetRegister(r1.index);
	int op2 = GetRegister(r2.index);
	unsigned int result;
	if (instruction == TOKEN_ADD) {
		result = op1 + op2;
	} else if (instruction == TOKEN_SUB) {
		result = op1 - op2;
	} else if (instruction == TOKEN_MUL) {
		result = op1 * op2;
	} else if (instruction == TOKEN_DIV) {
		result = op1 / op2;
		unsigned int second_result = op1%op2;
		SetRegister(r2.index, (int)second_result);
	}
	SetRegister(r1.index, (int)result);
}

void ExecuteInc(Scanner* scanner) {
	Register r1 = ScanRegister(scanner);
	SetRegister(r1.index, r1.value+1);
}

void ExecuteLoad(Scanner* scanner) {
	Register r1 = ScanRegister(scanner);
	Consume(scanner, TOKEN_COMMA);
	int value = ScanLoadValue(scanner);
	if (!casm_error) {
		SetRegister(r1.index, value);
	}
}

void ExecuteStore(Scanner* scanner) {
	Register r1 = ScanRegister(scanner);
	Consume(scanner, TOKEN_COMMA);
	int address = ScanStoreAddress(scanner); 
	if (!casm_error) {
		char* str_value = IntToString(r1.value);
		SetMemory(address, str_value);
	}
}


void ExecuteRead(Scanner* scanner) {
	Register r1 = ScanRegister(scanner);
	Consume(scanner, TOKEN_COMMA);
	int value = ScanReadValue(scanner); 
	if (!casm_error) {
		SetRegister(r1.index, value);
	}
}


void ExecuteWrite(Scanner* scanner) {
	Register r1 = ScanRegister(scanner);
	Consume(scanner, TOKEN_COMMA);
	int address = ScanWriteAddress(scanner); 
	if (!casm_error) {
		char* str_value = IntToString(r1.value);
		SetStorage(address, str_value);
	}
}

void ExecuteBr(Scanner* scanner) {
	int index = ScanLabelIndex(scanner);
	if (casm_error) {
		return;
	}
	num_label_jumps++;
	label_jump_counts[index]++;
	SetProgramCounter(label_locations[index]);
}

void ExecuteConditionalBranch(TokenType jump_type, Scanner* scanner) {
	Register r1 = ScanRegister(scanner);
	Consume(scanner, TOKEN_COMMA);
	Register r2 = ScanRegister(scanner);
	Consume(scanner, TOKEN_COMMA);
	int index = ScanLabelIndex(scanner);

	if (casm_error) {
		return;
	}

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
	if (jump) {
		num_label_jumps++;
		label_jump_counts[index]++;
		SetProgramCounter(label_locations[index]);
	}
}
// ============
// Jump Helper
// ============
int ScanLabelIndex(Scanner* scanner) {
	Token* token = Consume(scanner, TOKEN_LABEL_REF);
	if (token == NULL) {
		return 0;
	}

	char* label_ref;
	asprintf(&label_ref, "%.*s", token->length, token->literal);
	for (int i = 0; i < num_labels; i++) {
		if (strcmp(label_ref, label_names[i]) == 0) {
			free(label_ref);
			return i;
		}
	}
	char* error_msg;
	asprintf(&error_msg, "Failed to resolve label '%s'", label_ref);
	free(label_ref);
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
	
	if (casm_error) {
		return 0;
	}

	return addr+r.value;
}


int ScanIndirectAddress(Scanner* scanner) {
	Advance(scanner);
	int address = ScanRegister(scanner).value;

	if (casm_error) {
		return 0;
	}

	return GetMemory(address);
}

int ScanRelativeAddress(Scanner* scanner) {
	Advance(scanner);
	int offset = ScanRegister(scanner).value;
	int pc = 4*(GetProgramCounter()-1);
	return offset+pc;
}

Register ScanRegister(Scanner* scanner) {
	// From R1->5 returns { .index=1, .value=.5 }
	Token* register_token = Consume(scanner, TOKEN_REGISTER);
	Register r;
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
	if (casm_error) {
		return 0;
	}
	return atoi(number_token->literal);
}

// ============
// Getters
// ============
int GetProgramCounter() {
	return UIGetRegister(0);
}

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

void SetErrorMsg(char* msg) {
	if (casm_error) {
		free(msg);
		return;
	}
	casm_error = msg;
}

// ============
// Debug Info
// ============
void PrintRegisters() {
	printf("PC: %d\n", GetProgramCounter());
	for (int i = 1; i < 10; i++) {
		printf("R%d: %d\n", i, GetRegister(i));
	}
}

void PrintMemory() {
	PrintMemoryRange(0, MEMORY_SIZE-1);
}

void PrintMemoryRange(int lower, int upper) {
	for (int i = lower/4; i < upper/4+1; i++) {
		printf("%d: %s\n", i*4, UIGetMemory(i*4));
	}
}

char* PrintJumpLabelBreakdown() {
	char* result = NULL;
    char *temp = NULL;    
	asprintf(&result, "Jumps to each label:");

    for (int i = 0; i < num_labels; i++) {
        asprintf(&temp, "\n%s: %d", label_names[i], label_jump_counts[i]);

		char *new_result;
		asprintf(&new_result, "%s%s", result, temp);
		free(result);  
		result = new_result;
		free(temp);    
    }
	return result;
}


// ============
// Main
// ============
int main() {
	SetupTests();
	MathTest();
	LoadTest();
	StoreTest();
	StorageTest();
	LoopTest();
	TearDownTests();
}

int *test_registers = NULL;
char** test_memory = NULL;
char** test_storage = NULL;
void SetupTests() {
	test_registers = malloc(sizeof(int)*(MAX_REGISTERS+1));
	test_memory = malloc(sizeof(char*)*MEMORY_SIZE);
	test_storage = malloc(sizeof(char*)*STORAGE_SIZE);
	
	memset(test_registers, 0, sizeof(int)*(MAX_REGISTERS+1));
	memset(test_memory, 0, sizeof(char*)*MEMORY_SIZE);
	memset(test_storage, 0, sizeof(char*)*STORAGE_SIZE);

	UIGetRegister = MockGetRegister;
	UIGetMemory = MockGetMemory;
	UIGetStorage = MockGetStorage;
	UISetRegister = MockSetRegister;
	UISetMemory = MockSetMemory;
	UISetStorage = MockSetStorage;
}

void TearDownTests() {
	free(test_registers);
	free(test_memory);
	free(test_storage);
}

int	MockGetRegister(int reg_num) {return test_registers[reg_num];}
char* MockGetMemory(int address) {return test_memory[address/4];}
char* MockGetStorage(int address) {return test_storage[address/4];}
void MockSetRegister(int reg_num, int value) {test_registers[reg_num]=value;}
void MockSetMemory(int address, char* value) {test_memory[address/4]=value;}
void MockSetStorage(int address, char* value) {test_storage[address/4]=value;}

void MathTest() {
	char* lines[] = {
		"LOAD R1, =10",
		"LOAD R2, =10",
		"LOAD R3, =10",
		"LOAD R4, =10",
		"LOAD R5, =10",
		"LOAD R6, =5 ; Operand for all math",
		"ADD R1, R6",
		"SUB R2, R6",
		"MUL R3, R6",
		"DIV R4, R6",
		"INC R5",
		"HALT",
	};
	int num_lines = sizeof(lines)/sizeof(char*);
	if (!LoadProgram(lines, num_lines)) {
		PrintErrorMsg();
		return;
	}
	if (!RunProgram()) {
		PrintErrorMsg();
		return;
	}
	
	assert(GetRegister(1) == 15 && "10 + 5 == 15");
	assert(GetRegister(2) == 5 && "10 - 5 == 5");
	assert(GetRegister(3) == 50 && "10 * 5 == 50");
	assert(GetRegister(4) == 2 && "10 // 5 == 2");
	assert(GetRegister(6) == 0 && "10 % 5 == 0");
	assert(GetRegister(5) == 11 && "INC 10 == 11");
}


void LoadTest() {
	char* lines[] = {
		"LOAD R1, =8",
		"LOAD R2, R1",
		"LOAD R3, [72, R1]", // Expect 28
		"LOAD R4, =80",
		"LOAD R5, @R4", // Expect 21
		"LOAD R6, $R1", // Expect 21
		"HALT",
		"21"
	};
	int num_lines = sizeof(lines)/sizeof(char*);
	if (!LoadProgram(lines, num_lines)) {
		PrintErrorMsg();
		return;
	}
	SetMemory(80, "28"); // Override for index and indirect addressing
	if (!RunProgram()) {
		PrintErrorMsg();
	}
	assert(GetRegister(1) == 8);
	assert(GetRegister(2) == 8);
	assert(GetRegister(3) == 28);
	assert(GetRegister(4) == 80);
	assert(GetRegister(5) == 21);
	assert(GetRegister(6) == 21);
}


void StoreTest() {
	char* lines[] = {
		"LOAD R1, =100",
		"LOAD R2, =48",
		"LOAD R3, =4",
		"LOAD R4, =8",
		"STORE R1, R2", // M48:100
		"ADD R1, R3",
		"STORE R1, [4, R2]", // M52:104
		"ADD R1, R3",
		"STORE R1, $R4", // After haltflag: 108
		"HALT"
	};
	int num_lines = sizeof(lines)/sizeof(char*);
	if (!LoadProgram(lines, num_lines)) {
		PrintErrorMsg();
		return;
	}
	if (!RunProgram()) {
		PrintErrorMsg();
		return;
	}
	assert(UIGetMemory(48) != NULL && "Memory at address 48 is not null");
	assert(GetMemory(48) == 100 && "Memory at address 48 is 100");
	assert(UIGetMemory(52) != NULL && "Memory at address 52 is not null");
	assert(GetMemory(52) == 104 && "Memory at address 52 is 104");
	assert(UIGetMemory(num_lines*4) != NULL && "Memory at address after haltflag is not null");
	assert(GetMemory(num_lines*4) == 108 && "Memory at after haltflag is 108");
}


void StorageTest() {
	char* lines[] = {
		"LOAD R1, =100",
		"LOAD R2, =24 ; Disk write address",
		"LOAD R3, =4",
		"WRITE R1, R2",
		"READ R4, R2",
		"ADD R1, R3",
		"WRITE R1, [4, R2] ; S: 28 -> 104",
		"READ R5, [4, R2]; R5 -> 104",
		"HALT"
	};
	int num_lines = sizeof(lines)/sizeof(char*);
	if (!LoadProgram(lines, num_lines)) {
		PrintErrorMsg();
		return;
	}
	if (!RunProgram()) {
		PrintErrorMsg();
	}

	assert(UIGetStorage(24) != NULL && "Storage at address 24 is not null");
	assert(GetStorage(24) == 100 && "Storage at address 24 is 100");
	assert(GetRegister(4) == 100 && "R4 is 100");
	assert(UIGetStorage(28) != NULL && "Storage at address 28 is not null");
	assert(GetStorage(28) == 104 && "Storage at address 28 is 104");
	assert(GetRegister(5) == 104 && "R5 is 104");
}


void LoopTest() {
	char* lines[] = {
		"			LOAD R1, =0",
		"			LOAD R2, =10",
		"Label: 	BGEQ R1, R2, Label2", 
		"			INC R1 ",
		"			BR Label",
		"Label2:	HALT"
	};
	LoadProgram(lines, sizeof(lines)/sizeof(char*));
	
	if (!RunProgram()) {
		PrintErrorMsg();
	}
	printf("%s\n", PrintJumpLabelBreakdown());
}
