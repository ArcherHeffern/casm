#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <assert.h>
#include <raylib.h>

#define CELL_SIZE 1024
#define MEMORY_SIZE 64
#define STORAGE_SIZE 64
#define MAX_FUTURES 256
#define MAX_ANIMATIONS 256
#define MAX_STYLE_OVERRIDES 256
#define RESET_DELAY 0.1

typedef struct State State;
typedef struct RenderInfo RenderInfo;
typedef struct Future Future;
typedef struct StyleOverride StyleOverride;
typedef struct Animation Animation;
typedef enum EasingFunction EasingFunction;
typedef enum OverrideType OverrideType;

// Rendering Constants
#define HEADER_SIZE 24
#define TEXT_SIZE 12
#define SLIDE_IN_TIME 0.5 // Seconds
#define MEMORY_CELL_SET_VALUE_TIME 1
#define X_PADDING 40
#define CELL_EXPAND_PERCENT 1.2

struct RenderInfo {
	Color background;
	Color font_color;
	Color pc_color; 
	Color button_color;
	int header_gap;
	double register_height;
	double memory_height;
	double storage_height;
	int register_cell_width;
	int register_cell_height;
	float register_cell_gap; // Percent of cell height
	int cell_height;
	int cell_width;
	int cell_gap;
	Color cell_color;
	int scroll_speed;
	Future* futures[MAX_FUTURES];
	Animation* animations[MAX_ANIMATIONS]; 
	StyleOverride* style_overrides[MAX_STYLE_OVERRIDES];
	Rectangle memory_pointer; // Program Counter
	Rectangle storage_pointer; 
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

void AnimationDbg(Animation* a) {
	printf("{ .%%=%lf, .duration=%lf, .start=%lf, .end=%lf, .value=%lf }\n", a->percent, a->duration, a->start_val, a->end_val, *a->value);
}

struct State {
	RenderInfo render_info;
	int* registers[10];
	char* memory[MEMORY_SIZE];
	char* storage[STORAGE_SIZE];
};

// ============
// Plugins
// ============
void plug_init(char* filename);
State* plug_pre_reload();
void plug_update();

// ============
// StyleOverrides
// ============
StyleOverride* StyleOverrideCreate(OverrideType type, int id, float style);
double* StyleOverrideGet(OverrideType type, int id); // Returns reference to style or NULL
void StyleOverrideDestroy(StyleOverride* style_override);

// ============
// Futures
// ============
void CreateFuture(float delay, void** reference, void* future_value);
void FutureStep(Future* future, float dt);
void FutureFree(Future* future);

// ============
// Animations
// ============
void CreateAnimation(double end, double* value, EasingFunction easing, float duration, float delay, StyleOverride* style_override);
void AnimationStep(Animation* animation, float dt);
void AnimationFree(Animation* animation);

// ============
// Complex Animations
// ============
void SetActiveMemoryCell(int cell, EasingFunction easing, float delay);
void SetActiveStorageCell(int cell, EasingFunction easing, float delay);
void SetMemoryCellValue(int cell, char* value, float delay);
void SetRegisterCellValue(int cell, int value, float delay);
void SetStorageCellValue(int cell, char* value, float delay);

// ============
// Runners
// ============
void Run();
void StartVisualisation();
bool Step();
bool StepAnimations();
bool StepFutures();

// ============
// Renderers
// ============
void Render();
void RenderMemory();
void RenderMemoryCell(int i);
void RenderRegisters();
void RenderRegister(int i);
void RenderStorage();
void RenderStorageCell(int i);
void RenderControls();

// ============
// User Action Handlers
// ============
char* ErrorMsg = NULL;
void HandleFileUpload();
void HandleKeyPresses();
void ContinueProgram();
bool StepProgram(); // True if program is finished or errored. Sets char* ErrorMsg on error
float ResetState();

// ============
// Misc
// ============
float GetMidPoint();
uint16_t LoadFileIntoMemory(char* filepath, char* memory[], float delay, bool animate);

// ============
// Utilities
// ============
int MinInt(int a, int b) {
	return a < b ? a: b;
}
double MinDouble(double a, double b) {
	return a < b ? a: b;
}
float MinFloat(float a, float b) {
	return a < b? a: b;
}
float MaxFloat(float a, float b) {
	return a > b? a: b;
}
int MaxInt(int a, int b) {
	return a > b ? a: b;
}
int BoundInt(int v, int lower, int upper) {
	return MaxInt(MinInt(v, upper), lower);
}
float ParametricBlend(float t) {
	// Source: https://stackoverflow.com/questions/13462001/ease-in-and-ease-out-animation-formula
    float sqr = t * t;
    return sqr / (2.0f * (sqr - t) + 1.0f);
}

float SinInAndBack(float t) {
	// Divinely Inspired (Tsoding)
	return sinf(t*PI);
}


// ============
// Hot Reloading
// ============
static State* s = NULL;

void plug_init(char* filename) {
	RenderInfo render_info = {
		.background = GetColor(0x181818FF),
		.font_color = GetColor(0xFFFFFFFF),
		.pc_color = BLUE,
		.button_color = GRAY,
		.header_gap = 100,
		.register_height = GetScreenHeight(),
		.memory_height = GetScreenHeight(),
		.storage_height = GetScreenHeight(),
		.register_cell_width = 160,
		.register_cell_height = 35,
		.register_cell_gap = 0.15, 
		.cell_height = 65,
		.cell_width = 250,
		.cell_gap = 20,
		.cell_color = GetColor(0xcaccde),
		.scroll_speed = 4,
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
	float mid_y = GetMidPoint() - (s->render_info.memory_pointer.height - s->render_info.cell_height) / 2;
	s->render_info.memory_pointer.y = mid_y;
	s->render_info.memory_pointer.x = X_PADDING - (s->render_info.memory_pointer.width - s->render_info.cell_width) / 2;

	s->render_info.storage_pointer.y = mid_y;
	s->render_info.storage_pointer.x = GetScreenWidth() - X_PADDING - (s->render_info.storage_pointer.width - s->render_info.cell_width) / 2 - s->render_info.cell_width;
	LoadFileIntoMemory(filename, s->memory, 0, false);

	StartVisualisation();
}

State* plug_pre_reload() {
	return s;
}

void plug_post_reload(void* state) {
	s = (State*)state;
}

void plug_update() {
	Step();
}

// -------------
// StyleOverrides 
// -------------
StyleOverride* StyleOverrideCreate(OverrideType type, int id, float style) {
	StyleOverride** style_overrides = s->render_info.style_overrides;
	
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

double* StyleOverrideGet(OverrideType type, int id) {
	for (int i = 0; i < MAX_STYLE_OVERRIDES; i++) {
		StyleOverride* style_override = s->render_info.style_overrides[i];
		if (style_override == NULL) {
			continue;
		}
		if (style_override->type == type && style_override->id == id) {
			return &style_override->style;
		}
	}
	return NULL;
}

void StyleOverrideDestroy(StyleOverride* style_override) {
	StyleOverride** style_overrides = s->render_info.style_overrides;

	for (int i = 0; i < MAX_STYLE_OVERRIDES; i++) {
		if (style_override == style_overrides[i]) {
			style_overrides[i] = NULL;
			free(style_override);
			return;
		}
	}
	assert(false && "Could not find style override");
}
// -------------
// Futures 
// -------------
void CreateFuture(float delay, void** reference, void* future_value) {
	RenderInfo* render_info = &s->render_info;

	Future* future = (Future*)malloc(sizeof(Future));
	future->delay = delay;
	future->reference = reference;
	future->future_value = future_value;
	for (size_t i = 0; i < MAX_FUTURES; i++) {
		if (render_info->futures[i] == NULL) {
			render_info->futures[i] = future;
			return;
		}		
	}
	assert(false && "Future queue is full");
}

void FutureStep(Future* future, float dt) {
	if (future->delay > 0) {
		future->delay = MaxFloat(0, future->delay - dt);
	}
	if (future->delay > 0) {
		return;
	}
	*future->reference = future->future_value;
}

void FutureFree(Future* future) {
	free(future);
}
	
// -------------
// Animations
// -------------
void CreateAnimation(double end, double* value, EasingFunction easing, float duration, float delay, StyleOverride* style_override) {
	RenderInfo* render_info = &s->render_info;
	Animation* animation = (Animation*)malloc(sizeof(Animation));
	animation->easing = easing;
	animation->percent = 0;
	animation->duration = duration; 
	animation->delay = delay;
	animation->start_val = *value;
	animation->end_val = end;
	animation->value = value;
	animation->style_override = style_override;
	for (int i = 0; i < MAX_ANIMATIONS; i++) {
		if (render_info->animations[i] == NULL) {
			render_info->animations[i] = animation;
			return;
		}
	}
	assert(false && "Animation queue is full");
}

void AnimationStep(Animation* animation, float dt) {
	if (animation->delay > 0) {
		float remaining_time = animation->delay - dt;
		animation->delay = MaxFloat(0, remaining_time);
		dt += remaining_time;
	}
	if (animation->delay > 0) {
		return;
	}
	if (animation->duration == 0) {
		animation->percent = 1.0F;
		*animation->value = animation->end_val;
		return;
	}
	animation->percent = MinDouble(dt / animation->duration + animation->percent, 1.0F);
	float percent = animation->percent;
	switch (animation->easing) {
		case LINEAR:
			break;
		case IN_N_OUT:
			percent = ParametricBlend(percent);
			break;
		case IN_N_BACK:
			percent = SinInAndBack(percent);
			break;
		default:
			fprintf(stderr, "Unexpected easing function %d\n", animation->easing);
	}
	float val = animation->start_val + (animation->end_val - animation->start_val) * percent;
	*animation->value = val;
}

void AnimationFree(Animation* animation) {
	if (animation->style_override != NULL) {
		StyleOverrideDestroy(animation->style_override);
	}
	free(animation);
}

// ============
// Complex Animations
// ============
void SetActiveMemoryCell(int cell, EasingFunction easing, float delay) {
	RenderInfo* render_info = &s->render_info;
	float end = -render_info->cell_height*cell - render_info->cell_gap*cell + GetScreenHeight() / 2 - render_info->cell_height / 2;
	CreateAnimation(end, &render_info->memory_height, easing, SLIDE_IN_TIME, delay, NULL);
}

void SetActiveStorageCell(int cell, EasingFunction easing, float delay) {
	RenderInfo* render_info = &s->render_info;
	float end = -render_info->cell_height*cell - render_info->cell_gap*cell + GetScreenHeight() / 2 - render_info->cell_height / 2;
	CreateAnimation(end, &render_info->storage_height, easing, SLIDE_IN_TIME, delay, NULL);
}

void SetMemoryCellValue(int cell, char* value, float delay) {
	// Set text after delay
	CreateFuture(delay, (void*)&s->memory[cell], value);

	// Grow and shrink cell by a percent
	StyleOverride* style_override = StyleOverrideCreate(MEMORY_CELL_SIZE_MULTIPLIER, cell, 1);
	CreateAnimation(CELL_EXPAND_PERCENT, &style_override->style, IN_N_BACK, 1, delay, style_override);
}

void SetStorageCellValue(int cell, char* value, float delay) {
	// Set text after delay
	CreateFuture(delay, (void*)&s->storage[cell], value);

	// Grow and shrink cell by a percent
	StyleOverride* style_override = StyleOverrideCreate(STORAGE_CELL_SIZE_MULTIPLIER, cell, 1);
	CreateAnimation(CELL_EXPAND_PERCENT, &style_override->style, IN_N_BACK, 1, delay, style_override);

}
void SetRegisterCellValue(int cell, int value, float delay) {
	int* i = (int*)malloc(sizeof(int)); // TODO: Fix leak - Idea: Callback on future completions
	*i=value;
	float duration = 1;
	// Set text after delay
	CreateFuture(delay+duration/2, (void*)&s->registers[cell], i);

	// Fade text
	StyleOverride* style_override = StyleOverrideCreate(REGISTER_FADE, cell, 1);
	CreateAnimation(0, &style_override->style, IN_N_BACK, duration, delay, style_override);
}

// ============
// Runners
// ============

void StartVisualisation() {
	RenderInfo* render_info = &s->render_info;
	InitWindow(800, 600, "Mini Asm");	
	Render();
	SetActiveMemoryCell(0, IN_N_OUT, 0);
	Run();
	int gap = render_info->register_cell_height * render_info->register_cell_gap;
	CreateAnimation(
		GetScreenHeight() - 10*(render_info->register_cell_height + gap) - 7,
		&render_info->register_height,
		IN_N_OUT,
		SLIDE_IN_TIME,
		0,
		NULL
	);
	Run();
	SetActiveStorageCell(0, IN_N_OUT, 0);
	Run();
	return;
	SetActiveMemoryCell(1, IN_N_OUT, 0);
	Run();
	SetActiveMemoryCell(0, IN_N_OUT, 0);
	Run();
	SetActiveMemoryCell(3, IN_N_OUT, 0);
	Run();
	SetStorageCellValue(0, "Goodbye World", 0);
	Run();
	SetRegisterCellValue(0, 100, 0);
	Run();
	SetMemoryCellValue(0, "Goodbye World", 0);
	Run();
	return;
}

void Run() {
	while (!WindowShouldClose()) {
		if (!Step()) {
			return;
		}
	}
}

bool Step() {
	bool animations_left = StepAnimations();
	bool futures_left = StepFutures();
	HandleFileUpload();
	HandleKeyPresses();
	Render();
	return animations_left || futures_left;
}

bool StepFutures() {
	RenderInfo* render_info = &s->render_info;
	Future** futures = render_info->futures; 
	bool has_events = false;
	float dt = GetFrameTime();
	for (int i = 0; i < MAX_FUTURES; i++) {
		Future* future = futures[i];
		if (future == NULL) {
			continue;
		} 
		has_events = true;
		FutureStep(future, dt);

		if (future->delay == 0) {
			futures[i] = NULL;
			FutureFree(future);
		}
	}
	return has_events;
}

bool StepAnimations() {
	// @return bool: True if there are animations left
	RenderInfo* render_info = &s->render_info;
	Animation** animations = render_info->animations; 
	bool has_events = false;
	float dt = GetFrameTime();
	for (int i = 0; i < MAX_ANIMATIONS; i++) {
		Animation* animation = animations[i];
		if (animation == NULL) {
			continue;
		} 
		has_events = true;
		if (animation->percent == 1) {
			animations[i] = NULL;
			AnimationFree(animation);
		} else {
			AnimationStep(animation, dt);
		}

	}
	return has_events;
}

// ============
// Rendering
// ============
void Render() {
	BeginDrawing();
		ClearBackground(s->render_info.background);
		RenderMemory();
		RenderRegisters();
		RenderStorage();
		RenderControls();
	EndDrawing();
}

void RenderMemory() {
	RenderInfo* render_info = &s->render_info;
	char** memory = s->memory;

	DrawText("Memory", X_PADDING, render_info->cell_gap, HEADER_SIZE, render_info->font_color); // Title
	DrawRectangleLinesEx(render_info->memory_pointer, 2.0F, render_info->pc_color); // Program Counter
	for (int i = 0; i < MEMORY_SIZE; i++) {
		RenderMemoryCell(i);
	}
}

void RenderMemoryCell(int i) {
	RenderInfo* render_info = &s->render_info;
	char** memory = s->memory;
	double* maybe_multiplier = StyleOverrideGet(MEMORY_CELL_SIZE_MULTIPLIER, i);
	double multiplier = maybe_multiplier == NULL ? 1: *maybe_multiplier;

	int y = render_info->memory_height + render_info->cell_height*i + render_info->cell_gap*i;
	DrawRectangle(
		X_PADDING - 0.5 * (render_info->cell_width * multiplier - render_info->cell_width),
		y - 0.5 * (render_info->cell_height * multiplier - render_info->cell_height),
		render_info->cell_width * multiplier,
		render_info->cell_height * multiplier,
		render_info->cell_color
	);
	char* msg = NULL;
	asprintf(&msg, "Ox%x: %s", i*4, memory[i]);
	float textWidth = MeasureTextEx(GetFontDefault(), msg, TEXT_SIZE, 1).x;
	DrawText(msg, X_PADDING + render_info->cell_width/2 - textWidth/2, y+render_info->cell_height/2, TEXT_SIZE, render_info->font_color);
}

void RenderRegisters() {
	RenderInfo* render_info = &s->render_info;
	float textWidth = MeasureTextEx(GetFontDefault(), "Registers", HEADER_SIZE, 1).x;
	Vector2 position = { .x=GetScreenWidth()/2 - textWidth/2, .y=render_info->cell_gap };
	DrawTextEx(GetFontDefault(), "registers", position, HEADER_SIZE, 1, render_info->font_color);
	for (int i = 0; i < 10; i++) {
		RenderRegister(i);
	}
}

void RenderRegister(int i) {
	RenderInfo* render_info = &s->render_info;
	int x = GetScreenWidth() / 2 - (render_info->register_cell_width/2);
	int gap = render_info->register_cell_height*render_info->register_cell_gap;
	int y = render_info->register_height + i*(render_info->register_cell_height+gap);
	double* maybe_fade = StyleOverrideGet(REGISTER_FADE, i);
	double fade = maybe_fade == NULL ? 1: *maybe_fade;
	Color faded_color = Fade(render_info->font_color, fade);

	DrawRectangle(
		x,
		y,
		render_info->register_cell_width,
		render_info->register_cell_height,
		render_info->cell_color
	);
	char* msg = NULL;
	asprintf(&msg, "R%d: %d", i, *s->registers[i]);
	if (i == 0) {
		free(msg);
		asprintf(&msg, "PC: %d", *s->registers[0]);
	}
	DrawText(msg, x+20, y+render_info->register_cell_height/2, TEXT_SIZE, faded_color);
}

void RenderStorage() {
	RenderInfo* render_info = &s->render_info;
	char** storage = s->storage;

	float textWidth = MeasureTextEx(GetFontDefault(), "Storage", HEADER_SIZE, 1).x;
	DrawText("Storage", GetScreenWidth() - X_PADDING - textWidth, render_info->cell_gap, HEADER_SIZE, render_info->font_color);
	DrawRectangleLinesEx(render_info->storage_pointer, 2.0F, render_info->pc_color); 

	for (int i = 0; i < STORAGE_SIZE; i++) {
		RenderStorageCell(i);
	}
}

void RenderStorageCell(int i) {
	RenderInfo* render_info = &s->render_info;
	char** storage = s->storage;
	int y = render_info->storage_height + render_info->cell_height*i + render_info->cell_gap*i;
	double* maybe_multiplier = StyleOverrideGet(STORAGE_CELL_SIZE_MULTIPLIER, i);
	double multiplier = maybe_multiplier == NULL ? 1: *maybe_multiplier;

	DrawRectangle(
		GetScreenWidth() - X_PADDING - render_info->cell_width - 0.5 * (render_info->cell_width * multiplier - render_info->cell_width),
		y - 0.5 * (render_info->cell_height * multiplier - render_info->cell_height),
		render_info->cell_width * multiplier,
		render_info->cell_height * multiplier,
		render_info->cell_color
	);
	char* msg = NULL;
	asprintf(&msg, "Ox%x: %s", i*4, storage[i]);
	float textWidth = MeasureTextEx(GetFontDefault(), msg, TEXT_SIZE, 1).x;
	DrawText(msg, GetScreenWidth() - X_PADDING - render_info->cell_width/2 - textWidth/2, y+render_info->cell_height/2, TEXT_SIZE, render_info->font_color);
}

void RenderControls() {
	RenderInfo* render_info = &s->render_info;
	char* instruction = "Drag assembly file to upload";
	Vector2 textSize = MeasureTextEx(GetFontDefault(), instruction, TEXT_SIZE, 1);
	DrawText(instruction, GetScreenWidth()/2 - textSize.x/2, render_info->header_gap, TEXT_SIZE, render_info->font_color);
	
	float width = render_info->register_cell_width/2 * (1-render_info->register_cell_gap);
	float height = render_info->register_cell_height + render_info->register_cell_gap;
	float gap = render_info->register_cell_width * render_info->register_cell_gap * 0.5;
	Rectangle continue_button = {
		.x=GetScreenWidth()/2 + gap,
		.y=(render_info->header_gap + textSize.y) * (1+render_info->register_cell_gap),
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
		render_info->font_color
	);
	
	Rectangle step_button = {
		.x=GetScreenWidth()/2 - (render_info->register_cell_width/2),
		.y=(render_info->header_gap + textSize.y) * (1+render_info->register_cell_gap),
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
		render_info->font_color
	);
}

// ============
// User Action Handlers
// ============
void HandleFileUpload() {
	if (!IsFileDropped()) {
		return;
	}
	FilePathList files = LoadDroppedFiles();
	assert(files.count > 0 && "Expected a file to be uploaded");
	char* file_path = files.paths[0];
	
	ResetState();
	LoadFileIntoMemory(file_path, s->memory, CELL_EXPAND_PERCENT, true);
	UnloadDroppedFiles(files);
}

void HandleKeyPresses() {
	if (IsKeyPressed(KEY_T)) {
		ResetState();
	}
	if (IsKeyPressed(KEY_C)) {
		ContinueProgram();
	}
	if (IsKeyPressed(KEY_S)) {
		StepProgram();
	}
}

float ResetState() {
	SetActiveMemoryCell(0, IN_N_OUT, 0);
	SetActiveStorageCell(0, IN_N_OUT, 0);
	for (int i = 0; i < 4; i++) {
		SetMemoryCellValue(i, "000000", i*RESET_DELAY);
		SetStorageCellValue(i, "000000", i*RESET_DELAY);
	}
	for (int i = 0; i < 10; i++) {
		SetRegisterCellValue(i, 0, i*RESET_DELAY);
	}
	return 10 * RESET_DELAY;
}

void ContinueProgram() {
	while (!StepProgram()){
		sleep(1);
	}
}
bool StepProgram() {
	printf("Step\n");
	return false;
}

// ============
// Misc
// ============


uint16_t LoadFileIntoMemory(char* filepath, char* memory[], float delay, bool animate) {
	if (filepath == NULL) {
		return 0;
	}
	FILE* file_p;
	if ((file_p = fopen(filepath, "r")) < 0) {
		perror("Open file: ");
		exit(1);
	}

	uint16_t lines = 0;
	int n_read;
	size_t num_to_read;
	while (1) {
		char* linep = (char*) malloc(CELL_SIZE);
		n_read = getline(&linep, &num_to_read, file_p);
		if (n_read <= 0) {
			break;
		}
		linep[n_read-1] = '\0';
		if (animate) {
			SetMemoryCellValue(lines, linep, delay + RESET_DELAY*lines);
		} else {
			s->memory[lines] = linep;
		}
		lines++;
	}
	return lines;
}

float GetMidPoint() {
	RenderInfo* render_info = &s->render_info;
	return GetScreenHeight() / 2 - render_info->cell_height + render_info->cell_height / 2;
}
