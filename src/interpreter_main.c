#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "ui.h"
#include "ui_internal.h"

#define SHORT_WIDTH 4
#define LONG_WIDTH 30
const char* EMPTY_CELL = "000000";

int main(int argc, char** argv) {
	if (
        (argc == 2 && strcmp(argv[1], "-h") == 0)
        || (argc != 2 && argc != 3)
    ) {
		printf("Usage ./casm input_file [output_file]\n");
		exit(1);
	}

    FILE* f = stdout;
    // Open user provided output file confirm we can overwrite
    if (argc != 3) {}
    else if (strcmp(argv[2], "-") == 0) {}
    else {
        if (FileExists(argv[2])) {
            char exists;
            printf("%s exists. Overwrite? (Y/N)\n", argv[2]);
            scanf("%c", &exists);
            if (exists != 'y' && exists != 'Y') {
                printf("Aborting...\n");
                exit(0);
            }
            printf("Proceeding...\n");
        }
        f = fopen(argv[2], "w");
    } 

    Run(argv[1]);

    fprintf(f, "___Registers___\n");
    fprintf(f, "PC: %d\n", GetProgramCounter());
    for (int i = 1; i < MAX_REGISTERS + 1; i++) {
        fprintf(f, "R%d: %d\n", i, UIGetRegister(i));
    }
    fprintf(f, "\n");
    fprintf(f, "%-*s%-*s%-*s%-*s\n", 
        SHORT_WIDTH*3, 
        "Address", 
        LONG_WIDTH,
        "Labels",
        LONG_WIDTH, 
        "Memory", 
        LONG_WIDTH,
        "Storage"
    );

    bool prev_line_compressed = false;
    for (int i = 0; i < MaxInt(MEMORY_SIZE, STORAGE_SIZE); i++) {
        const char* memory = i<MEMORY_SIZE&&UIGetMemory(i*4)?UIGetMemory(i*4):EMPTY_CELL; 
        const char* storage = i<STORAGE_SIZE&&UIGetStorage(i*4)?UIGetStorage(i*4):EMPTY_CELL;
        bool compress_this_line = memory == EMPTY_CELL && storage == EMPTY_CELL;
        if (!compress_this_line) {
            fprintf(f, "%0*d%-*s%-*s%-*s%-*s\n", 
                SHORT_WIDTH,
                i*4, 
                SHORT_WIDTH*2,
                " ",
                LONG_WIDTH,
                "TODO",
                LONG_WIDTH,
                memory,
                LONG_WIDTH,
                storage
            );
        }
        else if (!prev_line_compressed) {
            printf("...\n");
        }

        prev_line_compressed = compress_this_line;
    } 

    if (HasError()) {
        PrintErrorMsg();
    }

    fclose(f);
}