#include <string.h>
#include <stdio.h>
#include <assert.h>

#include "casm.h"
#include "ui.h"
#include "ui_internal.h"

State* s = NULL;
char* ErrorMsg = NULL;

void Run(char* filename) {
    s = malloc(sizeof(State));
    assert(s != NULL);
    memset(s, 0, sizeof(State));
	for (int i = 0; i < 10; i++) {
		s->registers[i] = (int*)malloc(sizeof(int));
		*s->registers[i] = 0;
	}
    int num_lines;
    char** program = FileReadLines(filename, &num_lines, MEMORY_SIZE, SetErrorMsg);
    if (program == NULL) {
        printf("%s\n", GetErrorMsg());
        exit(1);
    }
    LoadProgram(program, num_lines);
    RunProgram();
}

// ============
// User Action Handlers
// ============
bool LoadProgram(char** program, int num_lines) {
	Preprocess(&s->label_state, program, num_lines);
	if (HasError()) {
		return false;
	}
	for (int i = 0; i < num_lines; i++) {
        UISetMemory(i*4, program[i]);
	}
	return !HasError();
}

int GetProgramCounter() {
    return *s->registers[0];
}
int UIGetRegister(int reg_num) {
    return *s->registers[reg_num];
}

char* UIGetMemory(int address) {
    return s->memory[address/4];
}

char* UIGetStorage(int address) {
    return s->storage[address/4];
}

void UISetRegister(int reg_num, int value) {
    int* v = malloc(sizeof(int));
    assert(v != NULL && "Failed to allocated memory for Register");
    *v = value;
    s->registers[reg_num] = v;
}

void UISetMemory(int address, char* value) {
    s->memory[address/4] = value;
}

void UISetStorage(int address, char* value) {
    s->storage[address/4] = value;
}

bool GetHaltflag() {
    return s->haltflag;
}

char* GetErrorMsg() {
    return s->error_msg;
}

void SetHaltflag(bool flag) {
    s->haltflag = flag;
}

void SetErrorMsg(char* msg) {
    if (s->error_msg != NULL) {
        free(msg);
        return;
    }
    s->error_msg = msg;
}

bool HasError() {
    return s->error_msg != NULL;
}

// Debug
void PrintRegisters() {
	printf("PC: %d\n", GetProgramCounter());
	for (int i = 1; i < 10; i++) {
		printf("R%d: %d\n", i, UIGetRegister(i));
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



void PrintErrorMsg() {
	if (!HasError()) {
		printf("Attempted to print error msg when there was no error\n");
		return;
	}
	int pc = GetProgramCounter();
	printf("Error at address %d executing '%s'\n", pc*4, UIGetMemory(pc*4)?UIGetMemory(pc*4): "000000");
	printf("%s\n", GetErrorMsg());
}
