
#ifndef PLUG_H_
#define PLUG_H_

/* Interpreter Constants */
#define MEMORY_SIZE 64
#define STORAGE_SIZE 64
#define MAX_REGISTERS 9
#define MAX_LABEL_JUMPS 1000
#define MAX_LABELS 16


int GetProgramCounter();
int UIGetRegister(int reg_num);
char* UIGetMemory(int address);
char* UIGetStorage(int address);
void UISetRegister(int reg_num, int value);
void UISetMemory(int address, char* value);
void UISetStorage(int address, char* value);

int GetLabelAddress(char* label_ref); // Sets error_msg if exceeds too many jumps
bool GetHaltflag();
char* GetErrorMsg();

void SetHaltflag(bool flag);
void SetErrorMsg(char* msg);
bool HasError();

// Debug
void PrintRegisters();
void PrintMemory();
void PrintMemoryRange(int lower, int upper);
char* PrintJumpLabelBreakdown();
void PrintErrorMsg();

#endif // PLUG_H_
