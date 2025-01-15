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

/*
 * Good fonts:
 * NotoSans-Regular.ttf : 18
 * NotoSans-SemiBold.ttf : 18
 * NotoSans-Medium.ttf : 18
*/
#define FONT "./font/static/NotoSans-Medium.ttf"
State *s = NULL;
char *ErrorMsg = NULL;

Color BACKGROUND_COLOR = {.r = 0x18,.g = 0x18,.b = 0x18,.a = 0xFF };
Color FONT_COLOR = {.r = 0xFF,.g = 0xFF,.b = 0xFF,.a = 0xFF };
Color CELL_COLOR = {.r = 0x2A,.g = 0x2C,.b = 0x2E,.a = 0xFF };

bool cont = false;
bool end = false;
Font font = {0};
Font no_font = {0};

int FD = -1;
char *FILE_PATH = NULL;
time_t last_modification_time_s = 0;

Font GetFont() {
	if (memcmp(&font, &no_font, sizeof(Font)) == 0) {
		return GetFontDefault();
	}
	return font;
}
void ADrawText(const char *text, int posX, int posY, int fontSize, Color color) {
	// Logic taken from raylib/src/rtext.c:DrawText so perform exactly the same as default implementation
	Vector2 position = { (float)posX, (float)posY };
	int defaultFontSize = 10;   // Default Font chars height in pixel
	if (fontSize < defaultFontSize) fontSize = defaultFontSize;
	int spacing = fontSize/defaultFontSize;

	DrawTextEx(font, text, position, fontSize, spacing, color);
}

// Same as DrawTextPro, but autofills the font
void ADrawTextPro(const char *text, Vector2 position, Vector2 origin, float rotation, float fontSize, float spacing, Color tint) {
	DrawTextPro(font, text, position, origin, rotation, fontSize, spacing, tint);
}

void Run(char *filename)
{
	InitWindow(800, 600, "Mini Asm");
	SetTargetFPS(60);
 	font = LoadFont(FONT);
	Rectangle memory_pointer = {	// Program Counter Box
		.x = 0,
		.y = 0,
		.width = 270,
		.height = 85
	};
	memory_pointer.x =
	    X_PADDING - (memory_pointer.width - CELL_WIDTH) / 2;
	memory_pointer.y =
	    GetScreenHeight() / 2 - CELL_HEIGHT / 2 -
	    (memory_pointer.height - CELL_HEIGHT) / 2;

	Rectangle storage_pointer = {	// Active Storage Box
		.x = 0,
		.y = 0,
		.width = 270,
		.height = 85
	};
	storage_pointer.x =
	    GetScreenWidth() - X_PADDING - (storage_pointer.width -
					    CELL_WIDTH) / 2 - CELL_WIDTH;
	storage_pointer.y = memory_pointer.y;

	Rectangle popup_box = {
		0,
		0,
		300,
		150
	};
	popup_box.x = GetScreenWidth() / 2 - popup_box.width / 2;
	popup_box.y = GetScreenHeight() / 2 - popup_box.height / 2;

	int x_box_width = popup_box.width * 0.1;
	Rectangle x_box = {
		.x = popup_box.x + popup_box.width - x_box_width -
		    X_BOX_GAP,
		.y = popup_box.y + X_BOX_GAP,
		.width = x_box_width,
		.height = x_box_width
	};

	RenderInfo render_info = {
		.register_height = GetScreenHeight(),
		.memory_height = GetScreenHeight(),
		.storage_height = GetScreenHeight(),
		.memory_pointer = memory_pointer,
		.storage_pointer = storage_pointer,
		.popup_box = popup_box,
		.x_box = x_box
	};


	s = malloc(sizeof(State));
	assert(s != NULL);
	memset(s, 0, sizeof(State));
	for (int i = 0; i < 10; i++) {
		s->registers[i] = (int *) malloc(sizeof(int));
		*s->registers[i] = GARBAGE;
	}
	s->render_info = render_info;

	HandleFileUpload(filename);

	StartVisualisation();
	while (!WindowShouldClose()) {
		Loop();
	}
}

// ============
// Runners
// ============

void StartVisualisation()
{
	for (int i = 0; i < MEMORY_SIZE; i++) {
		s->memory[i] = EMPTY_CELL;
	}
	for (int i = 0; i < STORAGE_SIZE; i++) {
		s->storage[i] = EMPTY_CELL;
	}
	RenderInfo *render_info = &s->render_info;
	Render(s);
	SetActiveMemoryCell(s, 0, IN_N_OUT, SET_ACTIVE_CELL_DURATION, 0);
	Loop();
	int gap = REGISTER_CELL_HEIGHT * REGISTER_CELL_GAP;
	CreateAnimation(s,
			GetScreenHeight() - 10 * (REGISTER_CELL_HEIGHT +
						  gap) - 7,
			&render_info->register_height, IN_N_OUT,
			SLIDE_IN_TIME, 0, NULL);
	Loop();
	SetActiveStorageCell(s, 0, IN_N_OUT, SET_ACTIVE_CELL_DURATION, 0);
	Loop();
	return;
}

void Loop()
{
	while (!WindowShouldClose()) {
		if (!Step()) {
			return;
		}
	}
}

bool FileHasChanged()
{
	#ifdef __EMSCRIPTEN__
		return false;
	#else
		if (FD == -1) {
			return false;
		}
		struct stat buf;
		if (fstat(FD, &buf) == -1) {
			perror("Fstat");
		}
		bool modified = false;

		time_t modification_time_s = buf.st_mtimespec.tv_sec;

		if (modification_time_s != last_modification_time_s) {
			modified = true;
			last_modification_time_s = modification_time_s;
		}
		return modified;
	#endif
}

bool Step()
{
	bool animations_left = StepAnimations(s);
	bool futures_left = StepFutures(s);
	bool finished_or_errored = false;

	if (!HandleFileDropped()) {
		finished_or_errored = true;
	}

	if (FileHasChanged() || IsKeyPressed(KEY_R)) {
		if (FD != -1) {
			char *tmp_file_path = NULL;
			asprintf(&tmp_file_path, "%s", FILE_PATH);
			int tmp_fd = FD;
			Reset();
			FD = tmp_fd;
			asprintf(&FILE_PATH, "%s", tmp_file_path);
			free(tmp_file_path);
			int num_lines = 0;
			char **program =
			    FileReadLines(FILE_PATH, &num_lines,
					  MEMORY_SIZE, SetErrorMsg);
			if (program == NULL) {
				finished_or_errored = true;
			} else {
				LoadProgram(program, num_lines);
			}
		}
	}

	double y = GetMouseWheelMoveV().y * SCROLL_SPEED;
	// printf("Scroll Speed: %d\n", SCROLL_SPEED);
	// double full_memory_height = (MEMORY_SIZE-1) * (CELL_HEIGHT + CELL_GAP);
	// double full_storage_height = 0;
	// double full_height = MaxDouble(full_memory_height, full_storage_height);
	s->render_info.scroll_offset = s->render_info.scroll_offset + y;

	if (IsKeyPressed(KEY_C)) {
		end = false;
		cont = !cont;
	}
	if (IsKeyPressed(KEY_E)) {
		end = true;
	}
	if (IsKeyPressed(KEY_S) || cont || end) {
		if (!animations_left && !futures_left) {
			if (!StepProgram()) {
				finished_or_errored = true;
			}
		}
	}

	if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
		Vector2 mouse_position = GetMousePosition();
		if (s->render_info.popup_opacity == 1
		    && CheckCollisionPointRec(mouse_position,
					      s->render_info.x_box)) {
			CreateAnimation(s, 0,
					&s->render_info.popup_opacity,
					IN_N_OUT, POPUP_FADE_DURATION, 0,
					NULL);
		}
	}

	if (finished_or_errored) {
		CreateAnimation(s, 1, &s->render_info.popup_opacity,
				IN_N_OUT, POPUP_FADE_DURATION, 0, NULL);
	}

	if (HasError() || GetHaltflag()) {
		if (end) {
			SetActiveMemoryCell(s, MaxInt(*s->registers[0], 0), IN_N_OUT,
					    SET_ACTIVE_CELL_DURATION, 0);
			SetActiveStorageCell(s,
					     s->render_info.
					     last_modified_storage_cell,
					     IN_N_OUT,
					     SET_ACTIVE_CELL_DURATION, 0);
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
bool LoadProgram(char **program, int num_lines)
{
	Preprocess(&s->label_state, program, num_lines);
	if (HasError()) {
		return false;
	}
	for (int i = 0; i < num_lines; i++) {
		SetMemoryCellValue(s, i, program[i], RESET_DURATION,
				   i * RESET_DELAY);
	}
	return !HasError();
}

bool HandleFileDropped()
{
	if (!IsFileDropped()) {
		return true;	// True because this isn't an error
	}
	FilePathList files = LoadDroppedFiles();
	assert(files.count > 0 && "Expected a file to be uploaded");

	bool errored = HandleFileUpload(files.paths[0]);

	UnloadDroppedFiles(files);
	return errored;
}

bool HandleFileUpload(char *path)
{
	if (path == NULL || !FileExists(path)) {
		return false;
	}
	Reset();
	asprintf(&FILE_PATH, "%s", path);

	int num_lines;
	char **program =
	    FileReadLines(FILE_PATH, &num_lines, MEMORY_SIZE, SetErrorMsg);
	if (program == NULL) {
		return false;
	}

	LoadProgram(program, num_lines);

	if ((FD = open(FILE_PATH, O_RDONLY)) == -1) {
		char *error_msg;
		asprintf(&error_msg, "Failed to open file %s", FILE_PATH);
		SetErrorMsg(error_msg);
		return false;
	}
	struct stat buf;
	fstat(FD, &buf);

	#ifdef __EMSCRIPTEN__
		last_modification_time_s = buf.st_mtime;
	#else
		last_modification_time_s = buf.st_mtimespec.tv_sec;
	#endif // __EMSCRIPTEN__

	return true;
}


float Reset()
{
	cont = false;
	end = false;
	s->render_info.popup_opacity = 0;
	s->render_info.last_modified_storage_cell = 0;
	FD = -1;
	if (FILE_PATH != NULL) {
		free(FILE_PATH);
		FILE_PATH = NULL;
	}
	if (HasError()) {
		free(s->error_msg);
		s->error_msg = NULL;
	}

	LabelState *ls = &s->label_state;

	for (int i = 0; i < ls->count; i++) {
		free(ls->label_names[i]);
		ls->label_names[i] = NULL;
		ls->label_locations[i] = 0;
		ls->label_jump_counts[i] = 0;
	}
	ls->count = 0;
	s->haltflag = false;

	CreateAnimation(s, 0, &s->render_info.scroll_offset, IN_N_OUT, SET_ACTIVE_CELL_DURATION, 0, NULL);	// Set Scroll to baseline
	SetActiveMemoryCell(s, 0, IN_N_OUT, SET_ACTIVE_CELL_DURATION, 0);
	SetActiveStorageCell(s, 0, IN_N_OUT, SET_ACTIVE_CELL_DURATION, 0);
	int mx = MinInt(MinInt(4, MEMORY_SIZE), STORAGE_SIZE);
	for (int i = 0; i < mx; i++) {
		SetMemoryCellValue(s, i, EMPTY_CELL, RESET_DURATION,
				   i * RESET_DELAY);
		SetStorageCellValue(s, i, EMPTY_CELL, RESET_DURATION,
				    i * RESET_DELAY);
	}
	for (int i = 4; i < MEMORY_SIZE; i++) {
		s->memory[i] = EMPTY_CELL;
	}
	for (int i = 4; i < STORAGE_SIZE; i++) {
		s->storage[i] = EMPTY_CELL;
	}
	SetRegisterCellValue(s, 0, 0, RESET_DURATION, 0);
	for (int i = 1; i < 10; i++) {
		SetRegisterCellValue(s, i, GARBAGE, RESET_DURATION,
				     i * RESET_DELAY);
	}
	return 10 * RESET_DELAY;
}

// ============
// External Getters and Setters
// ============
int GetProgramCounter()
{
	return UIGetRegister(0);
}

int UIGetRegister(int reg_num)
{
	return *s->registers[reg_num];
}


char *UIGetMemory(int address)
{
	return s->memory[address / 4];
}


char *UIGetStorage(int address)
{
	return s->storage[address / 4];
}


void UISetRegister(int reg_num, int value)
{
	if (end) {
		int *v = malloc(sizeof(int));	// TODO: Fix memory leak
		if (v == NULL) {
			printf("Out of memory!\n");
			exit(1);
		}
		*v = value;
		s->registers[reg_num] = v;
		return;
	}
	if (reg_num == 0) {
		SetActiveMemoryCell(s, value, IN_N_OUT,
				    SETTER_ANIMATION_DURATION,
				    SETTER_ANIMATION_DELAY);
	}
	SetRegisterCellValue(s, reg_num, value, SETTER_ANIMATION_DURATION,
			     SETTER_ANIMATION_DELAY);
}


void UISetMemory(int address, char *value)
{
	if (end) {
		s->memory[address / 4] = value;
		return;
	}
	SetMemoryCellValue(s, address / 4, value,
			   SETTER_ANIMATION_DURATION,
			   SETTER_ANIMATION_DELAY);
}


void UISetStorage(int address, char *value)
{
	if (end) {
		s->render_info.last_modified_storage_cell = address / 4;
		s->storage[address / 4] = value;
		return;
	}
	SetActiveStorageCell(s, address / 4, IN_N_OUT,
			     SETTER_ANIMATION_DURATION,
			     SETTER_ANIMATION_DELAY);
	SetStorageCellValue(s, address / 4, value,
			    SETTER_ANIMATION_DURATION,
			    SETTER_ANIMATION_DELAY);
}


bool GetHaltflag()
{
	return s->haltflag;
}


char *GetErrorMsg()
{
	return s->error_msg;
}


void SetHaltflag(bool flag)
{
	s->haltflag = flag;
}


void SetErrorMsg(char *msg)
{
	if (s->error_msg) {
		free(msg);
		return;
	}
	s->error_msg = msg;
}

bool HasError()
{
	return s->error_msg != NULL;
}

// ============
// Debug
// ============
void PrintRegisters()
{
	printf("PC: %d\n", GetProgramCounter());
	for (int i = 1; i < 10; i++) {
		printf("R%d: %d\n", i, UIGetRegister(i));
	}
}

void PrintMemory()
{
	PrintMemoryRange(0, MEMORY_SIZE - 1);
}

void PrintMemoryRange(int lower, int upper)
{
	for (int i = lower / 4; i < upper / 4 + 1; i++) {
		printf("%d: %s\n", i * 4, UIGetMemory(i * 4));
	}
}


void PrintErrorMsg()
{
	if (!HasError()) {
		printf
		    ("Attempted to print error msg when there was no error\n");
		return;
	}
	int pc = GetProgramCounter();
	printf("Error at address %d executing '%s'\n", MaxInt(pc * 4, 0),
	       UIGetMemory(pc * 4));
	printf("%s\n", GetErrorMsg());
}
