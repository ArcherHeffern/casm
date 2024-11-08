#include <stdio.h>
#include <assert.h>
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
		RenderPopup();
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
	char* msg;
	asprintf(&msg, "R%d: %d", i, *state->registers[i]);
	if (i == 0) {
		free(msg);
		asprintf(&msg, "PC: %d", *state->registers[0]);
	}
	DrawText(msg, x+20, y+REGISTER_CELL_HEIGHT/2, TEXT_SIZE, faded_color);
	free(msg);
}

void RenderMemory(State* s) {
	RenderInfo* render_info = &s->render_info;

	for (int i = 0; i < MEMORY_SIZE; i++) {
		RenderMemoryCell(s, i);
	}

	render_info->memory_pointer.y += s->render_info.scroll_offset;
	DrawRectangleLinesEx(render_info->memory_pointer, 2.0F, PC_COLOR); // Program Counter
	render_info->memory_pointer.y -= s->render_info.scroll_offset;
}

void RenderMemoryCell(State* state, int i) {
	RenderInfo* render_info = &state->render_info;
	char** memory = state->memory;
	double* maybe_multiplier = StyleOverrideGet(state, MEMORY_CELL_SIZE_MULTIPLIER, i);
	double multiplier = maybe_multiplier == NULL ? 1: *maybe_multiplier;
	int y = render_info->memory_height + CELL_HEIGHT*i + CELL_GAP*i;
	char* label_name = GetLabelName(i*4);
	double scroll_offset = s->render_info.scroll_offset;

	DrawRectangle(
		X_PADDING - 0.5 * (CELL_WIDTH * multiplier - CELL_WIDTH),
		y - 0.5 * (CELL_HEIGHT * multiplier - CELL_HEIGHT) + scroll_offset,
		CELL_WIDTH * multiplier,
		CELL_HEIGHT * multiplier,
		CELL_COLOR
	);
	char* msg = NULL;
	asprintf(&msg, "Ox%x: %s", i*4, memory[i]);
	float textWidth = MeasureTextEx(GetFontDefault(), msg, TEXT_SIZE, 1).x;
	DrawText(msg, X_PADDING + CELL_WIDTH/2 - textWidth/2, y+CELL_HEIGHT/2 + scroll_offset, TEXT_SIZE, FONT_COLOR);

	if (label_name) {
		char* pretty_label_name = NULL;
		asprintf(&pretty_label_name, "%s:", label_name);
		float textWidth = MeasureTextEx(GetFontDefault(), pretty_label_name, TEXT_SIZE, 1).x;
		DrawText(pretty_label_name, X_PADDING + 20, y+CELL_HEIGHT/2 + scroll_offset, TEXT_SIZE, FONT_COLOR);
		free(pretty_label_name);
	}
}

void RenderStorage(State* state) {
	RenderInfo* render_info = &state->render_info;

	for (int i = 0; i < STORAGE_SIZE; i++) {
		RenderStorageCell(state, i);
	}

	render_info->storage_pointer.y += s->render_info.scroll_offset;
	DrawRectangleLinesEx(render_info->storage_pointer, 2.0F, PC_COLOR); 
	render_info->storage_pointer.y -= s->render_info.scroll_offset;
}

void RenderStorageCell(State* state, int i) {
	RenderInfo* render_info = &state->render_info;
	char** storage = state->storage;
	int y = render_info->storage_height + CELL_HEIGHT*i + CELL_GAP*i;
	double* maybe_multiplier = StyleOverrideGet(state, STORAGE_CELL_SIZE_MULTIPLIER, i);
	double multiplier = maybe_multiplier == NULL ? 1: *maybe_multiplier;
	double scroll_offset = s->render_info.scroll_offset;

	DrawRectangle(
		GetScreenWidth() - X_PADDING - CELL_WIDTH - 0.5 * (CELL_WIDTH * multiplier - CELL_WIDTH),
		y - 0.5 * (CELL_HEIGHT * multiplier - CELL_HEIGHT) + scroll_offset,
		CELL_WIDTH * multiplier,
		CELL_HEIGHT * multiplier,
		CELL_COLOR
	);
	char* msg = NULL;
	asprintf(&msg, "Ox%x: %s", i*4, storage[i]);
	float textWidth = MeasureTextEx(GetFontDefault(), msg, TEXT_SIZE, 1).x;
	DrawText(msg, GetScreenWidth() - X_PADDING - CELL_WIDTH/2 - textWidth/2, y+CELL_HEIGHT/2+scroll_offset, TEXT_SIZE, FONT_COLOR);
	free(msg);
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

void RenderPopup() {
	if (s->render_info.popup_opacity == 0) {
		return;
	}

	double opacity = s->render_info.popup_opacity;
	char* msg;
	Color background_color;
	if (HasError()) {
		char* current_memory = UIGetMemory(GetProgramCounter()*4);
		asprintf(&msg, "Error at address %d executing '%s'\n%s\n", 
			GetProgramCounter()*4, 
			current_memory!=NULL?current_memory:"000000",
			GetErrorMsg()
		);
		background_color = RED;
	} else {
		background_color = GREEN;
		asprintf(&msg, "Program Complete!");
	}
	background_color = Fade(background_color, opacity);
	Color button_color = Fade(WHITE, opacity);
	Vector2 textSize = MeasureTextEx(GetFontDefault(), msg, TEXT_SIZE, 1);
	Color font_color = Fade(FONT_COLOR, opacity);
	Color shadow_color = Fade(WHITE, opacity);

	Vector2 origin = {.x=0, .y=(1-opacity)*-POPUP_FADE_Y_DISPLACEMENT_PX};
	Vector2 shadow_origin = {.x=-POPUP_SHADOW_GAP, .y=(1-opacity)*-POPUP_FADE_Y_DISPLACEMENT_PX+POPUP_SHADOW_GAP};
	Rectangle top_shadow = {
		.x=s->render_info.popup_box.x, 
		.y=s->render_info.popup_box.y, 
		.width=s->render_info.popup_box.width,
		.height=POPUP_SHADOW_GAP
	};
	Rectangle right_shadow = {
		.x=s->render_info.popup_box.x + s->render_info.popup_box.width - POPUP_SHADOW_GAP, 
		.y=s->render_info.popup_box.y, 
		.width=POPUP_SHADOW_GAP,
		.height=s->render_info.popup_box.height
	};
	// Top shadow
	DrawRectanglePro(top_shadow, shadow_origin, 0, shadow_color);
	// Right shadow
	DrawRectanglePro(right_shadow, shadow_origin, 0, shadow_color);
	// Bottom shadow
	DrawRectanglePro(s->render_info.popup_box, origin, 0, background_color);
	DrawRectanglePro(s->render_info.x_box, origin, 0, button_color);
	DrawTextPro(
		GetFontDefault(),
		msg, 
		(Vector2){ .x=GetScreenWidth()/2 - textSize.x/2, .y=GetScreenHeight()/2 - textSize.y/2},
		origin,
		0,
		TEXT_SIZE, 
		1,
		font_color
	);


	free(msg);
}
