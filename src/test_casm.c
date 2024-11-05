#include "ui_internal.h"
#include "casm.h"

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

int	MockGetRegister(int reg_num) {return test_registers[reg_num];}
char* MockGetMemory(int address) {return test_memory[address/4];}
char* MockGetStorage(int address) {return test_storage[address/4];}
void MockSetRegister(int reg_num, int value) {test_registers[reg_num]=value;}
void MockSetMemory(int address, char* value) {test_memory[address/4]=value;}
void MockSetStorage(int address, char* value) {test_storage[address/4]=value;}

void SetupTests() {
	test_registers = malloc(sizeof(int)*(MAX_REGISTERS+1));
	test_memory = malloc(sizeof(char*)*MEMORY_SIZE);
	test_storage = malloc(sizeof(char*)*STORAGE_SIZE);
	
	memset(test_registers, 0, sizeof(int)*(MAX_REGISTERS+1));
	memset(test_memory, 0, sizeof(char*)*MEMORY_SIZE);
	memset(test_storage, 0, sizeof(char*)*STORAGE_SIZE);
}

void TearDownTests() {
	free(test_registers);
	free(test_memory);
	free(test_storage);
}


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
