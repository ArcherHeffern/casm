
#ifndef TEST_H_
#define TEST_H_

#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>

#define DEFAULT_RULES_CAPACITY 8
#define TESTFILE_COMMENT '#'
#define AFTER_HALTFLAG "AFTER_HALTFLAG "

enum MemoryType {
    MEMTYPE_REGISTER,
    MEMTYPE_MEMORY,
    MEMTYPE_STORAGE
};

typedef struct {
    int address;
    int expected;
    enum MemoryType memtype;
    bool after_halt;
    char* actual; // Null if successful
    bool should_free_actual; 
} Rule;

typedef struct {
    Rule** rules;
    int size;
    int capacity;
    char* runtime_error;
} Rules;

Rules* RulesCreate(FILE* file);
void RulesAdd(Rules* rules, Rule* rule);
void RulesPrint(Rules* rules);
void RulePrint(Rule* rule);
Rule* ParseLine(char* line, int lineno);
void RulesDestroy(Rules* rules);
void RunTests(Rules* rules);
void RunTest(Rule* rule);
void PrintReport(Rules* rules);
#endif // TEST_H_