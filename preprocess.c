#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdio.h>
#include "preprocess.h"

bool IsAlpha(char c) {
	return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

bool IsWhitespace(char c) {
	return c == ' ' || c == '\n' || c == '\t' || c == '\r';
}

int Preprocess(char** lines, int num_lines, char** label_names, int* label_locations) {
	// Resolves labels. We assume newlines have already been removed
	if (error_msg) {
		free(error_msg);
		error_msg = NULL;
	}
	int num_labels = 0;
	for (int i = 0; i < num_lines; i++) {
		char* line = lines[i];
		int cur = 0;
		while (1) {
			if (line[cur] == '\0') break;
			if (line[cur] == ' ') break;
			if (IsAlpha(line[cur])) {};
			if (line[cur] == ':') {
				if (cur == 0) {
					error_msg = malloc(64);
					asprintf(&error_msg, "Cannot have label with no name on line %d", i);
					return -1;
				}
				char* label_name = malloc(cur);
				memcpy(label_name, line, cur);
				// Check for dupliacates 
				for (int j = 0; j < num_labels; j++) {
					if (strcasecmp(label_names[j], label_name) == 0) {
						error_msg = malloc(64);
						asprintf(&error_msg, "Found duplicate labels on lines %d and %d", label_locations[j], i);
						return -1;
					}
				}
				label_names[num_labels] = label_name;
				label_locations[num_labels] = i;
				line = line+cur+1;
				while (IsWhitespace(*line)){line++;}
				if (*line=='\0') {
					error_msg = malloc(64);
					asprintf(&error_msg, "Most have instruction on same line as a label on line %d", i);
					return -1;
				}
				lines[i] = line;
				num_labels++;
				break;
			}
			cur++;
		}
	}
	return num_labels;
}

int main() {
	int num_lines = 5;
	char* lines[] = {
		"L1: hello world",
		"jdskfjsdf",
		"jdskfjsdf dfj",
		"jdskfjsdf: a",
		"jdskfjsdf: a",
	};
	char* label_names[4];
	int label_locations[4];
	
	int num_labels = Preprocess(lines, num_lines, label_names, label_locations);
	if (num_labels < 0) {
		fprintf(stderr, "ERROR: %s\n", error_msg);
	}
	printf("Num Labels: %d\n", num_labels);
	for (int i = 0; i < num_labels; i++) {
		printf("Label '%s' is on line %d\n", label_names[i], label_locations[i]);
	}
	for (int i = 0; i < num_lines; i++) {
		printf("%s\n", lines[i]);
	}
}
