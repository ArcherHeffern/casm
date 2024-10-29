#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <assert.h>
#include <raylib.h>

#define CELL_SIZE 1024
#define MEMORY_SIZE 64
#define STORAGE_SIZE 64

typedef struct State State;
typedef struct Registers Registers;
typedef struct RenderInfo RenderInfo;

struct Registers {
	int pc; // Program Counter
	char* ir; // Instuction Register
	int r1;
	int r2;
	int r3;
	int r4;
	int r5;
	int r6;
	int r7;
	int r8;
	int r9;
}; 

struct RenderInfo {
	Color background;
	float start_time;
	int header_gap;
	double memory_height;
	double storage_height;
	int cell_height;
	int cell_width;
	int cell_gap;
	Color cell_color;
	int scroll_speed;
};

struct State {
	RenderInfo render_info;
	Registers registers;
	char* memory[MEMORY_SIZE];
	char* storage[STORAGE_SIZE];
};

static State* s = NULL;

uint16_t LoadFileIntoMemory(char* filename, char* memory[]);
void plug_init(char* filename);
State* plug_pre_reload();
void plug_update();
void StartVisualisation();
void Render();
int MinInt(int a, int b) {
	return a < b ? a: b;
}
int MaxInt(int a, int b) {
	return a > b ? a: b;
}

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

void plug_init(char* filename) {
	RenderInfo render_info = {
		.background = GetColor(0x181818FF),
		.start_time = 2.5,
		.header_gap = 100,
		.memory_height = 0,
		.storage_height = 0,
		.cell_height = 100,
		.cell_width = 250,
		.cell_gap = 20,
		.cell_color = GetColor(0xcaccde),
		.scroll_speed = 4
	};

	Registers registers;
	registers.pc = 0;
	registers.ir = "";

	s = malloc(sizeof(State));
	assert(s != NULL);
	memset(s, 0, sizeof(*s));
	s->render_info = render_info;
	s->registers = registers;
	LoadFileIntoMemory(filename, s->memory);

	StartVisualisation();
}

State* plug_pre_reload() {
	return s;
}

void plug_post_reload(void* state) {
	s = (State*)state;
}

void StartVisualisation() {
	RenderInfo* render_info = &s->render_info;
	InitWindow(800, 600, "Mini Asm");	
	int initial_screen_height = GetScreenHeight();
	render_info->memory_height = initial_screen_height;
	while (render_info->memory_height > render_info->header_gap) {
		Render();
		double dt = GetFrameTime();
		double movement = initial_screen_height * (dt / render_info->start_time);
		render_info->memory_height -= movement;
	}
}

void plug_update() {
	RenderInfo* render_info = &s->render_info;
	render_info->memory_height -= (int)(GetMouseWheelMove()*render_info->scroll_speed);
	render_info->memory_height = MinInt(render_info->memory_height, render_info->header_gap);
	render_info->memory_height = MaxInt(render_info->memory_height, -(render_info->cell_height*MEMORY_SIZE + render_info->cell_gap*MEMORY_SIZE) + GetScreenHeight());
	Render();
}

void Render() {
	RenderInfo* render_info = &s->render_info;
	BeginDrawing();
	// Memory
	ClearBackground(render_info->background);
	for (int i = 0; i < MEMORY_SIZE; i++) {
		DrawRectangle(
			20,
			render_info->memory_height + render_info->cell_height*i + render_info->cell_gap*i,
			render_info->cell_width,
			render_info->cell_height,
			render_info->cell_color
		);
	}
	EndDrawing();
}


