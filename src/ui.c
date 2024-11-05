#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <raylib.h>

#include "util.h"
#include "casm.h"
#include "preprocess.h"
#include "ui_internal.h"

static State* s = NULL;
char* ErrorMsg = NULL;
Color BACKGROUND_COLOR = { .r = 0x18, .g = 0x18, .b = 0x18, .a = 0xFF };
Color FONT_COLOR = { .r = 0xFF, .g = 0xFF, .b = 0xFF, .a = 0xFF };
Color CELL_COLOR = { .r = 0x2A, .g = 0x2C, .b = 0x2E, .a = 0xFF };
bool cont = false;
char* EMPTY_CELL = "000000";


void Run(char* filename) {
	InitWindow(800, 600, "Mini Asm");	
	SetTargetFPS(60);
	RenderInfo render_info = {
		.register_height = GetScreenHeight(),
		.memory_height = GetScreenHeight(),
		.storage_height = GetScreenHeight(),
		.memory_pointer = {
			.x = 0,
			.y = 0,
			.width = 270,
			.height = 85
		},
		.storage_pointer = {
			.x = 0,
			.y = 0,
			.width = 270,
			.height = 85
		},
	};

	s = malloc(sizeof(State));
	assert(s != NULL);
	memset(s, 0, sizeof(State));
	for (int i = 0; i < 10; i++) {
		s->registers[i] = (int*)malloc(sizeof(int));
		*s->registers[i] = 0;
	}
	s->render_info = render_info;

	float mid_y = GetScreenHeight() / 2 - CELL_HEIGHT / 2 - (s->render_info.memory_pointer.height - CELL_HEIGHT) / 2;
	s->render_info.memory_pointer.y = mid_y;
	s->render_info.memory_pointer.x = X_PADDING - (s->render_info.memory_pointer.width - CELL_WIDTH) / 2;

	s->render_info.storage_pointer.y = mid_y;
	s->render_info.storage_pointer.x = GetScreenWidth() - X_PADDING - (s->render_info.storage_pointer.width - CELL_WIDTH) / 2 - CELL_WIDTH;
	int num_lines;

	FileReadLines(filename, &num_lines);

	StartVisualisation();
	while (!WindowShouldClose()) {
		Loop();
	}
}

// ============
// Runners
// ============

void StartVisualisation() {
	for (int i = 0; i < MEMORY_SIZE; i++) {
		s->memory[i] = EMPTY_CELL;
	}
	for (int i = 0; i < STORAGE_SIZE; i++) {
		s->storage[i] = EMPTY_CELL;
	}
	RenderInfo* render_info = &s->render_info;
	Render(s);
	SetActiveMemoryCell(s, 0, IN_N_OUT, SET_ACTIVE_CELL_DURATION, 0);
	Loop();
	int gap = REGISTER_CELL_HEIGHT * REGISTER_CELL_GAP;
	CreateAnimation(
		s,
		GetScreenHeight() - 10*(REGISTER_CELL_HEIGHT + gap) - 7,
		&render_info->register_height,
		IN_N_OUT,
		SLIDE_IN_TIME,
		0,
		NULL
	);
	Loop();
	SetActiveStorageCell(s, 0, IN_N_OUT, SET_ACTIVE_CELL_DURATION, 0);
	Loop();
	return;
}

void Loop() {
	while (!WindowShouldClose()) {
		if (!Step()) {
			return;
		}
	}
}

bool Step() {
	bool animations_left = StepAnimations(s);
	bool futures_left = StepFutures(s);

	HandleFileUpload();
	if (IsKeyPressed(KEY_R)) {
		Reset();
	}
	if (IsKeyPressed(KEY_C)) {
		cont = !cont;
	}
	if (IsKeyPressed(KEY_S) || cont) {
		if (animations_left || futures_left) {
		} else {
			StepProgram();
		}
	}

	Render(s);
	return animations_left || futures_left;
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
		SetMemoryCellValue(s, i, program[i], RESET_DURATION, i*RESET_DELAY);
	}
	return !HasError();
}

void HandleFileUpload() {
	if (!IsFileDropped()) {
		return;
	}
	FilePathList files = LoadDroppedFiles();
	assert(files.count > 0 && "Expected a file to be uploaded");
	char* file_path = files.paths[0];
	
	Reset();

	int num_lines;

	char** program = FileReadLines(file_path, &num_lines);
	LoadProgram(program, num_lines);
	UnloadDroppedFiles(files);
}


float Reset() {
	cont = false;
	if (s->error_msg) {
		free(s->error_msg);
		s->error_msg = NULL;
	}

	LabelState* ls = &s->label_state;
	
	for (int i = 0; i < ls->count; i++) {
		free(ls->label_names[i]);
		ls->label_names[i] = NULL;
		ls->label_locations[i] = 0; 
		ls->label_jump_counts[i] = 0;
	}
	ls->count = 0;
	s->haltflag = false;

	SetActiveMemoryCell(s, 0, IN_N_OUT, SET_ACTIVE_CELL_DURATION, 0);
	SetActiveStorageCell(s, 0, IN_N_OUT, SET_ACTIVE_CELL_DURATION, 0);
	for (int i = 0; i < 4; i++) {
		SetMemoryCellValue(s, i, EMPTY_CELL, RESET_DURATION, i*RESET_DELAY);
		SetStorageCellValue(s, i, EMPTY_CELL, RESET_DURATION, i*RESET_DELAY);
	}
	for (int i = 4; i < MEMORY_SIZE; i++) {
		s->memory[i] = EMPTY_CELL;
	}
	for (int i = 4; i < STORAGE_SIZE; i++) {
		s->storage[i] = EMPTY_CELL;
	}
	for (int i = 0; i < 10; i++) {
		SetRegisterCellValue(s, i, 0, RESET_DURATION, i*RESET_DELAY);
	}
	return 10 * RESET_DELAY;
}

// ============
// External Getters and Setters
// ============
int GetProgramCounter() { return UIGetRegister(0);}

int UIGetRegister(int reg_num) {return *s->registers[reg_num];}


char* UIGetMemory(int address) {return s->memory[address/4];}


char* UIGetStorage(int address) {return s->storage[address/4];}


void UISetRegister(int reg_num, int value) {
	if (reg_num == 0) {
		SetActiveMemoryCell(s, value, IN_N_OUT, SETTER_ANIMATION_DURATION, SETTER_ANIMATION_DELAY);
	}
	SetRegisterCellValue(s, reg_num, value, SETTER_ANIMATION_DURATION, SETTER_ANIMATION_DELAY);
}


void UISetMemory(int address, char* value) {
	SetMemoryCellValue(s, address/4, value, SETTER_ANIMATION_DURATION, SETTER_ANIMATION_DELAY);
}


void UISetStorage(int address, char* value) {
	SetActiveStorageCell(s, address/4, IN_N_OUT, SETTER_ANIMATION_DURATION, SETTER_ANIMATION_DELAY);
	SetStorageCellValue(s, address/4, value, SETTER_ANIMATION_DURATION, SETTER_ANIMATION_DELAY);
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
	if (s->error_msg) {
		free(msg);
		return;
	}
	s->error_msg = msg;
}

bool HasError() {
	return s->error_msg != NULL;
}

// ============
// Debug
// ============
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
	printf("Error at address %d executing '%s'\n", pc*4, UIGetMemory(pc*4));
	printf("%s\n", GetErrorMsg());
}
