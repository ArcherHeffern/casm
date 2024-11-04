
#ifndef CASM_H_
#define CASM_H_

#include <stdbool.h>

bool LoadProgram(
	char** program,
	int num_lines,
	int (*GetRegister)(int reg_num),
	char* (*GetMemory)(int address),
	char* (*GetStorage)(int address),
	void (*SetRegister)(int reg_num, int value),
	void (*SetMemory)(int address, char* value),
	void (*SetStorage)(int address, char* value)
);
bool RunProgram();
bool StepProgram();
void PrintErrorMsg();

#endif // CASM_H_
