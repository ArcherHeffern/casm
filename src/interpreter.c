#include <string.h>
#include <stdio.h>
#include <assert.h>

#include "casm.h"
#include "ui.h"
#include "ui_internal.h"
#include "preprocess.h"

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
    char** program = FileReadLines(filename, &num_lines);
    LoadProgram(program, num_lines);
    RunProgram();
}

// ============
// User Action Handlers
// ============
bool LoadProgram(char** program, int num_lines) {
	LabelState* ls = &s->label_state;
	ls->count = Preprocess(program, num_lines, ls->label_names, ls->label_locations);
	if (ls->count < 0) {
		char* error_msg;
		asprintf(&error_msg, "Preprocess error: %s", preprocess_error_msg);
		SetErrorMsg(error_msg);
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

int GetLabelAddress(char* label_ref) {
	for (int i = 0; i < s->label_state.count; i++) {
		if (strcmp(label_ref, s->label_state.label_names[i]) == 0) {
			if (++s->label_state.label_jump_counts[i] >= MAX_LABEL_JUMPS) {
				char* error_msg;
				asprintf(&error_msg, "%d jumps performed - Possible infinite loop\n\n%s", MAX_LABEL_JUMPS, PrintJumpLabelBreakdown());
				SetErrorMsg(error_msg);
			}
			free(label_ref);
			return s->label_state.label_locations[i];
		}
	}
	return -1;
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

char* PrintJumpLabelBreakdown() {
	char* result = NULL;
    char *temp = NULL;    
	LabelState* ls = &s->label_state;
	asprintf(&result, "Jumps to each label:");

    for (int i = 0; i < ls->count; i++) {
        asprintf(&temp, "\n%s: %d", ls->label_names[i], ls->label_jump_counts[i]);

		char *new_result;
		asprintf(&new_result, "%s%s", result, temp);
		free(result);  
		result = new_result;
		free(temp);    
    }
	return result;
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
