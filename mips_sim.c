// COMP1521 20T3 Assignment 1: mips_sim -- a MIPS simulator
// starting point code v0.1 - 13/10/20
// Author: Demiao Chen (Derek)
// Date: 29-Oct-2020
// zID: z5289988

/* 
    A simulator for a small simple subset of the MIPS.
    Generate the correspoding mips instructions from 32-bit
    numbers. Run with -r will only output syscall result and 
    error messages.
    Instructions can be simulated:
        add, sub, slt, mul, beq, bne, addi, ori, lui, syscall.
    syscall supports: 
        print integer, exit, print character.
    This program only accept 32-bit valid instrutions.
*/

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>

#define MAX_LINE_LENGTH 256
#define INSTRUCTIONS_GROW 64

//////////////////////////////////////////////////////////////////////////////

// MY CONSTANTS

// op instructions:
#define ADD  0
#define SUB  1
#define SLT  2
#define MUL  3
#define BEQ  4
#define BNE  5
#define ADDI 6
#define ORI  7
#define LUI  8
#define SYS  9
#define INVALID -1

#define ELEVEN_MASK 0b11111111111
#define ADD_INS     0b00000100000
#define SUB_INS     0b00000100010
#define SLT_INS     0b00000101010
#define SYS_INS     0b00000001100

#define SIX_MASK  0b111111
#define THREE_INS 0b000000
#define MUL_INS   0b011100
#define BEQ_INS   0b000100
#define BNE_INS   0b000101
#define ADDI_INS  0b001000
#define ORI_INS   0b001101
#define LUI_INS   0b001111

// shifting:
#define FIVE_MASK 0b11111
#define RS_SHIFT  21
#define RT_SHIFT  16
#define RD_SHIFT  11

#define FOUR_MASK 0xFFFF

// syscall instructions:
#define ERROR       -1
#define EXIT        10
#define PRINT_INT   1
#define PRINT_CHAR  11
#define JUMP        77

//////////////////////////////////////////////////////////////////////////////

// FUNCTION PROTOTYPES
void execute_instructions(int n_instructions,
                          uint32_t instructions[n_instructions],
                          int trace_mode);
char *process_arguments(int argc, char *argv[], int *trace_mode);
uint32_t *read_instructions(char *filename, int *n_instructions_p);
uint32_t *instructions_realloc(uint32_t *instructions, int n_instructions);


// FUNCTION PROTOTYPES: HELPER FUNCTIONS

void printInstruction(int op, int rs, int rt, int rd, int16_t immediate);
int printExplanation(int op, int rs, int rt, int rd, 
                      int16_t immediate, int *registers, int *pc,
                      int n_instructions, int trace_mode);

int opGet(uint32_t input);
int rsGet(uint32_t input);
int rtGet(uint32_t input);
int rdGet(uint32_t input);
int16_t immediateGet(uint32_t input);
int isSyscall(int op, int rs, int rt);

//////////////////////////////////////////////////////////////////////////////

int main(int argc, char *argv[]) {
    int trace_mode;
    char *filename = process_arguments(argc, argv, &trace_mode);

    int n_instructions;
    uint32_t *instructions = read_instructions(filename, &n_instructions);

    execute_instructions(n_instructions, instructions, trace_mode);

    free(instructions);
    return 0;
}


// simulate execution of  instruction codes in  instructions array
// output from syscall instruction & any error messages are printed
//
// if trace_mode != 0:
//     information is printed about each instruction as it executed
//
// execution stops if it reaches the end of the array

void execute_instructions(int n_instructions,
                          uint32_t instructions[n_instructions],
                          int trace_mode) {

    // All 32 registers are set to be zero when execution begins.
    int registers[32] = {0};
    int pc = 0;
    while (pc < n_instructions) {

        int op = opGet(instructions[pc]);
        int rs = rsGet(instructions[pc]);
        int rt = rtGet(instructions[pc]);
        int rd = rdGet(instructions[pc]);
        int16_t immediate = immediateGet(instructions[pc]);

        if (trace_mode) printf("%d: 0x%08X ", pc, instructions[pc]);
        if (op == INVALID) { // invalid instruction halts program
            printf("invalid instruction code\n");
            return;
        }
        if (trace_mode) printInstruction(op, rs, rt, rd, immediate);
        int signal = printExplanation(op, rs, rt, rd, 
                                        immediate, registers, 
                                        &pc, n_instructions,
                                        trace_mode);
        // Halting when there is an error or exit instruction 
        if (signal == ERROR || signal == EXIT) return;        

        pc++;
    }
}

//////////////////// Helper Functions for execute_instructions ///////////////

// This function will translate a given instruction to corresponding 
// meaning, and print them out.
void printInstruction(int op, int rs, int rt, int rd, int16_t immediate) {

    if (op == SYS) {
        printf("syscall\n");
        return;
    }

    if (op == ADD) printf("add  ");
    if (op == SUB) printf("sub  ");
    if (op == SLT) printf("slt  ");
    if (op == MUL) printf("mul  ");
    if (op == BEQ) printf("beq  ");
    if (op == BNE) printf("bne  ");
    if (op == ADDI) printf("addi ");
    if (op == ORI) printf("ori  ");
    if (op == LUI) printf("lui  ");

    if (op == ADD || op == SUB || op == SLT || op == MUL) {
        printf("$%d, $%d, $%d\n", rd, rs, rt);
    }
    if (op == BEQ || op == BNE) {
        printf("$%d, $%d, %d\n", rs, rt, immediate);
    }
    if (op == ADDI || op == ORI) {
        printf("$%d, $%d, %d\n", rt, rs, immediate);
    }
    if (op == LUI) {
        printf("$%d, %d\n", rt, immediate);
    }
}

// This function will execute given instruction, and print out 
// the result.
int printExplanation(int op, int rs, int rt, int rd, 
                      int16_t immediate, int *registers, int *pc,
                      int n_instructions, int trace_mode) {
    if (op == SYS) {
        if (trace_mode) printf(">>> syscall %d\n", registers[2]);
        if (registers[2] == PRINT_INT) {
            if (trace_mode) printf("<<< ");
            if (trace_mode) printf("%d\n", registers[4]); // $4 = $a0
            if (!trace_mode) printf("%d", registers[4]); 
            return PRINT_INT;
        } else if (registers[2] == EXIT) {
            return EXIT;
        } else if (registers[2] == PRINT_CHAR) {
            if (trace_mode) printf("<<< ");
            if (trace_mode) printf("%c\n", registers[4]); 
            if (!trace_mode) printf("%c", registers[4]); 
            return PRINT_CHAR;
        } else {
            printf("Unknown system call: %d\n", registers[2]);
            return ERROR;
        }
        return 0;
    }

    if (op == ADD) {
        if (rd != 0) { // register 0 cannot be changed
            registers[rd] = registers[rs] + registers[rt];
            if (trace_mode) printf(">>> $%d = %d\n", rd, registers[rd]);
        } else {
            if (trace_mode) printf(">>> $%d = %d\n", rd, 
            (registers[rs] + registers[rt]));
        }
    }
    if (op == SUB) {
        if (rd != 0) {
            registers[rd] = registers[rs] - registers[rt];
            if (trace_mode) printf(">>> $%d = %d\n", rd, registers[rd]);
        } else {
            if (trace_mode) printf(">>> $%d = %d\n", rd, 
            (registers[rs] - registers[rt]));
        }
    }
    if (op == SLT) {
        if (rd != 0) {
            registers[rd] = (registers[rs] < registers[rt]);
            if (trace_mode) printf(">>> $%d = %d\n", rd, registers[rd]);
        } else {
            if (trace_mode) printf(">>> $%d = %d\n", rd, 
            (registers[rs] < registers[rt]));
        }
    }
    if (op == MUL) {
        if (rd != 0) {
            registers[rd] = registers[rs] * registers[rt];
            if (trace_mode) printf(">>> $%d = %d\n", rd, registers[rd]);
        } else {
            if (trace_mode) printf(">>> $%d = %d\n", rd, 
            (registers[rs] * registers[rt]));
        }
    }
    if (op == BEQ) {
        if (registers[rs] == registers[rt]) {
            *pc += immediate;
            if (trace_mode) printf(">>> branch taken to PC = %d\n", *pc);
            if (*pc < 0 || *pc >= n_instructions ) {
                printf(
                    "Illegal branch to address before instructions: PC = %d\n",
                    *pc);
                return ERROR;       
            }
            *pc -= 1;
        } else {
            if (trace_mode) printf(">>> branch not taken\n");
        }             
    }
    if (op == BNE) {
        if (registers[rs] != registers[rt]) {
            *pc += immediate;
            if (trace_mode) printf(">>> branch taken to PC = %d\n", *pc);
            if (*pc < 0 || *pc >= n_instructions ) {
                printf(
                    "Illegal branch to address before instructions: PC = %d\n",
                    *pc);
                return ERROR;       
            }
            *pc -= 1;
        } else {
            if (trace_mode) printf(">>> branch not taken\n");
        }  
    }

    if (op == ADDI) {
        if (rt != 0) {
            registers[rt] = registers[rs] + immediate;
            if (trace_mode) printf(">>> $%d = %d\n", rt, registers[rt]);
        }
    }
    if (op == ORI) {
        if (rt != 0) {
            registers[rt] = registers[rs] | immediate;
            if (trace_mode) printf(">>> $%d = %d\n", rt, registers[rt]);
        } else {
            if (trace_mode) printf(">>> $%d = %d\n", rt, 
            (registers[rs] | immediate));
        }
    }
    if (op == LUI) {
        if (rt != 0) {
            registers[rt] = (immediate << 16);
            if (trace_mode) printf(">>> $%d = %d\n", rt, registers[rt]);
        } else {
            if (trace_mode) printf(">>> $%d = %d\n", rt, (immediate << 16));
        }
    }
    return 0;
}


// The following 5 functioncs extract op, rs ,rt, rd and 
// immediate of an 32-bit instruction, and return them as an integer.
int opGet(uint32_t input) {
    uint32_t mask = SIX_MASK;
    int type = mask & (input >> 26);

    if (type == THREE_INS) {
        mask = ELEVEN_MASK;
        type = mask & input;

        if (type == ADD_INS) return ADD; // add
        if (type == SUB_INS) return SUB; // sub
        if (type == SLT_INS) return SLT; // slt
        if (type == SYS_INS) return SYS; // syscall

    }    
    if (type == MUL_INS) return MUL;  // mul
    if (type == BEQ_INS) return BEQ;  // beq
    if (type == BNE_INS) return BNE;  // bne
    if (type == ADDI_INS) return ADDI; // addi
    if (type == ORI_INS) return ORI;  // ori
    if (type == LUI_INS) return LUI;  // lui

    return INVALID;
}
int rsGet(uint32_t input) {
    uint32_t mask = FIVE_MASK;
    return mask & (input >> RS_SHIFT);
}
int rtGet(uint32_t input) {
    uint32_t mask = FIVE_MASK;
    return mask & (input >> RT_SHIFT);
}
int rdGet(uint32_t input) {
    uint32_t mask = FIVE_MASK;
    return mask & (input >> RD_SHIFT);
}
int16_t immediateGet(uint32_t input) {
    uint16_t mask = FOUR_MASK;
    return mask & input;
}

//////////////////////////////////////////////////////////////////////////////

// check_arguments is given command-line arguments
// it sets *trace_mode to 0 if -r is specified
//          *trace_mode is set to 1 otherwise
// the filename specified in command-line arguments is returned

char *process_arguments(int argc, char *argv[], int *trace_mode) {
    if (
        argc < 2 ||
        argc > 3 ||
        (argc == 2 && strcmp(argv[1], "-r") == 0) ||
        (argc == 3 && strcmp(argv[1], "-r") != 0)) {
        fprintf(stderr, "Usage: %s [-r] <file>\n", argv[0]);
        exit(1);
    }
    *trace_mode = (argc == 2);
    return argv[argc - 1];
}


// read hexadecimal numbers from filename one per line
// numbers are return in a malloc'ed array
// *n_instructions is set to size of the array

uint32_t *read_instructions(char *filename, int *n_instructions_p) {
    FILE *f = fopen(filename, "r");
    if (f == NULL) {
        fprintf(stderr, "%s: '%s'\n", strerror(errno), filename);
        exit(1);
    }

    uint32_t *instructions = NULL;
    int n_instructions = 0;
    char line[MAX_LINE_LENGTH + 1];
    while (fgets(line, sizeof line, f) != NULL) {

        // grow instructions array in steps of INSTRUCTIONS_GROW elements
        if (n_instructions % INSTRUCTIONS_GROW == 0) {
            instructions = instructions_realloc(instructions, n_instructions + INSTRUCTIONS_GROW);
        }

        char *endptr;
        instructions[n_instructions] = strtol(line, &endptr, 16);
        if (*endptr != '\n' && *endptr != '\r' && *endptr != '\0') {
            fprintf(stderr, "%s:line %d: invalid hexadecimal number: %s",
                    filename, n_instructions + 1, line);
            exit(1);
        }
        n_instructions++;
    }
    fclose(f);
    *n_instructions_p = n_instructions;
    // shrink instructions array to correct size
    instructions = instructions_realloc(instructions, n_instructions);
    return instructions;
}


// instructions_realloc is wrapper for realloc
// it calls realloc to grow/shrink the instructions array
// to the speicfied size
// it exits if realloc fails
// otherwise it returns the new instructions array
uint32_t *instructions_realloc(uint32_t *instructions, int n_instructions) {
    instructions = realloc(instructions, n_instructions * sizeof *instructions);
    if (instructions == NULL) {
        fprintf(stderr, "out of memory");
        exit(1);
    }
    return instructions;
}