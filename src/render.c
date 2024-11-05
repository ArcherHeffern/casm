#include <stdio.h>
#include "ui_internal.h"

// ============
// Rendering
// ============
void Render(State* s) {
	BeginDrawing();
		ClearBackground(BACKGROUND_COLOR);
		RenderMemory(s);
		RenderRegisters(s);
		RenderStorage(s);
		RenderHeader();
		RenderControls(s);
		RenderErrorMsg();
	EndDrawing();
}

void RenderRegisters(State* state) {
	for (int i = 0; i < 10; i++) {
		RenderRegister(state, i);
	}
}

void RenderRegister(State* state, int i) {
	RenderInfo* render_info = &state->render_info;
	int x = GetScreenWidth() / 2 - (REGISTER_CELL_WIDTH/2);
	int gap = REGISTER_CELL_HEIGHT*REGISTER_CELL_GAP;
	int y = render_info->register_height + i*(REGISTER_CELL_HEIGHT+gap);
	double* maybe_fade = StyleOverrideGet(state, REGISTER_FADE, i);
	double fade = maybe_fade == NULL ? 1: *maybe_fade;
	Color faded_color = Fade(FONT_COLOR, fade);

	DrawRectangle(
		x,
		y,
		REGISTER_CELL_WIDTH,
		REGISTER_CELL_HEIGHT,
		CELL_COLOR
	);
	char* msg = NULL;
	asprintf(&msg, "R%d: %d", i, *state->registers[i]);
	if (i == 0) {
		free(msg);
		asprintf(&msg, "PC: %d", *state->registers[0]);
	}
	DrawText(msg, x+20, y+REGISTER_CELL_HEIGHT/2, TEXT_SIZE, faded_color);
}

void RenderMemory(State* s) {
	RenderInfo* render_info = &s->render_info;

	for (int i = 0; i < MEMORY_SIZE; i++) {
		RenderMemoryCell(s, i);
	}

	DrawRectangleLinesEx(render_info->memory_pointer, 2.0F, PC_COLOR); // Program Counter
}

void RenderMemoryCell(State* state, int i) {
	RenderInfo* render_info = &state->render_info;
	char** memory = state->memory;
	double* maybe_multiplier = StyleOverrideGet(state, MEMORY_CELL_SIZE_MULTIPLIER, i);
	double multiplier = maybe_multiplier == NULL ? 1: *maybe_multiplier;
	int y = render_info->memory_height + CELL_HEIGHT*i + CELL_GAP*i;

	DrawRectangle(
		X_PADDING - 0.5 * (CELL_WIDTH * multiplier - CELL_WIDTH),
		y - 0.5 * (CELL_HEIGHT * multiplier - CELL_HEIGHT),
		CELL_WIDTH * multiplier,
		CELL_HEIGHT * multiplier,
		CELL_COLOR
	);
	char* msg = NULL;
	asprintf(&msg, "Ox%x: %s", i*4, memory[i]);
	float textWidth = MeasureTextEx(GetFontDefault(), msg, TEXT_SIZE, 1).x;
	DrawText(msg, X_PADDING + CELL_WIDTH/2 - textWidth/2, y+CELL_HEIGHT/2, TEXT_SIZE, FONT_COLOR);
}

void RenderStorage(State* state) {
	RenderInfo* render_info = &state->render_info;

	for (int i = 0; i < STORAGE_SIZE; i++) {
		RenderStorageCell(state, i);
	}

	DrawRectangleLinesEx(render_info->storage_pointer, 2.0F, PC_COLOR); 
}

void RenderStorageCell(State* state, int i) {
	RenderInfo* render_info = &state->render_info;
	char** storage = state->storage;

	int y = render_info->storage_height + CELL_HEIGHT*i + CELL_GAP*i;
	double* maybe_multiplier = StyleOverrideGet(state, STORAGE_CELL_SIZE_MULTIPLIER, i);
	double multiplier = maybe_multiplier == NULL ? 1: *maybe_multiplier;

	DrawRectangle(
		GetScreenWidth() - X_PADDING - CELL_WIDTH - 0.5 * (CELL_WIDTH * multiplier - CELL_WIDTH),
		y - 0.5 * (CELL_HEIGHT * multiplier - CELL_HEIGHT),
		CELL_WIDTH * multiplier,
		CELL_HEIGHT * multiplier,
		CELL_COLOR
	);
	char* msg = NULL;
	asprintf(&msg, "Ox%x: %s", i*4, storage[i]);
	float textWidth = MeasureTextEx(GetFontDefault(), msg, TEXT_SIZE, 1).x;
	DrawText(msg, GetScreenWidth() - X_PADDING - CELL_WIDTH/2 - textWidth/2, y+CELL_HEIGHT/2, TEXT_SIZE, FONT_COLOR);
}

void RenderControls(State* state) {
	float gap = REGISTER_CELL_WIDTH * REGISTER_CELL_GAP * 0.5;
	int right = GetScreenWidth()/2 + gap; 
	int left =GetScreenWidth()/2 - REGISTER_CELL_WIDTH/2;
	int bottom = GetScreenHeight() - (REGISTER_CELL_HEIGHT + REGISTER_CELL_GAP) * 12 - 20;
	int top = bottom - REGISTER_CELL_HEIGHT;


	char* instruction = "Drag assembly file to upload";
	Vector2 textSize = MeasureTextEx(GetFontDefault(), instruction, TEXT_SIZE, 1);
	DrawText(instruction, GetScreenWidth()/2 - textSize.x/2, top - REGISTER_CELL_HEIGHT*.75, TEXT_SIZE, FONT_COLOR);
	
	RenderButton(state, left, top, "(S)tep");
	RenderButton(state, right, top, "(C)ontinue");
	RenderButton(state, left, bottom, "(R)eset");
	RenderButton(state, right, bottom, "(E)nd");
}

void RenderButton(State* state, int x, int y, char* text) {
	RenderInfo* render_info = &state->render_info;
	float width = REGISTER_CELL_WIDTH/2 * (1-REGISTER_CELL_GAP);
	float height = REGISTER_CELL_HEIGHT*0.75;
	Rectangle button = {
		.x=x,
		.y=y,
		.width=width,
		.height=height
	};
	DrawRectangleRec(button, render_info->button_color);
	Vector2 textSize = MeasureTextEx(GetFontDefault(), text, TEXT_SIZE, 1);
	DrawText(
		text, 
		button.x+button.width/2-textSize.x/2, 
		button.y+button.height/2-textSize.y/2, 
		TEXT_SIZE, 
		FONT_COLOR
	);
}

void RenderHeader() {
	DrawRectangle(0, 0, GetScreenWidth(), HEADER_GAP, BACKGROUND_COLOR);

	float textWidth = MeasureTextEx(GetFontDefault(), "Registers", HEADER_SIZE, 1).x;
	Vector2 position = { .x=GetScreenWidth()/2 - textWidth/2, .y=CELL_GAP };
	DrawTextEx(GetFontDefault(), "registers", position, HEADER_SIZE, 1, FONT_COLOR);

	DrawText("Memory", X_PADDING, CELL_GAP, HEADER_SIZE, FONT_COLOR); // Title

	textWidth = MeasureTextEx(GetFontDefault(), "Storage", HEADER_SIZE, 1).x;
	DrawText("Storage", GetScreenWidth() - X_PADDING - textWidth, CELL_GAP, HEADER_SIZE, FONT_COLOR);
	
}

void RenderErrorMsg() {
	if (!HasError()) {
		return;
	}
	int width = 300;
	int height = 150;
	DrawRectangle(GetScreenWidth()/2-width/2, GetScreenHeight()/2-height/2, width, height, RED);
	static char* _error_msg = NULL;
	if (_error_msg != NULL) {
		free(_error_msg);
		_error_msg = NULL;
	}
	asprintf(&_error_msg, "Error at address %d executing '%s'\n%s\n", 
		GetProgramCounter()*4, 
		UIGetMemory(GetProgramCounter()*4),
		GetErrorMsg()
	);
	Vector2 textSize = MeasureTextEx(GetFontDefault(), _error_msg, TEXT_SIZE, 1);
	DrawText(
		_error_msg, 
		GetScreenWidth()/2 - textSize.x/2,
		GetScreenHeight()/2 - textSize.y/2,
		TEXT_SIZE, 
		FONT_COLOR
	);
}
