# Brandeis Operating Systems MiniASM Interpreter and Visualizer

## Visualizer
__[Online Demo using Webassembly](https://archerheffern.github.io/casm)__  

## Interpreter

![interpreter_big_loop](https://github.com/user-attachments/assets/a58860d3-428e-4d80-9dab-84235162515c)

## Features
- Easily upload assembly files using drag and drop
- Testing Framework
- Visualize with single step and continue. Or jump to the end
- Get fast feedback to program changes. Casm detects changes to your assembly source file and automatically reloads file
- Find syntax errors quickly with descriptive error messages
- Catches possible issues like infinite loops, memory overwrite bugs, and UB (eg. Reading from uninitialized memory)

## Getting Started
1. Use online demo

2. Build from source
``` bash
	git clone https://github.com/ArcherHeffern/casm.git
	make [all|interp|vis]
	./macos/visualizer [assembly_program]
```
NOTE: I have not yet tested the windows target. Let me know if you run into any issues running these

# Tests
`./interpreter casm_file -t test_file`  

See test/ for example tests

## Test File Format
FILE = RULE "\n" *  
RULE = LOCATION " " EXPECTED_VALUE  
LOCATION = ADDRESS | "AFTER_HALTFLAG"  
ADDRESS = MEMTYPE 0..limit  
MEMTYPE = "R" | "M" | "S"  
EXPECTED_VALUE = \d+
  
## Test Output File Format  
RESULT = COMPILE_STATUS "\0" RULE_STATUSES  
COMPILE_STATUS = Failure reason | ""  
RULE_STATUSES = RULE_STATUS "\0"  
RULE_STATUS = "" | "Expected " NUMBER "but found " NUMBER  

# Instruction Set Overview
## Syntax Definitions
\<label\>: One or more letters postfixed with a colon (e.g., LOOP:, END:)  
\<label_ref\>: One of more letters (e.g., START, ACCUMULATE)  
\<register\>: R\<number\> (e.g., R1, R2).  
\<- : Assignment  
address : An address must be a multiple of 4 (since each cell is 4 bytes), and can be an expression of numbers and the values of registers  
M[address]: Memory access at address (Can be used on either side of assignment)  

## Load Instructions
Direct Load
* Syntax: LOAD \<register:Ri\>, \<register:Rj\>
* Description: Loads data from the memory address stored in register Rj into register Ri.
* Formal Definition: Ri \<- M[Rj]

Immediate Load
* Syntax: LOAD \<register\>, =\<number\>
* Description: Loads \<number\> directly into \<register\>.
* Formal Definition: register \<- number

Relative Load
* Syntax: LOAD \<register:Ri\>, $\<register:Rj\>
* Description: Loads data from the memory address located at pc+Rj into Ri.
* Formal Definition: Ri \<- M[pc+Rj]

Index Load
* Syntax: LOAD \<register:Ri\>, [\<number\>, \<register:Rj\>]
* Description: Loads data from the memory address located at number+Rj into Ri.
* Formal Definition: Ri \<- M[number+Rj]

Indirect Load
* Syntax: LOAD \<register:Ri\>, @\<register:Rj\>
* Description: Read the formal definition ;-;
* Formal Definition: Ri \<- M[M[Rj]]

## Store Instructions
Direct Store		
* Syntax: STORE \<register:Ri\>, \<register:Rj\>
* Formal Definition: M[Rj] = Ri

Index Store
* Syntax: STORE \<register:Ri\>, [\<number\>, \<register:Rj\>]
* Formal Definition: M[number+Rj] = Ri

Relative Store
* Syntax: STORE \<register:Ri\>, $\<register:Rj\>
* Formal Definition: M[Rj+pc] = ri

## Read Instructions
Direct Read
* Syntax: READ \<register:Ri\>, \<register:Rj\>
* Formal Definition: Ri = S[Rj]

Index Read
* Syntax: READ \<register:Ri\>, [\<number\>, \<register:Rj\>]
* Formal Definition: Ri = S[number+Rj]

## Write Instructions
Direct Write
* Syntax: WRITE \<register:Ri\>, \<register:Rj\>
* Formal Definition: S[Rj] = Ri

Index Write
* Syntax: WRITE \<register:Ri\>, [\<number\>, \<register:Rj\>]
* Formal Definition: S[number+Rj] = Ri

## Math Instructions
Note: This language only supports positive integers.

Addition
* Syntax: ADD \<register:Ri\>, \<register:Rj\>
* Description: Stores Ri+Rj in Ri.

Subtraction
* Syntax: SUB \<register:Ri\>, \<register:Rj\>
* Description: Stores Max(Ri - Rj, 0) in Ri.

Multiplication
* Syntax: MUL \<register:Ri\>, \<register:Rj\>
* Description: Stores Ri * Rj in Ri.

Division
* Syntax: DIV \<register:Ri\>, \<register:Rj\>
* Description: Performs Integer Division and Modulus. 
	1. Stores Ri // Rj in Ri.
	2. Stores Max(Ri % Rj), 0) in Rj.

## Branch Instructions
Unconditional Branch
* Syntax: BR \<label_ref\>
* Description: Jumps to the specified label unconditionally.

Branch if Less Than
* Syntax: BLT \<register:Ri\>, \<register:Rj\>, \<label_ref\>
* Description: Jumps to \<label\> if Ri \< Rj

Branch if Greater Than
* Syntax: BGT \<register:Ri\>, \<register:Rj\>, \<label_ref\>
* Description: Jumps to \<label\> if Ri \> Rj

Branch if Less Than or Equal
* Syntax: BLEQ \<register:Ri\>, \<register:Rj\>, \<label_ref\>
* Description: Jumps to \<label\> if Ri \<= Rj

Branch if Greater Than or Equal
* Syntax: BGEQ \<register:Ri\>, \<register:Rj\>, \<label_ref\>
* Description: Jumps to \<label\> if Ri \>= Rj

Branch if Equal
* Syntax: BEQ \<register:Ri\>, \<register:Rj\>, \<label_ref\>
* Description: Jumps to \<label\> if Ri == Rj

Branch if Not Equal
* Syntax: BNEQ \<register:Ri\>, \<register:Rj\>, \<label_ref\>
* Description: Jumps to \<label\> if Ri != Rj
