#include <stdio.h>
#include "preprocess.h"
#include "lexer.h"

#define MAX_LABELS 16

int main() {
	int num_lines = 7;
	char* lines[] = {
		"			LOAD R1 =0",
		"			LOAD R2, =10",
		"Label: 	BGEQ R1, R2, Label2", 
		"			ADD R2, R1",
		"			INC R1 ",
		"Label2:	BR Label",
		"HALT"
	};
	char* label_names[MAX_LABELS];
	int label_locations[MAX_LABELS];
	int num_labels = Preprocess(lines, num_lines, label_names, label_locations);
	if (num_labels < 0) {
		fprintf(stderr, "ERROR: %s\n", preprocess_error_msg);
		return 0;
	}
	// for (int i = 0; i < num_lines; i++) {
	// 	printf("%s\n", lines[i]);
	// }
	
	int pc = 0;
	while (1) {
		Token** tokens = TokenizeLine(lines[pc]);
		TokensPrint(tokens);
		if (tokens[0]->type == TOKEN_HALT) {
			break;
		}
		TokenListFree(tokens);
		pc++;
	}
	return 0;
}
// TODO: Errored on space at end of line
