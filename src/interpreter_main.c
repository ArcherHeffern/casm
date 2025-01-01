#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>

#include "ui.h"
#include "ui_internal.h"
#include "test.h"

#define SHORT_WIDTH 4
#define LONG_WIDTH 30

void PrintHelp(bool error);

int main(int argc, char** argv) {
    char* program_file = NULL;
    char* output_file = NULL;
    char* test_file = NULL;
    int i = 1;
    while (i < argc) {
        char* arg = argv[i];
        if (arg[0] != '-') {
            if (program_file == NULL) {
                program_file = arg;
            } else {
                fprintf(stderr, "2 program files were provided but only 1 is expected\n");
                exit(1);
            }
        } else if (strcmp(arg, "-h") == 0 || strcmp(arg, "-help") == 0) {
            PrintHelp(false);
        } else if (arg[1] == '\0') {
            fprintf(stderr, "Incomplete flag '-'\n");
            exit(1);
        } else if (i == argc-1) {
            fprintf(stderr, "%s is missing its argument or isn't an option\n", arg);
            exit(1);
        } else if (strcmp(arg, "-o") == 0) {
            i++;
            output_file = argv[i];
        } else if (strcmp(arg, "-t") == 0) {
            i++;
            test_file = argv[i];
        }
        i++;
    }
    if (program_file == NULL) {
        PrintHelp(true);
    }

    if (!FileExists(program_file)) {
        fprintf(stderr, "Error: '%s' does not exist\n", program_file);
        exit(1);
    }
    if (FileExists(output_file)) {
        printf("%s exists. Overwrite? (Y/N)\n", output_file);
        char exists = getc(stdin);
        if (exists != 'y' && exists != 'Y') {
            printf("Aborting...\n");
            exit(0);
        }
        printf("Proceeding...\n");
    }

    Rules* rules = NULL;
    if (test_file) {
        if (!FileExists(test_file)) {
            fprintf(stderr, "Test file %s does not exist\n", test_file);
            exit(1);
        }
        FILE* f = fopen(test_file, "r");
        rules = RulesCreate(f);
    }

    Run(program_file);

    if (rules != NULL) {
        RunTests(rules);
        if (output_file) {
            int f = open(output_file, O_WRONLY|O_CREAT|O_TRUNC, 777);
            FileReport(rules, f);
        } else {
            PrintReport(rules);
        }
        exit(0);
    }


    int f = -1;
    if (output_file != NULL) {
        f = open(output_file, O_WRONLY|O_CREAT|O_TRUNC, 777);
        dup2(f, STDOUT_FILENO);
        dup2(f, STDOUT_FILENO);
    }
    printf("___Registers___\n");
    printf("PC: %d\n", GetProgramCounter());
    for (int i = 1; i < MAX_REGISTERS + 1; i++) {
        printf("R%d: %d\n", i, UIGetRegister(i));
    }
    printf("\n");
    printf("%-*s%-*s%-*s%-*s\n", 
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
        char* label_name = GetLabelName(i*4);
        if (!compress_this_line) {
            printf("%0*d%-*s%-*s%-*s%-*s\n", 
                SHORT_WIDTH,
                i*4, 
                SHORT_WIDTH*2,
                " ",
                LONG_WIDTH,
                label_name!=NULL?label_name:"",
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

    if (f != -1) {
        close(f);
    }
}

void PrintHelp(bool error) {
    printf("Usage ./casm input_file [-o output_file] [-t test_file]\n");
    printf("OPTIONS: \n");
    printf("-h|-help: Print help message\n");
    printf("-o|-outfile: Output File\n");
    printf("-t|-testfile: Test File\n");
    printf("Pairing -testfile and -outfile writes test results in a program parsable format documented in README.md:## Test Output File Format \n");
    exit(error);
}