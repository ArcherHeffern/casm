# Brandeis Operating Systems MiniASM Interpreter and Visualizer
__[Demo(TODO)](https://youtube.com)__
## Visualizer
<img width="912" alt="Screenshot 2024-11-05 at 9 44 58 PM" src="https://github.com/user-attachments/assets/9af2739d-0d0e-4927-95c5-ef48c93d5f55">

## Interpreter

![interpreter_big_loop](https://github.com/user-attachments/assets/a58860d3-428e-4d80-9dab-84235162515c)

## Features
- Easily upload assembly files using drag and drop
- Aids in visualization via single step and continue. Or jump to the end if thats not for you.
- Get fast feedback to program changes. Casm detects changes to your assembly source file and automatically reloads file
- Find syntax errors quickly with descriptive error messages
- Pleasing to the eyes with smooth animations
- Easy to use. Power and simplicity don't have to be mutually exclusive

## Getting Started
1. Install precompiled binaries from releases  
Run on the command line `./casm [assembly_program]`

2. Build from source
``` bash
	git clone https://github.com/ArcherHeffern/casm.git
	make [macos|windows|linux]
	./main [assembly_program]
```
NOTE: I have not yet tested the windows and linux targets. Let me know if you run into any issues running these

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
