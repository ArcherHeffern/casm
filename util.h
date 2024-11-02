#ifndef UTIL_H_
#define UTIL_H_

#include <stdbool.h>

bool IsAlpha(char c);
bool IsWhitespace(char c);
bool IsDigit(char c);
bool IsNonZeroDigit(char c);
bool IsAlpha(char c);
bool ToInteger(char* s, int* i);

#endif
