#include <stdio.h>
#include <string.h>
#include <raylib.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <dlfcn.h>
#include "ui_internal.h"

int main(int argc, char** argv) {
	if (argc == 2 && strcmp(argv[1], "-h") == 0) {
		printf("Usage ./casm [filename]\n");
		exit(1);
	}
	char* filename = NULL;
	if (argc == 2) {
		filename = argv[1];
	}

	Run(filename);
}

