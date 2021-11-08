#define _CRT_SECURE_NO_WARNINGS
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <math.h>

#define OPCODELEN 32
#define REGSNUM 8
#define SRAMSIZE 65536
#define MAXPC 65535 //maximum number of instructions 0xFFFF
#define LINELEN 10

static int PC;
static int instructions_count = 0;
static int num_instructions;
static int sram[SRAMSIZE] = {0};
static int regs[REGSNUM] = {0};
static char instructions_hex[MAXPC][9] = {NULL};
const char opcodes[25][4] = {"ADD", "SUB", "LSF", "RSF", "AND", "OR", "XOR", "LHI", "LD", "ST", "", "", "", "", "", "", "JLT", "JLE", "JEQ", "JNE", "JIN", "", "", "", "HLT"};

void hex_binary(char* hex, char * res){ //converts an hexa string to binary string
    char binary[16][5] = {"0000", "0001", "0010", "0011", "0100", "0101","0110", "0111", "1000", "1001", "1010", "1011", "1100", "1101", "1110","1111"};
    char digits [] = "0123456789abcdef";

    res[0] = '\0';
    int p = 0;
    int value =0;
    while(hex[p])
    {
        const char *v = strchr(digits, tolower(hex[p]));
        if(v[0]>96){
            value=v[0]-87;
        }
        else{
            value=v[0]-48;
        }
        if (v){
            strcat(res, binary[value]);
        }
        p++;
    }
    return;
}

int update_instructions(char* file_name){ //updates the instructions as binary in array and returns number of instructions
    int i=0;
    char line[20];
    FILE *input = fopen(file_name, "r");
    if (input == NULL) {
        fprintf(stderr, "Can't open input file \n");
        exit(1);
    }
    while (fgets(line, LINELEN, input) != NULL) {
        char* hex = line;
        int inst;
        inst = strtoul(hex, NULL, 16);
        sram[i] = inst;
        hex[8] = '\0';
        strcpy(instructions_hex[i], hex);
        instructions_hex[i][8] = '\0';
        i++;
    }
    printf("out of loop");
    fclose(input);
    printf("closed file");
    return i;
}

void print_sram(FILE *trace){
    FILE *mem_out = fopen("sram_out.txt", "w+");
    if (mem_out == NULL) {
        fprintf(stderr, "Can't open sram file \n");
        exit(1);
    }
    int i=0;
    for(i = 0; i < SRAMSIZE; i++){
        fprintf(mem_out, "%08x\n", sram[i]);
    }
    fprintf(trace, "sim finished at pc %d, %d instructions", PC, instructions_count+1);
    fclose(trace);
    fclose(mem_out);
}

bool execute_instruction(int immediate, int src1, int src0, int dst, int opcode, FILE *trace){//handles op and returns true iff a jump was taken
    if(opcode == 0){//ADD
        regs[dst] = regs[src0] + regs[src1];
        return false;
    }
    else if(opcode == 1){//SUB
        regs[dst] = regs[src0] - regs[src1];
        return false;
    }
    else if(opcode == 2){//LSF
        regs[dst] = regs[src0] << regs[src1];
        return false;
    }
    else if(opcode == 3){//RSF
        regs[dst] = regs[src0] >> regs[src1];
        return false;
    }
    else if(opcode == 4){//AND
        regs[dst] = regs[src0] & regs[src1];
        return false;
    }
    else if(opcode == 5){//OR
        regs[dst] = regs[src0] | regs[src1];
        return false;
    }
    else if(opcode == 6){//XOR
        regs[dst] = regs[src0] ^ regs[src1];
        return false;
    }
    else if(opcode == 7){//LHI
        regs[dst] = regs[dst] & 0x0000ffff;
        int immediate_high = immediate << 0x10;
        regs[dst] = regs[dst] | immediate_high;
        return false;
    }
    else if(opcode == 8){//LD
        regs[dst] = sram[regs[src1]];
        return false;
    }
    else if(opcode == 9){//ST
        sram[regs[src1]] = regs[src0];
        return false;
    }
    else if(opcode == 16){//JLT
        if(regs[src0] < regs[src1])
        {
            regs[7] = PC;
            PC = immediate;
            return true;
        }
        return false;
    }
    else if(opcode == 17){//JLE
        if(regs[src0] <= regs[src1])
        {
            regs[7] = PC;
            PC = immediate;
            return true;
        }
        return false;
    }
    else if(opcode == 18){//JEQ
        if(regs[src0] == regs[src1])
        {
            regs[7] = PC;
            PC = immediate;
            return true;
        }
        return false;
    }
    else if(opcode == 19){//JNE
        if(regs[src0] != regs[src1])
        {
            regs[7] = PC;
            PC = immediate;
            return true;
        }
        return false;
    }
    else if(opcode == 20){//JIN
        regs[7] = PC;
        PC = immediate;
        return true;
    }
    else if(opcode == 24){//HLT
        print_sram(trace);
        exit(0);
    }
    else{
        fprintf(stderr, "invalid opcode \n");
        exit(1);
    }
}

void print_trace(int immediate, int src1, int src0, int dst, int opcode, FILE *trace){
    fprintf(trace, "--- instruction %d (%04x) @ PC %d (%04x) -----------------------------------------------------------\n",
            instructions_count, instructions_count, PC, PC);
    fprintf(trace, "pc = %04d, inst = %s, opcode = %d (%s), ", PC, instructions_hex[PC], opcode, opcodes[opcode]);
    fprintf(trace, "dst = %d, src0 = %d, src1 = %d, immediate = %08x\n", dst, src0, src1, immediate);
    fprintf(trace, "r[0] = %08x r[1] = %08x r[2] = %08x r[3] = %08x \nr[4] = %08x r[5] = %08x r[6] = %08x r[7] = %08x \n\n",
            regs[0], regs[1], regs[2], regs[3], regs[4], regs[5], regs[6], regs[7]);

    if(opcode <= 7){
        fprintf(trace, ">>>> EXEC: R[%d] = %d %s %d <<<<\n\n", dst, regs[src0], opcodes[opcode], regs[src1]);
    }
    else if(opcode == 8){
        fprintf(trace, ">>>> EXEC: R[%d] = MEM[%d] = %08x <<<<\n\n", dst, regs[src1], sram[regs[src1]]);
    }
    else if(opcode == 9){
        fprintf(trace, ">>>> EXEC: MEM[%d] = R[%d] = %08x <<<<\n\n", regs[src1], src0, regs[src0]);
    }
    else if(opcode == 24){
        fprintf(trace, ">>>> EXEC: HALT at PC %04x<<<<\n", PC);
    }
}

void print_trace_jump(int src1, int src0, int opcode, FILE *trace){
    fprintf(trace, ">>>> EXEC: %s %d, %d, %d <<<<\n\n", opcodes[opcode], regs[src0],regs[src1], PC);
}


int main(int argc, char** argv[]){
    char* input_name = argv[1];
    int opcode = 0, dst = 0, src0 = 0, src1 = 0, immediate = 0, i;
    num_instructions = update_instructions(input_name);
    printf("num of lines=%d\n", num_instructions);
    PC=0;
    FILE *trace = fopen("trace.txt", "w+");
    if (trace == NULL) {
        fprintf(stderr, "Can't open trace file \n");
        exit(1);
    }
    for (i = 0; i < 8; i++)
        sram[15+i] = i;
    fprintf(trace, "program %s loaded, %d lines\n", argv[1], num_instructions);
    while(true){
        int inst;
        inst = strtoul(instructions_hex[PC], NULL, 16); //Converting string to hex
        // determine opcode
        opcode = inst & 0x3E000000;
        opcode = opcode >> 0x19;
        //determine destination register
        dst = inst & 0x01c00000;
        dst = dst >> 0x16;
        if(dst < 0 || dst > 7){
            fprintf(stderr, "invalid reg number\n");
            exit(1);
        }
        //determine src0
        src0 = inst & 0x00380000;
        src0 = src0 >> 0x13;
        if(src0 < 0 || src0 > 7){
            fprintf(stderr, "invalid reg number\n");
            exit(1);
        }
        //determine src1
        src1 = inst & 0x00070000;
        src1 = src1 >> 0x10;
        if(src1 < 0 || src1 > 7){
            fprintf(stderr, "invalid reg number\n");
            exit(1);
        }
        //determine immediate
        immediate = inst & 0x0000ffff;
        printf("imm=%d, src1=%d src0=%d, dst=%d, opcode=%d\n",immediate, src1, src0, dst, opcode );
        regs[1] = immediate;
        print_trace(immediate, src1, src0, dst, opcode, trace);
        bool jump_taken = execute_instruction(immediate, src1, src0, dst, opcode, trace);
        if(!jump_taken){
            if(PC == MAXPC){
                PC=0;
            }
            else{
                PC++;
            }
        }
        if(opcode>=16 && opcode<=20)
            print_trace_jump( src1, src0, opcode, trace);
        instructions_count++;
    }
    exit(1);
}
