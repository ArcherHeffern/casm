#include <time.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <raylib.h>

#include "util.h"
#include "casm.h"
#include "ui_internal.h"

State* s = NULL;
char* ErrorMsg = NULL;

Color BACKGROUND_COLOR = { .r = 0x18, .g = 0x18, .b = 0x18, .a = 0xFF };
Color FONT_COLOR = { .r = 0xFF, .g = 0xFF, .b = 0xFF, .a = 0xFF };
Color CELL_COLOR = { .r = 0x2A, .g = 0x2C, .b = 0x2E, .a = 0xFF };
bool cont = false;
bool end = false;
char* EMPTY_CELL = "000000";

int FD = -1;
char* FILE_PATH = NULL;
struct timespec LAST_MODIFICATION = { 0 };


void Run(char* filename) {
	InitWindow(800, 600, "Mini Asm");	
	SetTargetFPS(60);
	RenderInfo render_info = {
		.register_height = GetScreenHeight(),
		.memory_height = GetScreenHeight(),
		.storage_height = GetScreenHeight(),
		.memory_pointer = { // Program Counter Box
			.x = 0,
			.y = 0,
			.width = 270,
			.height = 85
		},
		.storage_pointer = { // Active Storage Box
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

bool FileHasChanged() {
	if (FD == -1) {
		return false;
	}
	struct stat buf;
	if (fstat(FD, &buf) == -1) {
		perror("Fstat");
	}
	bool modified = false;
	if (buf.st_mtimespec.tv_sec != LAST_MODIFICATION.tv_sec 
		|| buf.st_mtimespec.tv_nsec != LAST_MODIFICATION.tv_nsec) {
		modified = true;
	}
	LAST_MODIFICATION = buf.st_mtimespec;
	return modified;
}

bool Step() {
	bool animations_left = StepAnimations(s);
	bool futures_left = StepFutures(s);

	HandleFileUpload();

	if (FileHasChanged() || IsKeyPressed(KEY_R)) {
		char* tmp_file_path = NULL;
		asprintf(&tmp_file_path, "%s", FILE_PATH);
		int tmp_fd = FD;
		Reset();
		FD = tmp_fd;
		asprintf(&FILE_PATH, "%s", tmp_file_path);
		free(tmp_file_path);
		int num_lines = 0;
		char** program = FileReadLines(FILE_PATH, &num_lines);
		LoadProgram(program, num_lines);
	}
	if (IsKeyPressed(KEY_C)) {
		end = false;
		cont = !cont;
	}
	if (IsKeyPressed(KEY_E)) {
		end = true;
	}
	if (IsKeyPressed(KEY_S) || cont || end) {
		if (animations_left || futures_left) {
		} else {
			StepProgram();
		}
	}

	if (HasError() || GetHaltflag()) {
		if (end) {
			SetActiveMemoryCell(s, *s->registers[0], IN_N_OUT, SET_ACTIVE_CELL_DURATION, 0);
			SetActiveStorageCell(s, s->render_info.last_modified_storage_cell, IN_N_OUT, SET_ACTIVE_CELL_DURATION, 0);
			end = false;
		}
		cont = false;
	}

	Render(s);
	return animations_left || futures_left;
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

	Reset();

	asprintf(&FILE_PATH, "%s", files.paths[0]);

	int num_lines;
	char** program = FileReadLines(FILE_PATH, &num_lines);
	LoadProgram(program, num_lines);

	if ((FD = open(FILE_PATH, O_RDONLY)) == -1) {
		char* error_msg;
		asprintf(&error_msg, "Failed to open file %s", FILE_PATH);
		SetErrorMsg(error_msg);
		return;
	}
	struct stat buf;
	fstat(FD, &buf);
	LAST_MODIFICATION = buf.st_mtimespec;
	
	UnloadDroppedFiles(files);
}


float Reset() {
	cont = false;
	end = false;
	s->render_info.last_modified_storage_cell = 0;
	FD = -1;
	if (FILE_PATH != NULL) {
		free(FILE_PATH);
		FILE_PATH = NULL;
	}
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
	if (end) {
		int* v = malloc(sizeof(int)); // TODO: Fix memory leak
		if (v == NULL) {
			printf("Out of memory!\n");
			exit(1);
		}
		*v = value;
		s->registers[reg_num] = v;
		return;
	}
	if (reg_num == 0) {
		SetActiveMemoryCell(s, value, IN_N_OUT, SETTER_ANIMATION_DURATION, SETTER_ANIMATION_DELAY);
	}
	SetRegisterCellValue(s, reg_num, value, SETTER_ANIMATION_DURATION, SETTER_ANIMATION_DELAY);
}


void UISetMemory(int address, char* value) {
	if (end) {
		s->memory[address/4] = value;
		return;
	}
	SetMemoryCellValue(s, address/4, value, SETTER_ANIMATION_DURATION, SETTER_ANIMATION_DELAY);
}


void UISetStorage(int address, char* value) {
	if (end) {
		s->render_info.last_modified_storage_cell = address/4;
		s->storage[address/4] = value;
		return;
	}
	SetActiveStorageCell(s, address/4, IN_N_OUT, SETTER_ANIMATION_DURATION, SETTER_ANIMATION_DELAY);
	SetStorageCellValue(s, address/4, value, SETTER_ANIMATION_DURATION, SETTER_ANIMATION_DELAY);
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


void PrintErrorMsg() {
	if (!HasError()) {
		printf("Attempted to print error msg when there was no error\n");
		return;
	}
	int pc = GetProgramCounter();
	printf("Error at address %d executing '%s'\n", pc*4, UIGetMemory(pc*4));
	printf("%s\n", GetErrorMsg());
}
