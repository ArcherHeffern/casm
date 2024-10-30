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
#define MAX_ANIMATIONS 256
#define RESET_DELAY 0.1

typedef struct State State;
typedef struct RenderInfo RenderInfo;
typedef struct Animation Animation;
typedef enum EasingFunction EasingFunction;

// Rendering Constants
#define SLIDE_IN_TIME 0.5 // Seconds
#define X_PADDING 40

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
	Animation* animations[MAX_ANIMATIONS]; 
	Rectangle memory_pointer; // Program Counter
	Rectangle storage_pointer; 
};

enum EasingFunction {
	LINEAR,
	IN_N_OUT,
	IN_N_BACK // Treats end_val as the midpoint value and returns to start_val
};

struct Animation {
	EasingFunction easing;
	double percent; // 0-1 completeness
	float duration; // Seconds
	float delay; // Seconds
	double start_val;
	double end_val;
	double* value;
};

void AnimationDbg(Animation* a) {
	printf("{ .%%=%lf, .duration=%lf, .start=%lf, .end=%lf, .value=%lf }\n", a->percent, a->duration, a->start_val, a->end_val, *a->value);
}

struct State {
	RenderInfo render_info;
	int registers[10];
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
// Animations
// ============
void CreateAnimation(double end, double* value, EasingFunction easing, float delay);
void AnimationStep(Animation* animation, float dt);
void SetActiveMemoryCell(int cell, EasingFunction easing, float delay);
void SetActiveStorageCell(int cell, EasingFunction easing, float delay);
void AnimationFree(Animation* animation);

// ============
// Runners
// ============
void Run();
void StartVisualisation();
bool Step();
bool StepAnimations();

// ============
// Renderers
// ============
void Render();
void RenderMemory();
void RenderRegisters();
void RenderRegister(int i);
void RenderStorage();
void RenderControls();

// ============
// Misc
// ============
float GetMidPoint();
void HandleFileUpload();
uint16_t LoadFileIntoMemory(char* filepath, char* memory[]);

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
		.animations = { NULL },
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
	s->render_info = render_info;
	float mid_y = GetMidPoint() - (s->render_info.memory_pointer.height - s->render_info.cell_height) / 2;
	s->render_info.memory_pointer.y = mid_y;
	s->render_info.memory_pointer.x = X_PADDING - (s->render_info.memory_pointer.width - s->render_info.cell_width) / 2;

	s->render_info.storage_pointer.y = mid_y;
	s->render_info.storage_pointer.x = GetScreenWidth() - X_PADDING - (s->render_info.storage_pointer.width - s->render_info.cell_width) / 2 - s->render_info.cell_width;
	LoadFileIntoMemory(filename, s->memory);

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
// Animations
// -------------
void CreateAnimation(double end, double* value, EasingFunction easing, float delay) {
	RenderInfo* render_info = &s->render_info;
	Animation* animation = (Animation*)malloc(sizeof(Animation));
	animation->easing = easing;
	animation->percent = 0;
	animation->duration = SLIDE_IN_TIME; 
	animation->delay = delay;
	animation->start_val = *value;
	animation->end_val = end;
	animation->value = value;
	for (int i = 0; i < MAX_ANIMATIONS; i++) {
		if (render_info->animations[i] == NULL) {
			render_info->animations[i] = animation;
			break;
		}
	}
}

void SetActiveMemoryCell(int cell, EasingFunction easing, float delay) {
	RenderInfo* render_info = &s->render_info;
	float end = -render_info->cell_height*cell - render_info->cell_gap*cell + GetScreenHeight() / 2 - render_info->cell_height / 2;
	CreateAnimation(end, &render_info->memory_height, easing, delay);
}

void SetActiveStorageCell(int cell, EasingFunction easing, float delay) {
	RenderInfo* render_info = &s->render_info;
	float end = -render_info->cell_height*cell - render_info->cell_gap*cell + GetScreenHeight() / 2 - render_info->cell_height / 2;
	CreateAnimation(end, &render_info->storage_height, easing, delay);
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
	animation->percent = MinDouble(dt / animation->duration + animation->percent, 1.0F);
	float percent = animation->percent;
	switch (animation->easing) {
		case LINEAR:
			break;
		case IN_N_OUT:
			percent = ParametricBlend(percent);
			break;
		default:
			fprintf(stderr, "Unexpected easing function %d\n", animation->easing);
	}
	float val = animation->start_val + (animation->end_val - animation->start_val) * percent;
	*animation->value = val;
}

void AnimationFree(Animation* animation) {
	free(animation);
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
	CreateAnimation(
		GetScreenHeight() - 10*(render_info->register_cell_height + render_info->register_cell_gap),
		&render_info->register_height,
		IN_N_OUT,
		0
	);
	Run();
	SetActiveStorageCell(0, IN_N_OUT, 0);
	Run();
	SetActiveMemoryCell(1, IN_N_OUT, 0);
	Run();
	SetActiveMemoryCell(2, IN_N_OUT, 0);
	Run();
	SetActiveMemoryCell(3, IN_N_OUT, 1);
	return;
	Run();
	// CreateRegisterSeekEvent(render_info->register_cell_height+, IN_N_OUT);
	Run();
	CreateAnimation(
		0, 
		&render_info->storage_height, 
		IN_N_OUT,
		0
	);
	Run();
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
	HandleFileUpload();
	Render();
	return animations_left;
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
		AnimationStep(animation, dt);

		if (animation->percent == 1) {
			animations[i] = NULL;
			AnimationFree(animation);
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

	DrawText("Memory", X_PADDING, render_info->cell_gap, 24, render_info->font_color);
	for (int i = 0; i < MEMORY_SIZE; i++) {
		int y = render_info->memory_height + render_info->cell_height*i + render_info->cell_gap*i;
		DrawRectangle(
			X_PADDING,
			y,
			render_info->cell_width,
			render_info->cell_height,
			render_info->cell_color
		);
		char* msg = NULL;
		asprintf(&msg, "Ox%x: %s", i*4, memory[i]);
		DrawText(msg, X_PADDING + 20, y+render_info->cell_height/2, 12, render_info->font_color);
	}
	DrawRectangleLinesEx(render_info->memory_pointer, 2.0F, render_info->pc_color); 
}

void RenderRegisters() {
	RenderInfo* render_info = &s->render_info;
	float textWidth = MeasureTextEx(GetFontDefault(), "Registers", 24, 1).x;
	Vector2 position = { .x=GetScreenWidth()/2 - textWidth/2, .y=render_info->cell_gap };
	DrawTextEx(GetFontDefault(), "registers", position, 24, 1, render_info->font_color);
	for (int i = 0; i < 10; i++) {
		RenderRegister(i);
	}
}

void RenderRegister(int i) {
	RenderInfo* render_info = &s->render_info;
	int x = GetScreenWidth() / 2 - (render_info->register_cell_width/2);
	int y = render_info->register_height + render_info->register_cell_height* (1+render_info->register_cell_gap) * (i-1) - 20;
	DrawRectangle(
		x,
		y,
		render_info->register_cell_width,
		render_info->register_cell_height,
		render_info->cell_color
	);
	char* msg = NULL;
	asprintf(&msg, "R%d: %d", i, s->registers[i]);
	if (i == 0) {
		free(msg);
		asprintf(&msg, "PC: %d", s->registers[0]);
	}
	DrawText(msg, x+20, y+render_info->register_cell_height/2, 12, render_info->font_color);
}

void RenderStorage() {
	RenderInfo* render_info = &s->render_info;
	char** storage = s->storage;

	float textWidth = MeasureTextEx(GetFontDefault(), "Storage", 24, 1).x;
	DrawText("Storage", GetScreenWidth() - X_PADDING - textWidth, render_info->cell_gap, 24, render_info->font_color);
	for (int i = 0; i < STORAGE_SIZE; i++) {
		int y = render_info->storage_height + render_info->cell_height*i + render_info->cell_gap*i;
		DrawRectangle(
			GetScreenWidth() - X_PADDING - render_info->cell_width,
			y,
			render_info->cell_width,
			render_info->cell_height,
			render_info->cell_color
		);
		char* msg = NULL;
		asprintf(&msg, "Ox%x: %s", i*4, storage[i]);
		DrawText(msg, GetScreenWidth() - X_PADDING - render_info->cell_width + 20, y+render_info->cell_height/2, 12, render_info->font_color);
	}
	DrawRectangleLinesEx(render_info->storage_pointer, 2.0F, render_info->pc_color); 
}

void RenderControls() {
	RenderInfo* render_info = &s->render_info;
	char* instruction = "Drag assembly file to upload";
	Vector2 textSize = MeasureTextEx(GetFontDefault(), instruction, 12, 1);
	DrawText(instruction, GetScreenWidth()/2 - textSize.x/2, render_info->header_gap, 12, render_info->font_color);
	
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
	textSize = MeasureTextEx(GetFontDefault(), continue_text, 12, 1);
	DrawText(
		continue_text, 
		continue_button.x+continue_button.width/2-textSize.x/2, 
		continue_button.y+continue_button.height/2-textSize.y/2, 
		12, 
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
	textSize = MeasureTextEx(GetFontDefault(), step_text, 12, 1);
	DrawText(
		step_text, 
		step_button.x+step_button.width/2-textSize.x/2, 
		step_button.y+step_button.height/2-textSize.y/2, 
		12, 
		render_info->font_color
	);
}
// ============
// Misc
// ============

void HandleFileUpload() {
	if (!IsFileDropped()) {
		return;
	}
	FilePathList files = LoadDroppedFiles();
	assert(files.count > 0 && "Expected a file to be uploaded");
	char* file_path = files.paths[0];
	LoadFileIntoMemory(file_path, s->memory);
	UnloadDroppedFiles(files);
}

void ResetState() {
	SetActiveMemoryCell(0, IN_N_OUT, 0);
	SetActiveStorageCell(0, IN_N_OUT, 0);
	// TODO: Go booop boop boop each cell that now has a value and removed a value (Memory and Storage)
	// TODO: Reset the registers one after another
}

uint16_t LoadFileIntoMemory(char* filepath, char* memory[]) {
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
		memory[lines++] = linep;
	}
	return lines;
}

float GetMidPoint() {
	RenderInfo* render_info = &s->render_info;
	return GetScreenHeight() / 2 - render_info->cell_height + render_info->cell_height / 2;
}
