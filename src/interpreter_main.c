#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "ui.h"
#include "ui_internal.h"

#define SHORT_WIDTH 4
#define LONG_WIDTH 30

int main(int argc, char** argv) {
	if (
        (argc == 2 && strcmp(argv[1], "-h") == 0)
        || (argc != 2 && argc != 3)
    ) {
		printf("Usage ./casm input_file [output_file]\n");
		exit(1);
	}

    char* out_file = stdout;
    // Open user provided output file confirm we can overwrite
    if (argc == 3) {
        out_file = argv[2];
        if (FileExists(out_file)) {
            char exists;
            printf("%s exists. Overwrite? (Y/N)\n", out_file);
            scanf("%c", &exists);
            if (exists != 'y' && exists != 'Y') {
                printf("Aborting...\n");
                exit(0);
            }
            printf("Proceeding...\n");
        }
    } 

    Run(argv[1]);
    if (HasError()) {
        PrintErrorMsg();
    }

    FILE* f = fopen(out_file, "w");
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

    for (int i = 0; i < MaxInt(MEMORY_SIZE, STORAGE_SIZE); i++) {
        fprintf(f, "%0*d%-*s%-*s%-*s%-*s\n", 
            SHORT_WIDTH,
            i*4, 
            SHORT_WIDTH*2,
            " ",
            LONG_WIDTH,
            "TODO",
            LONG_WIDTH,
            i<MEMORY_SIZE&&UIGetMemory(i*4)?UIGetMemory(i*4):"000000", 
            LONG_WIDTH,
            i<STORAGE_SIZE&&UIGetStorage(i*4)?UIGetStorage(i*4):"000000"
        );
    }
    fclose(f);
}