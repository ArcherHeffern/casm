#include "util.h"
#include <assert.h>
#include <stdio.h>

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
