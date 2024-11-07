#ifndef CASM_INTERNAL_H_
#define CASM_INTERNAL_H_

typedef struct Scanner {
	TokenList* token_list;
	int cur;
} Scanner;

typedef struct Register {
	int index;
	int value;
} Register;

// Scanner
Token* Advance(Scanner* scanner);
Token* Check(Scanner* scanner, TokenType token_type);
Token* Consume(Scanner* scanner, TokenType token_type);
Token* Peek(Scanner* scanner);
Token* Prev(Scanner* scanner);
bool IsAtEnd(Scanner* scanner);
// Executors
int ExecuteInstruction(Scanner* scanner); // Returns new program counter
void ExecuteMath(TokenType instruction, Scanner* scanner);
void ExecuteInc(Scanner* scanner);
void ExecuteLoad(Scanner* scanner);
void ExecuteStore(Scanner* scanner);
void ExecuteRead(Scanner* scanner);
void ExecuteWrite(Scanner* scanner);
int ExecuteBr(Scanner* scanner);
int ExecuteConditionalBranch(TokenType instruction, Scanner* scanner);
// Jump Helper
int ScanLabelIndex(Scanner* scanner, bool increment_count);
// Addressing Combinations
int ScanLoadValue(Scanner* scanner);
int ScanStoreAddress(Scanner* scanner);
int ScanReadValue(Scanner* scanner);
int ScanWriteAddress(Scanner* scanner);
// Addressing Primatives
int ScanDirectAddress(Scanner* scanner);
int ScanImmediateAddressValue(Scanner* scanner);
int ScanIndexAddress(Scanner* scanner);
int ScanIndirectAddress(Scanner* scanner);
int ScanRelativeAddress(Scanner* scanner);
Register ScanRegister(Scanner* scanner);
int ScanNumberValue(Scanner* scanner);
// Getters
int GetRegister(int reg_num);
int GetMemory(int address);
int GetStorage(int address);
// Setters
void SetProgramCounter(int pc);
void SetRegister(int num, int value);
void SetMemory(int num, char* value);
void SetStorage(int num, char* value);
void SetErrorMsg(char* msg);
// Debug Info
void PrintRegisters();
void PrintMemory();
void PrintMemoryRange(int lower, int upper); // Inclusive on both bounds
// Main
int main();
// Tests
void SetupTests();
void TearDownTests();
bool TestLoadProgram(char** program, int num_lines);
int	MockGetRegister(int reg_num);
char* MockGetMemory(int address);
char* MockGetStorage(int address);
void MockSetRegister(int reg_num, int value);
void MockSetMemory(int address, char* value);
void MockSetStorage(int address, char* value);
void LoadTest();
void MathTest();
void StoreTest();
void StorageTest();
void LoopTest();

#endif // CASM_INTERNAL_H_
