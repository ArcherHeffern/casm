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
#define MAX_ANIMATIONS 8

typedef struct State State;
typedef struct RenderInfo RenderInfo;
typedef struct Animation Animation;
typedef enum EasingFunction EasingFunction;

// Rendering Constants
#define SLIDE_IN_TIME 1.5 // Seconds
#define X_PADDING 40

struct RenderInfo {
	Color background;
	Color font_color;
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
	Rectangle pointer; // Program Counter
};

enum EasingFunction {
	LINEAR,
	IN_N_OUT
};

struct Animation {
	EasingFunction easing;
	double percent; // 0-1 completeness
	float duration; // Seconds
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
void CreateAnimation(double end, double* value, EasingFunction easing);
void AnimationStep(Animation* animation, float dt);
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

// ============
// Misc
// ============
float GetMidPoint();
uint16_t LoadFileIntoMemory(char* filename, char* memory[]);

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
		.pointer = {
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
	s->render_info.pointer.y = GetMidPoint() - (s->render_info.pointer.height - s->render_info.cell_height) / 2;
	s->render_info.pointer.x = X_PADDING - (s->render_info.pointer.width - s->render_info.cell_width) / 2;
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
void CreateAnimation(double end, double* value, EasingFunction easing) {
	RenderInfo* render_info = &s->render_info;
	Animation* animation = (Animation*)malloc(sizeof(Animation));
	animation->easing = easing;
	animation->percent = 0;
	animation->duration = SLIDE_IN_TIME; 
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

void SetActiveMemoryCell(int cell, EasingFunction easing) {
	RenderInfo* render_info = &s->render_info;
	float end = -render_info->cell_height*cell - render_info->cell_gap*cell + GetScreenHeight() / 2 - render_info->cell_height / 2;
	CreateAnimation(end, &render_info->memory_height, easing);
}

void AnimationStep(Animation* animation, float dt) {
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
	SetActiveMemoryCell(0, IN_N_OUT);
	Run();
	SetActiveMemoryCell(1, IN_N_OUT);
	Run();
	SetActiveMemoryCell(2, IN_N_OUT);
	Run();
	return;
	Run();
	// CreateRegisterSeekEvent(render_info->register_cell_height+, IN_N_OUT);
	Run();
	CreateAnimation(0, &render_info->storage_height, IN_N_OUT);
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
		DrawText(msg, X_PADDING, y+render_info->cell_height/2, 12, render_info->font_color);
	}
	DrawRectangleLinesEx(render_info->pointer, 2.0F, BLUE); 
}

void RenderRegisters() {
	RenderInfo* render_info = &s->render_info;
	float textWidth = MeasureTextEx(GetFontDefault(), "Registers", 24, 1).x;
	Vector2 position = { .x=GetScreenWidth()/2 - textWidth/2, .y=render_info->cell_gap };
	DrawTextEx(GetFontDefault(), "registers", position, 24, 1, render_info->font_color);
	RenderRegister(9);
	//for (int i = 0; i < 10; i++) {
		//RenderRegister(i);
	//}
}

void RenderRegister(int i) {
	RenderInfo* render_info = &s->render_info;
	int x = GetScreenWidth() / 2 - (render_info->register_cell_width/2);
	int y = GetScreenHeight(); //+ render_info->register_height - render_info->register_cell_height* 1.15 * (9-i);
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
	DrawText(msg, x, y+render_info->register_cell_height/2, 12, render_info->font_color);
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
		DrawText(msg, GetScreenWidth() - X_PADDING - render_info->cell_width, y+render_info->cell_height/2, 12, render_info->font_color);
	}
}

// ============
// Misc
// ============
uint16_t LoadFileIntoMemory(char* filename, char* memory[]) {
	FILE* file_p;
	if ((file_p = fopen(filename, "r")) < 0) {
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
