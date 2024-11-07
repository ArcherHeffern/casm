#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdio.h>
#include "util.h"
#include "ui_internal.h"

void Preprocess(LabelState* ls, char** lines, int num_lines) {
	// Removes leading whitespace
	// Resolves labels
	// Removes Comments
	// Removes whitespace after labels
	// We assume newlines have already been removed
	int num_labels = 0;
	for (int i = 0; i < num_lines; i++) {
		char* line = lines[i];
		int cur = 0;
		while (IsWhitespace(*line)){line++;}
		lines[i] = line;
		while (1) {
			if (line[cur] == '\0') break;
			if (line[cur] == ' ') break;
			if (line[cur] == ';') {
				line[cur] = '\0';
				break;
			}
			if (IsAlpha(line[cur])) {};
			if (line[cur] == ':') {
				if (cur == 0) {
					char* error_msg = malloc(64);
					asprintf(&error_msg, "Preprocess Error: Cannot have label with no name on line %d", i);
					return;
				}
				char* label_name = malloc(cur);
				memcpy(label_name, line, cur);
				// Check for dupliacates 
				for (int j = 0; j < num_labels; j++) {
					if (strcasecmp(ls->label_names[j], label_name) == 0) {
						char* error_msg = malloc(64);
						asprintf(&error_msg, "Preprocess Error: Found duplicate labels on lines %d and %d", ls->label_locations[j], i);
						return;
					}
				}
				ls->label_names[num_labels] = label_name;
				ls->label_locations[num_labels] = i;
				line = line+cur+1;
				while (IsWhitespace(*line)){line++;}
				if (*line=='\0') {
					char* error_msg = malloc(64);
					asprintf(&error_msg, "Most have instruction on same line as a label on line %d", i);
					return;
				}
				lines[i] = line;
				num_labels++;
				break;
			}
			cur++;
		}
		// Remove Comments
		while (line[cur] != '\0') {
			if (line[cur] == ';') {
				line[cur] = '\0';
				break;
			}
			cur++;
		}
		// Remove Trailing Space
		cur--;
		while (&line[cur] != line && IsWhitespace(line[cur])) {
			cur--;
		}
		line[cur+1] = '\0';
	}
	ls->count = num_labels;
}

char* PrintJumpLabelBreakdown() {
	char* result = NULL;
    char *temp = NULL;    
	LabelState* ls = &s->label_state;
	asprintf(&result, "Jumps to each label:");

    for (int i = 0; i < ls->count; i++) {
        asprintf(&temp, "\n%s: %d", ls->label_names[i], ls->label_jump_counts[i]);

		char *new_result;
		asprintf(&new_result, "%s%s", result, temp);
		free(result);  
		result = new_result;
		free(temp);    
    }
	return result;
}

int GetLabelAddress(char* label_ref) {
	for (int i = 0; i < s->label_state.count; i++) {
		if (strcmp(label_ref, s->label_state.label_names[i]) == 0) {
			if (++s->label_state.label_jump_counts[i] >= MAX_LABEL_JUMPS) {
				char* error_msg;
				asprintf(&error_msg, "%d jumps performed - Possible infinite loop\n\n%s", MAX_LABEL_JUMPS, PrintJumpLabelBreakdown());
				SetErrorMsg(error_msg);
			}
			free(label_ref);
			return s->label_state.label_locations[i];
		}
	}
	return -1;
}

char* GetLabelName(int addr) {
	LabelState* ls = &s->label_state;
	if (ls == NULL) {
		return NULL;
	}
	for (int i = 0; i < ls->count; i++) {
		if (ls->label_locations[i] == addr/4) {
			return ls->label_names[i];
		}
	}
	return NULL;
}

/*
int main() {
	int num_lines = 5;
	char* lines[] = {
		"L1: hello world",
		"	jdskfjsdf",
		"jdskfjsdf dfj",
		"jdskfjsdf: a",
		"jdskfjsdf: a",
	};
	char* label_names[4];
	int label_locations[4];
	
	int num_labels = Preprocess(lines, num_lines, label_names, label_locations);
	if (num_labels < 0) {
		fprintf(stderr, "ERROR: %s\n", preprocess_error_msg);
	}
	printf("Num Labels: %d\n", num_labels);
	for (int i = 0; i < num_labels; i++) {
		printf("Label '%s' is on line %d\n", label_names[i], label_locations[i]);
	}
	for (int i = 0; i < num_lines; i++) {
		printf("%s\n", lines[i]);
	}
}
*/
