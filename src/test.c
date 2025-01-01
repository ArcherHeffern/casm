#include "test.h"

Rules* RulesCreate(FILE* file) {
    Rules* rules = malloc(sizeof(Rules));
    rules->size = 0;
    rules->capacity = DEFAULT_RULES_CAPACITY;
    rules->rules = malloc(sizeof(int)*DEFAULT_RULES_CAPACITY);

    char *line = NULL;
    size_t linecap = 0;
    ssize_t linelen;
    int lineno = 0; 
    while ((linelen = getline(&line, &linecap, file)) > 0) {

        Rule* maybe_rule = ParseLine(line, lineno);
        if (maybe_rule != NULL) {
            RulesAdd(rules, maybe_rule);
        }
        lineno++;
    }
    return rules;
}

void RulesAdd(Rules* rules, Rule* rule) {
    if (rules->size == rules->capacity) {
        rules->capacity *= 2;
        rules->rules = realloc(rules->rules, sizeof(Rule*)*rules->capacity);
    }
    rules->rules[rules->size] = rule;
    rules->size++;
}

void RulesPrint(Rules* rules) {
    for (int i = 0; i < rules->size; i++) {
        RulePrint(rules->rules[i]);
    }
}

void RulePrint(Rule* rule) {
    char c;
    switch (rule->memtype) {
    case MEMTYPE_REGISTER:
        c = 'R';
        break;
    case MEMTYPE_MEMORY:
        c = 'M';
        break;
    case MEMTYPE_STORAGE:
        c = 'S';
        break;
    }
    printf("%c%d %d\n", c, rule->address, rule->expected);
}

Rule* ParseLine(char* line, int lineno) {
    // If error: exit program 
    // Return NULL if there is no rule
    enum MemoryType memtype;
    int address = 0;
    int expected = 0;
    bool after_halt = false;
    int i = 0; 
    while (line[i] != '\0') {
        if (line[i] == TESTFILE_COMMENT) {
            line[i] = '\0';
            break;
        }
        i++;
    }

    int cur = 0;
    while (isspace(line[cur])) {
        cur++;
    }
    if (line[cur] == '\0') {
        return NULL;
    }

    switch (line[cur]) {
    case 'R':
        memtype = MEMTYPE_REGISTER;
        break;
    case 'M':
        memtype = MEMTYPE_MEMORY;
        break;
    case 'S':
        memtype = MEMTYPE_STORAGE;
        break;
    default:
        fprintf(stderr, "Expected rule to begin with R, M, or S, but found %c\n on line %d", line[cur], lineno);
        exit(1);
    }
    cur++;
    while (isspace(line[cur])) {
        cur++;
    }
    if (line[cur] == '\0') {
        fprintf(stderr, "Incomplete rule on line %d. Missing LOCATION and EXPECTED_VALUE\n", lineno);
        exit(1);
    }
    
    // Consume address or AFTER_HALTFLAG
    if (strcmp(line+cur, AFTER_HALTFLAG) == 0) { // AFTER_HALTFLAG contains the space
        after_halt = true;
        cur += strlen(AFTER_HALTFLAG);
    } else if (isdigit(line[cur])) {
        while (isdigit(line[cur])) {
            address = address * 10 + line[cur] - '0';
            cur++;
        }
        if (!isspace(line[cur])) {
            fprintf(stderr, "Address must have whitespace afterwards on line %d\n", lineno);
            exit(1);
        }
        if (address % 4 != 0 && (memtype == MEMTYPE_MEMORY || memtype == MEMTYPE_STORAGE)) {
            fprintf(stderr, "Address must be a multiple of 4 on line %d\n", lineno);
            exit(1);
        }
        if (address < 0) {
            fprintf(stderr, "Address cannot be negative on line %d. Possible overflow error\n", lineno);
            exit(1);
        }
    } else {
        fprintf(stderr, "Incomplete rule on line %d. Bad LOCATION\n", lineno);
        exit(1);
    }

    while (isspace(line[cur])) {
        cur++;
    }

    if (line[cur] == '\0' || !isdigit(line[cur])) {
        fprintf(stderr, "Incomplete rule on line %d\n. Missing EXPECTED_VALUE", lineno);
        exit(1);
    }

    while (isdigit(line[cur])) {
        expected = expected * 10 + line[cur] - '0';
        cur++;
    }

    while (isspace(line[cur])) {
        cur++;
    }

    if (line[cur] != '\0') {
        fprintf(stderr, "Too much on line %d\n", lineno);
        exit(1);
    }

    Rule* rule = malloc(sizeof(Rule));
    rule->memtype = memtype;
    rule->address = address;
    rule->after_halt = after_halt;
    rule->expected = expected;
    return rule;
}

void RulesDestroy(Rules* rules) {
    for (int i = 0; i < rules->size; i++) {
        if (rules->rules[i]->should_free_actual) {
            free(rules->rules[i]->actual);
        }
        free(rules->rules[i]);
    }
    free(rules);
}


void RunTests(Rules* rules) {
    if (HasError()) {
        rules->runtime_error = GetErrorMsg();
    }
    for (int i = 0; i < rules->size; i++) {
        RunTest(rules->rules[i]);
    }
}

void RunTest(Rule* rule) {
    rule->actual = NULL;
    rule->should_free_actual = false;
    if (rule->after_halt) {
        ;
    } else {
        char* value = NULL;
        switch (rule->memtype) {
        case MEMTYPE_REGISTER: {
            int n = UIGetRegister(rule->address);
            if (n == GARBAGE) {
                value = EMPTY_CELL;
            } else {
                asprintf(&value, "%d", n);
            }
        }
        break;
        case MEMTYPE_MEMORY:
            value = UIGetMemory(rule->address);
        break;
        case MEMTYPE_STORAGE:
            value = UIGetStorage(rule->address);
            break;
        }
        int v = 0;
        if (value == NULL) {
            rule->actual = EMPTY_CELL;
        } else if (!ToInteger(value, &v)) {
            rule->actual = value;
        } else if (v != rule->expected) {
            rule->actual = value;
        }
    }
}

void FileReport(Rules* rules, int fd) {
    char null_term = '\0';
    if (rules->runtime_error) {
        write(fd, rules->runtime_error, strlen(rules->runtime_error));
    }
    write(fd, &null_term, 1);
    for (int i = 0; i < rules->size; i++) {
        if (rules->rules[i]->actual != NULL) {
            write(fd, rules->rules[i]->actual, strlen(rules->rules[i]->actual));
        }
        write(fd, &null_term, 1);
    }
}

void PrintReport(Rules* rules) {
    if (rules->runtime_error) {
        int pc = GetProgramCounter();
        printf("Error at address %d executing '%s'\n", pc*4, UIGetMemory(pc*4)?UIGetMemory(pc*4): EMPTY_CELL);
    }
    for (int i = 0; i < rules->size; i++) {
        Rule* rule = rules->rules[i];
        char c;
        switch (rule->memtype) {
        case MEMTYPE_REGISTER:
            c = 'R';
            break;
        case MEMTYPE_MEMORY:
            c = 'M';
            break;
        case MEMTYPE_STORAGE:
            c = 'S';
            break;
        }
        printf("Test %d: ", i+1);
        if (rule->actual == NULL) {
            printf("OK\n");
        } else {
            printf("ERR: Expect %c[%d]=%d, found '%s'\n", c, rule->address, rule->expected, rule->actual);
        }
    }
}
// int main() {
//     FILE* f = fopen("./test/basic.test", "r");
//     if (f == NULL) {
//         fprintf(stderr, "File could not be opened\n");
//     }
//     Run("");
//     Rules* rules = RulesCreate(f);
//     RulesPrint(rules);
//     UISetMemory(12, "44");
//     RunTests(rules);
//     PrintReport(rules);
//     RulesDestroy(rules);
// }