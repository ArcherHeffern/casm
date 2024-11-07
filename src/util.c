#include <assert.h>
#include <unistd.h>
#include <stdio.h>
#include <math.h>
#include <stdlib.h>

#include "ui_internal.h"

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
	return sinf(t*PI);
}

char** FileReadLines(char* filepath, int* num_lines) {
	*num_lines = 0;
	if (filepath == NULL) {
		return 0;
	}
	FILE* file_p;
	if ((file_p = fopen(filepath, "r")) < 0) {
		perror("Open file: ");
		exit(1);
	}

	char** lines = malloc(sizeof(char*)*MEMORY_SIZE);
	int n_read;
	size_t num_to_read;

	while (1) {
		char* linep = (char*) malloc(CELL_SIZE);
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
