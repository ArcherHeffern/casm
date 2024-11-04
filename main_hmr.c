#include <stdio.h>
#include <string.h>
#include <raylib.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <dlfcn.h>

#include "plug_main.h"


void* libplug = NULL;
const char* dl_path = "libplug.so";
void* state = NULL;

void ReloadPlugin();

int main(int argc, char** argv) {
	if (argc == 2 && strcmp(argv[1], "-h") == 0) {
		printf("Usage ./casm [filename]\n");
		exit(1);
	}
	char* filename = NULL;
	if (argc == 2) {
		filename = argv[1];
	}


	InitWindow(800, 600, "Mini Asm");	
	SetTargetFPS(60);
	ReloadPlugin();
	
	yerp:
	plug_init(filename);
	while (!WindowShouldClose()) {
		if (IsKeyPressed(KEY_T)) {
			state = plug_pre_reload();
			ReloadPlugin();
			plug_post_reload(state);
			goto yerp;
		}
		plug_update();
	}
}


void ReloadPlugin() {
	if (libplug != NULL) {
		if (dlclose(libplug) == -1) {
			fprintf(stderr, "ERROR: %s\n", dlerror());
			exit(1);
		}
	}

	libplug = dlopen(dl_path, RTLD_NOW);
	if (libplug == NULL) {
		fprintf(stderr, "ERROR: %s\n", dlerror());
		exit(1);
	}
	plug_init = dlsym(libplug, "plug_init");
	if (plug_init == NULL) {
		fprintf(stderr, "ERROR: %s\n", dlerror());
		exit(1);
	}

	plug_pre_reload = dlsym(libplug, "plug_pre_reload");
	if (plug_pre_reload == NULL) {
		fprintf(stderr, "ERROR: %s\n", dlerror());
		exit(1);
	}
	plug_post_reload = dlsym(libplug, "plug_post_reload");
	if (plug_post_reload == NULL) {
		fprintf(stderr, "ERROR: %s\n", dlerror());
		exit(1);
	}

	plug_update = dlsym(libplug, "plug_update");
	if (plug_update == NULL) {
		fprintf(stderr, "ERROR: %s\n", dlerror());
		exit(1);
	}
}

