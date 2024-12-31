#ifndef UI_INTERNAL_H_
#define UI_INTERNAL_H_

#include <stdlib.h>

#include "raylib.h"
#include "ui.h"
#include "util.h"

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
#ifdef __EMSCRIPTEN__
	#define SCROLL_SPEED 8
#else
	#define SCROLL_SPEED 4
#endif // __EMSCRIPTEN__
#define X_BOX_GAP 10
#define POPUP_FADE_Y_DISPLACEMENT_PX 20
#define POPUP_SHADOW_GAP 5
#define LABEL_BOX_PADDING 10

// Timing
#define RESET_DURATION 0.5
#define RESET_DELAY 0.1
#define SET_ACTIVE_CELL_DURATION 0.5
#define SETTER_ANIMATION_DURATION 0.5
#define SETTER_ANIMATION_DELAY 0.0
#define POPUP_FADE_DURATION 0.3

#define EMPTY_CELL "garbage"
#define GARBAGE -4 // Must be negative multiple of 4

typedef struct State State;
typedef struct RenderInfo RenderInfo;
typedef struct Future Future;
typedef struct LabelState LabelState;
typedef struct StyleOverride StyleOverride;
typedef struct Animation Animation;
typedef enum EasingFunction EasingFunction;
typedef enum OverrideType OverrideType;

extern State* s;
extern Color BACKGROUND_COLOR;
extern Color FONT_COLOR;
extern Color CELL_COLOR;
extern char* ErrorMsg;

struct RenderInfo {
	Color button_color;
	double register_height;
	double memory_height;
	double storage_height;
	Rectangle memory_pointer; // Program Counter
	Rectangle storage_pointer; // Active Storage Box
	int last_modified_storage_cell;
	Rectangle popup_box;
	Rectangle x_box;
	double popup_opacity;
	double scroll_offset;

	Future* futures[MAX_FUTURES];
	Animation* animations[MAX_ANIMATIONS]; 
	StyleOverride* style_overrides[MAX_STYLE_OVERRIDES];
};

enum EasingFunction {
	LINEAR,
	IN_N_OUT,
	IN_N_BACK // Treats end_val as the midpoint value and returns to start_val
};

struct Future {
	float delay; // Seconds
	void** reference;
	void* future_value;
	void (*callable)(void); 
};

struct Animation {
	EasingFunction easing;
	double percent; // 0-1 completeness
	float duration; // Seconds
	float delay; // Seconds
	double start_val;
	double end_val;
	double* value;
	StyleOverride* style_override;
};

enum OverrideType {
	NONE,
	MEMORY_CELL_SIZE_MULTIPLIER,
	REGISTER_FADE,
	STORAGE_CELL_SIZE_MULTIPLIER
};

struct StyleOverride {
	OverrideType type;
	int id;
	double style;	
};

struct LabelState {
	int count;
	char* label_names[MAX_LABELS];
	int label_locations[MAX_LABELS];
	int label_jump_counts[MAX_LABELS];
};

struct State {
	RenderInfo render_info;
	int* registers[10];
	char* memory[MEMORY_SIZE];
	char* storage[STORAGE_SIZE];
	LabelState label_state;
	bool haltflag;
	char* error_msg;
};


// ============
// StyleOverrides
// ============
StyleOverride* StyleOverrideCreate(State* state, OverrideType type, int id, float style);
double* StyleOverrideGet(State* state, OverrideType type, int id); // Returns reference to style or NULL
void StyleOverrideDestroy(State* state, StyleOverride* style_override);

// ============
// Futures
// ============
void CreateFuture(State* state, float delay, void** reference, void* future_value, void (*callable)(void));
bool StepFutures(State* state);
void FutureFree(Future* future);

// ============
// Animations
// ============
void CreateAnimation(State* state, double end, double* value, EasingFunction easing, float duration, float delay, StyleOverride* style_override);
bool StepAnimations(State* state);
void AnimationFree(State*, Animation* animation);

// ============
// Complex Animations
// ============
void SetActiveMemoryCell(State*, int cell, EasingFunction easing, float duration, float delay);
void SetActiveStorageCell(State*, int cell, EasingFunction easing, float duration, float delay);
void SetMemoryCellValue(State*,int cell, char* value, float duration, float delay);
void SetRegisterCellValue(State*, int cell, int value, float duration, float delay);
void SetStorageCellValue(State*, int cell, char* value, float duration, float delay);

// ============
// Runners
// ============
void Loop();
void StartVisualisation();
bool Step();

// ============
// Renderers
// ============
void Render(State*);
void RenderRegisters(State*);
void RenderRegister(State*, int i);
void RenderMemory(State*);
void RenderMemoryCell(State*, int i);
void RenderStorage(State*);
void RenderStorageCell(State*, int i);
void RenderControls(State*);
void RenderButton(State*, int x, int y, char* text);
void RenderHeader();
void RenderPopup();

// ============
// User Action Handlers
// ============
bool LoadProgram(char** program, int num_lines);
bool HandleFileDropped();
bool HandleFileUpload(char* path);
void HandleKeyPresses(State*);
float Reset();

// ============
// Preprocess
// ============
void Preprocess(LabelState* ls, char** lines, int num_lines);
char* GetLabelName(int addr); 

#endif // UI_INTERNAL_H_
