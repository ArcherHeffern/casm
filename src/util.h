#ifndef UTIL_H_
#define UTIL_H_

#include <stdbool.h>

#define BEGIN printf("BEGIN\n");
#define END printf("END\n");

bool IsWhitespace(char c);
bool IsDigit(char c);
bool IsNonZeroDigit(char c);
bool IsAlpha(char c);
bool ToInteger(char* s, int* i);
char* IntToString(int i);
int MinInt(int a, int b);
double MinDouble(double a, double b);
double MaxDouble(double a, double b);
double ClampDouble(double v, double lower, double upper);
float MinFloat(float a, float b);
float MaxFloat(float a, float b);
int MaxInt(int a, int b);
int BoundInt(int v, int lower, int upper);
float ParametricBlend(float t);
float SinInAndBack(float t);
char** FileReadLines(char* filepath, int* num_lines, int max_lines, void (*SetErrorMsg)(char*));
void JustifyText(const char *str, int max_width);

#endif
