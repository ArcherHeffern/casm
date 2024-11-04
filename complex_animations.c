#include "plug_internal.h"

// ============
// Complex Animations
// ============
void SetActiveMemoryCell(State* s, int cell, EasingFunction easing, float duration, float delay) {
	RenderInfo* render_info = &s->render_info;
	float end = -CELL_HEIGHT*cell - CELL_GAP*cell + GetScreenHeight() / 2 - CELL_HEIGHT / 2;
	CreateAnimation(s, end, &render_info->memory_height, easing, duration, delay, NULL);
}

void SetActiveStorageCell(State* s, int cell, EasingFunction easing, float duration, float delay) {
	RenderInfo* render_info = &s->render_info;
	float end = -CELL_HEIGHT*cell - CELL_GAP*cell + GetScreenHeight() / 2 - CELL_HEIGHT / 2;
	CreateAnimation(s, end, &render_info->storage_height, easing, duration, delay, NULL);
}

void SetMemoryCellValue(State* s, int cell, char* value, float duration, float delay) {
	// Set text after delay
	CreateFuture(s, delay, (void*)&s->memory[cell], value, NULL);

	// Grow and shrink cell by a percent
	StyleOverride* style_override = StyleOverrideCreate(s, MEMORY_CELL_SIZE_MULTIPLIER, cell, 1);
	CreateAnimation(s, CELL_EXPAND_PERCENT, &style_override->style, IN_N_BACK, duration, delay, style_override);
}

void SetStorageCellValue(State* s, int cell, char* value, float duration, float delay) {
	// Set text after delay
	CreateFuture(s, delay, (void*)&s->storage[cell], value, NULL);

	// Grow and shrink cell by a percent
	StyleOverride* style_override = StyleOverrideCreate(s, STORAGE_CELL_SIZE_MULTIPLIER, cell, 1);
	CreateAnimation(s, CELL_EXPAND_PERCENT, &style_override->style, IN_N_BACK, duration, delay, style_override);

}
void SetRegisterCellValue(State* s, int cell, int value, float duration, float delay) {
	int* i = (int*)malloc(sizeof(int)); // TODO: Fix leak - Idea: Callback on future completions
	*i=value;
	// Set text after delay
	CreateFuture(s, delay+duration/2, (void*)&s->registers[cell], i, NULL);

	// Fade text
	StyleOverride* style_override = StyleOverrideCreate(s, REGISTER_FADE, cell, 1);
	CreateAnimation(s, 0, &style_override->style, IN_N_BACK, duration, delay, style_override);
}