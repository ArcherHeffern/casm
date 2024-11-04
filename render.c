#include <stdio.h>
#include "plug_internal.h"

// ============
// Rendering
// ============
void Render(State* s) {
	BeginDrawing();
		ClearBackground(BACKGROUND_COLOR);
		RenderMemory(s);
		RenderRegisters(s);
		RenderStorage(s);
		RenderControls(s);
	EndDrawing();
}

void RenderMemory(State* s) {
	RenderInfo* render_info = &s->render_info;
	char** memory = s->memory;

	DrawText("Memory", X_PADDING, CELL_GAP, HEADER_SIZE, FONT_COLOR); // Title
	DrawRectangleLinesEx(render_info->memory_pointer, 2.0F, PC_COLOR); // Program Counter
	for (int i = 0; i < MEMORY_SIZE; i++) {
		RenderMemoryCell(s, i);
	}
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

void RenderRegisters(State* state) {
	RenderInfo* render_info = &state->render_info;
	float textWidth = MeasureTextEx(GetFontDefault(), "Registers", HEADER_SIZE, 1).x;
	Vector2 position = { .x=GetScreenWidth()/2 - textWidth/2, .y=CELL_GAP };
	DrawTextEx(GetFontDefault(), "registers", position, HEADER_SIZE, 1, FONT_COLOR);
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

void RenderStorage(State* state) {
	RenderInfo* render_info = &state->render_info;
	char** storage = state->storage;

	float textWidth = MeasureTextEx(GetFontDefault(), "Storage", HEADER_SIZE, 1).x;
	DrawText("Storage", GetScreenWidth() - X_PADDING - textWidth, CELL_GAP, HEADER_SIZE, FONT_COLOR);
	DrawRectangleLinesEx(render_info->storage_pointer, 2.0F, PC_COLOR); 

	for (int i = 0; i < STORAGE_SIZE; i++) {
		RenderStorageCell(state, i);
	}
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
	RenderInfo* render_info = &state->render_info;
	char* instruction = "Drag assembly file to upload";
	Vector2 textSize = MeasureTextEx(GetFontDefault(), instruction, TEXT_SIZE, 1);
	DrawText(instruction, GetScreenWidth()/2 - textSize.x/2, HEADER_GAP, TEXT_SIZE, FONT_COLOR);
	
	float width = REGISTER_CELL_WIDTH/2 * (1-REGISTER_CELL_GAP);
	float height = REGISTER_CELL_HEIGHT + REGISTER_CELL_GAP;
	float gap = REGISTER_CELL_WIDTH * REGISTER_CELL_GAP * 0.5;
	Rectangle continue_button = {
		.x=GetScreenWidth()/2 + gap,
		.y=(HEADER_GAP + textSize.y) * (1+REGISTER_CELL_GAP),
		.width=width,
		.height=height
	};
	DrawRectangleRec(continue_button, render_info->button_color);

	char* continue_text = "(C)ontinue";
	textSize = MeasureTextEx(GetFontDefault(), continue_text, TEXT_SIZE, 1);
	DrawText(
		continue_text, 
		continue_button.x+continue_button.width/2-textSize.x/2, 
		continue_button.y+continue_button.height/2-textSize.y/2, 
		TEXT_SIZE, 
		FONT_COLOR
	);
	
	Rectangle step_button = {
		.x=GetScreenWidth()/2 - REGISTER_CELL_WIDTH/2,
		.y=(HEADER_GAP + textSize.y) * (1+REGISTER_CELL_GAP),
		.width=width,
		.height=height,
	};
	DrawRectangleRec(step_button, render_info->button_color);

	char* step_text = "(S)tep";
	textSize = MeasureTextEx(GetFontDefault(), step_text, TEXT_SIZE, 1);
	DrawText(
		step_text, 
		step_button.x+step_button.width/2-textSize.x/2, 
		step_button.y+step_button.height/2-textSize.y/2, 
		TEXT_SIZE, 
		FONT_COLOR
	);
}
