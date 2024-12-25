#include <string.h>
#include <assert.h>
#include <unistd.h>
#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <stdbool.h>

#define PI_ 3.14159265358979

bool IsWhitespace(char c) {
	return c == ' ' || c == '\n' || c == '\t' || c == '\r';
}

bool IsDigit(char c) {
	return c <='9' && c >= '0';
}

bool IsNonZeroDigit(char c) {
	return c <='9' && c >= '1';
}

bool IsAlpha(char c) {
	return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

bool ToInteger(char* s, int* i) {
	for (*i=0; *s; s++) {
		if (!IsDigit(*s)) return false;
		*i = *i*10 + (*s-'0');
	}
	return true;
}

char* IntToString(int i) {
	char* s;
	asprintf(&s, "%i", i);
	return s;
}

int MinInt(int a, int b) {
	return a < b ? a: b;
}

double MinDouble(double a, double b) {
	return a < b ? a: b;
}

double MaxDouble(double a, double b) {
	return a > b ? a: b;
}

double ClampDouble(double v, double lower, double upper) {
	return MinDouble(MaxDouble(v, lower), upper);
}

float MinFloat(float a, float b) {
	return a < b? a: b;
}

float MaxFloat(float a, float b) {
	return a > b? a: b;
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

float SinInAndBack(float t) {
	// Divinely Inspired (Tsoding)
	return sinf(t*PI_);
}

char** FileReadLines(char* filepath, int* num_lines, int max_lines, void (*SetErrorMsg)(char*)) {
	*num_lines = 0;
	if (filepath == NULL) {
		return 0;
	}
	FILE* file_p;
	if ((file_p = fopen(filepath, "r")) < 0) {
		perror("Open file: ");
		exit(1);
	}

	char** lines = malloc(sizeof(char*)*max_lines);
	int n_read;
	size_t num_to_read;

	while (1) {
		if (*num_lines > max_lines) {
			char* msg = NULL;
			asprintf(&msg, "Input file too big. Memory is %d lines", max_lines);
			SetErrorMsg(msg);
			return NULL;
		}
		char* linep = (char*) malloc(max_lines);
		n_read = getline(&linep, &num_to_read, file_p);
		if (n_read <= 0) {
			break;
		}
		if (linep[n_read-1] == '\n') {
			linep[n_read-1] = '\0';
		}
		lines[(*num_lines)++] = linep;
	}
	return lines;
}

void JustifyText(char *str, int max_width) {
	// Justifies text in place
	size_t length = strlen(str);
	size_t prev_space = 0;
	size_t cur_length = 0;

	for (int i = 0; i < length; i++) {
		char c = str[i];
		cur_length++;

		if (c == '\n') {
			cur_length = 0;
		}
		else if (IsWhitespace(c)) {
			prev_space = i;
		}
		else if (cur_length > max_width) {
			str[prev_space] = '\n';
			cur_length = i - prev_space;
		}
	}
}

// int main() {
// 	char s[] = "Cannot read register 1 since it contains garbage\nWhile *Technically* valid, I'm assuming this was not intended.";
// 	JustifyText(s, 18);
// 	printf("%s\n", s);
// }

/*
int main() {
	char* s1 = "100";
	int i1 = 0; 
	assert(ToInteger(s1, &i1));
	assert(i1 == 100);

	char* s2 = "100i";
	int i2 = 0; 
	assert(!ToInteger(s2, &i2));
}
*/
