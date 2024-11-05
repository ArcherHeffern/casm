#include "ui_internal.h"
#include "assert.h"


StyleOverride* StyleOverrideCreate(State* state, OverrideType type, int id, float style) {
	StyleOverride** style_overrides = state->render_info.style_overrides;
	
	StyleOverride* style_override = (StyleOverride*)malloc(sizeof(StyleOverride));
	style_override->type = type;
	style_override->id = id;
	style_override->style = style;

	for (int i = 0; i < MAX_STYLE_OVERRIDES; i++) {
		if (style_overrides[i] == NULL) {
			style_overrides[i] = style_override;
			return style_override;
		}
	}
	assert(false && "Style Override queue is full");
}

double* StyleOverrideGet(State* state, OverrideType type, int id) {
	for (int i = 0; i < MAX_STYLE_OVERRIDES; i++) {
		StyleOverride* style_override = state->render_info.style_overrides[i];
		if (style_override == NULL) {
			continue;
		}
		if (style_override->type == type && style_override->id == id) {
			return &style_override->style;
		}
	}
	return NULL;
}

void StyleOverrideDestroy(State* state, StyleOverride* style_override) {
	StyleOverride** style_overrides = state->render_info.style_overrides;

	for (int i = 0; i < MAX_STYLE_OVERRIDES; i++) {
		if (style_override == style_overrides[i]) {
			style_overrides[i] = NULL;
			free(style_override);
			return;
		}
	}
	assert(false && "Could not find style override");
}
