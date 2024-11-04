
#ifndef CASM_H_
#define CASM_H_

#include <stdbool.h>

/* Interpreter Constants */
#define MEMORY_SIZE 64
#define STORAGE_SIZE 64
#define MAX_REGISTERS 9
#define MAX_LABEL_JUMPS 1000
#define MAX_LABELS 16

#define CELL_SIZE 1024
/* Animation Constants */
#define MAX_FUTURES 256
#define MAX_ANIMATIONS 256
#define MAX_STYLE_OVERRIDES 256

// Rendering Constants
#define HEADER_SIZE 24
#define TEXT_SIZE 12
#define SLIDE_IN_TIME 0.2 // Seconds
#define MEMORY_CELL_SET_VALUE_TIME 1
#define X_PADDING 40
#define CELL_EXPAND_PERCENT 1.2
#define PC_COLOR BLUE
#define HEADER_GAP 100
#define REGISTER_CELL_WIDTH 160
#define REGISTER_CELL_HEIGHT 35
#define REGISTER_CELL_GAP 0.15
#define CELL_HEIGHT 65
#define CELL_WIDTH 250
#define CELL_GAP 20
#define SCROLL_SPEED 4

// Timing
#define RESET_DURATION 0.5
#define RESET_DELAY 0.1
#define SET_ACTIVE_CELL_DURATION 0.5
#define SETTER_ANIMATION_DURATION 0.5
#define SETTER_ANIMATION_DELAY 0.0

void PassUIGettersAndSetters(
	int (*_UIGetRegister)(int reg_num),
	char* (*_UIGetMemory)(int address),
	char* (*_UIGetStorage)(int address),
	void (*_UISetRegister)(int reg_num, int value),
	void (*_UISetMemory)(int address, char* value),
	void (*_UISetStorage)(int address, char* value)
);

bool LoadProgram(
	char** program,
	int num_lines
);

bool RunProgram();
bool StepProgram();
void PrintErrorMsg();

void SetMemoryCellValue(int cell, char* value, float duration, float delay);

#endif // CASM_H_
