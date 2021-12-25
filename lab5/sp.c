#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h> 
//#include <sys/socket.h>
//#include <netinet/in.h>

#include "llsim.h"

#define sp_printf(a...)						\
	do {							\
		llsim_printf("sp: clock %d: ", llsim->clock);	\
		llsim_printf(a);				\
	} while (0)

int nr_simulated_instructions = 0;
FILE *inst_trace_fp = NULL, *cycle_trace_fp = NULL;


// DMA states
#define DMA_IDLE	0
#define DMA_READ	1
#define DMA_WRITE	2


int instructions = 0;
int branch_predictor = 1; //2 bit branch predictor: 00 and 01 means dont take jump, 10 and 11 means take jump
//we chose to init it to 1

int mem_available = 0; // if mem is not in LD\SD state (available) = 1, else (not available) = 0	
int start_dma = 0; // if need to start the DMA = 1 else = 0	

typedef struct sp_registers_s {
	// 6 32 bit registers (r[0], r[1] don't exist)
	int r[8];

	// 32 bit cycle counter
	int cycle_counter;

	// fetch0
	int fetch0_active; // 1 bit
	int fetch0_pc; // 16 bits

	// fetch1
	int fetch1_active; // 1 bit
	int fetch1_pc; // 16 bits

	// dec0
	int dec0_active; // 1 bit
	int dec0_pc; // 16 bits
	int dec0_inst; // 32 bits

	// dec1
	int dec1_active; // 1 bit
	int dec1_pc; // 16 bits
	int dec1_inst; // 32 bits
	int dec1_opcode; // 5 bits
	int dec1_src0; // 3 bits
	int dec1_src1; // 3 bits
	int dec1_dst; // 3 bits
	int dec1_immediate; // 32 bits

	// exec0
	int exec0_active; // 1 bit
	int exec0_pc; // 16 bits
	int exec0_inst; // 32 bits
	int exec0_opcode; // 5 bits
	int exec0_src0; // 3 bits
	int exec0_src1; // 3 bits
	int exec0_dst; // 3 bits
	int exec0_immediate; // 32 bits
	int exec0_alu0; // 32 bits
	int exec0_alu1; // 32 bits

	// exec1
	int exec1_active; // 1 bit
	int exec1_pc; // 16 bits
	int exec1_inst; // 32 bits
	int exec1_opcode; // 5 bits
	int exec1_src0; // 3 bits
	int exec1_src1; // 3 bits
	int exec1_dst; // 3 bits
	int exec1_immediate; // 32 bits
	int exec1_alu0; // 32 bits
	int exec1_alu1; // 32 bits
	int exec1_aluout;

	// DMA
	int dma_state;	// 3 bit DMA state machine state register
	int dma_busy;	// 1 bit if dma busy = 1 
	int dma_src;	// 32 bit DMA source address
	int dma_dst;	// 32 bit DMA destination address
	int dma_len;	// length of the copied data
} sp_registers_t;

/*
 * Master structure
 */
typedef struct sp_s {
	// local srams
#define SP_SRAM_HEIGHT	64 * 1024
	llsim_memory_t *srami, *sramd;

	unsigned int memory_image[SP_SRAM_HEIGHT];
	int memory_image_size;

	int start;

	sp_registers_t *spro, *sprn;
} sp_t;

static void sp_reset(sp_t *sp)
{
	sp_registers_t *sprn = sp->sprn;

	memset(sprn, 0, sizeof(*sprn));
}

/*
 * opcodes
 */
#define ADD 0
#define SUB 1
#define LSF 2
#define RSF 3
#define AND 4
#define OR  5
#define XOR 6
#define LHI 7
#define LD 8
#define ST 9
#define CPY 10  //opcode for copying
#define DMS 11  // opcode for getting the dma status -dms
#define JLT 16
#define JLE 17
#define JEQ 18
#define JNE 19
#define JIN 20
#define NOP 23
#define HLT 24

static char opcode_name[32][4] = {"ADD", "SUB", "LSF", "RSF", "AND", "OR", "XOR", "LHI",
				 "LD", "ST", "CPY", "DMS", "U", "U", "U", "U",
				 "JLT", "JLE", "JEQ", "JNE", "JIN", "U", "U", "NOP",
				 "HLT", "U", "U", "U", "U", "U", "U", "U"};

static void dump_sram(sp_t *sp, char *name, llsim_memory_t *sram)
{
	FILE *fp;
	int i;

	fp = fopen(name, "w");
	if (fp == NULL) {
                printf("couldn't open file %s\n", name);
                exit(1);
	}
	for (i = 0; i < SP_SRAM_HEIGHT; i++)
		fprintf(fp, "%08x\n", llsim_mem_extract(sram, i, 31, 0));
	fclose(fp);
}

int alu_op(int op) { // return true in case we have ALU or DMA operations
	if (op == ADD || op == SUB || op == AND || op == OR || op == XOR || op == LSF || op == RSF || op == LHI || op == CPY || op == DMS) return 1;

	return 0;
}

int brach_op(int op) {// return true in case we have branch op
	if (op == JLT || op == JNE || op == JLE || op == JEQ)
		return 1;
	return 0;
}
void stall_dec1(sp_t* sp) {
	sp_registers_t* spro = sp->spro;
	sp_registers_t* sprn = sp->sprn;
	sprn->exec1_active = 0;
	sprn->exec0_pc = 0;
	sprn->exec0_inst = 0;
	sprn->exec0_opcode = NOP;
	sprn->exec0_dst = 0;
	sprn->exec0_src0 = 0;
	sprn->exec0_src1 = 0;
	sprn->exec0_immediate = 0;
	sprn->exec0_active = 1;
	//stalling fetch0, fetch1
	sprn->fetch0_active = spro->fetch0_active;
	sprn->fetch0_pc = spro->fetch0_pc;
	sprn->fetch1_active = spro->fetch1_active;
	sprn->fetch1_pc = spro->fetch1_pc;
	//stalling dec0, dec1
	sprn->dec0_active = spro->dec0_active;
	sprn->dec0_pc = spro->dec0_pc;
	sprn->dec0_inst = spro->dec0_inst;
	sprn->dec1_active = spro->dec1_active;
	sprn->dec1_pc = spro->dec1_pc;
	sprn->dec1_inst = spro->dec1_inst;
	sprn->dec1_opcode = spro->dec1_opcode;
	sprn->dec1_src0 = spro->dec1_src0;
	sprn->dec1_src1 = spro->dec1_src1;
	sprn->dec1_dst = spro->dec1_dst;
	sprn->dec1_immediate = spro->dec1_immediate;
}

int exec1_data_hazard(sp_t* sp, int src) {
	sp_registers_t* spro = sp->spro;
	if (spro->exec1_opcode == LD && spro->exec1_active && spro->exec1_dst == src)
	{
		return 1;
	}
	return 0;
}



int exec1_control_hazard(sp_t* sp, int src) {
	sp_registers_t* spro = sp->spro;
	if (spro->exec1_active && src == 7 && ((brach_op(spro->exec1_opcode) && spro->exec1_aluout == 1) || (spro->exec1_opcode == JIN)))
	{
		return 1;
	}
	return 0;
}

int exec1_reg_data_hazard(sp_t* sp, int src) {
	sp_registers_t* spro = sp->spro;
	if (spro->exec1_active && alu_op(spro->exec1_opcode) && spro->exec1_dst == src) {
		return 1;
	}
	return 0;

}


void prepare_alu(sp_t* sp) {

	sp_registers_t* spro = sp->spro;
	sp_registers_t* sprn = sp->sprn;

	//check possible hazards and immediate on src0

	if (spro->dec1_src0 == 1) {
		sprn->exec0_alu0 = spro->dec1_immediate;
		sprn->r[1] = spro->dec1_immediate;
	}

	else if (exec1_control_hazard(sp, spro->dec1_src0)) {
		sprn->exec0_alu0 = spro->exec1_pc;
	}

	else if (exec1_data_hazard(sp, spro->dec1_src0))
	{
		sprn->exec0_alu0 = llsim_mem_extract_dataout(sp->sramd, 31, 0);
	}
	else if (exec1_reg_data_hazard(sp, spro->dec1_src0))
	{
		sprn->exec0_alu0 = spro->exec1_aluout;
	}

	else if (spro->dec1_src0 == 0)
	{
		sprn->exec0_alu0 = 0;
	}
	else
	{
		sprn->exec0_alu0 = spro->r[spro->dec1_src0];
	}

	//check possible hazards and immediate on src1
	if (spro->dec1_src1 == 1) {
		sprn->exec0_alu1 = spro->dec1_immediate;
		sprn->r[1] = spro->dec1_immediate;
	}
	else if (exec1_reg_data_hazard(sp, spro->dec1_src1)) {
		sprn->exec0_alu1 = spro->exec1_aluout;
	}
	else if (exec1_control_hazard(sp, spro->dec1_src1)) {
		sprn->exec0_alu1 = spro->exec1_pc;
	}
	else if (spro->dec1_src1 == 0) {
		sprn->exec0_alu1 = 0;
	}

	else if (exec1_data_hazard(sp, spro->dec1_src1)) {
		sprn->exec0_alu1 = llsim_mem_extract_dataout(sp->sramd, 31, 0);
	}
	else {
		sprn->exec0_alu1 = spro->r[spro->dec1_src1];
	}


}

static void clone_exec1(sp_t* sp) {
	sp_registers_t* spro = sp->spro;
	sp_registers_t* sprn = sp->sprn;

	sprn->exec1_pc = spro->exec1_pc;
	sprn->exec1_inst = spro->exec1_inst;
	sprn->exec1_opcode = spro->exec1_opcode;
	sprn->exec1_alu0 = spro->exec1_alu0;
	sprn->exec1_alu1 = spro->exec1_alu1;
	sprn->exec1_dst = spro->exec1_dst;
	sprn->exec1_src0 = spro->exec1_src0;
	sprn->exec1_src1 = spro->exec1_src1;
	sprn->exec1_immediate = spro->exec1_immediate;
}

void alu_execution(sp_t* sp, int alu0, int alu1)
{
	sp_registers_t* spro = sp->spro;
	sp_registers_t* sprn = sp->sprn;
	int op = spro->exec0_opcode;
	if (op == ADD) {
		sprn->exec1_aluout = alu0 + alu1;
	}
	else if (op == SUB) {
		sprn->exec1_aluout = alu0 - alu1;
	}
	else if (op == LSF) {
		sprn->exec1_aluout = alu0 << alu1;
	}
	else if (op == RSF) {
		sprn->exec1_aluout = alu0 >> alu1;
	}
	else if (op == AND) {
		sprn->exec1_aluout = alu0 & alu1;
	}
	else if (op == OR) {
		sprn->exec1_aluout = alu0 | alu1;
	}
	else if (op == XOR) {
		sprn->exec1_aluout = alu0 ^ alu1;
	}
	else if (op == LHI) {
		sprn->exec1_aluout = (alu1 << 16) | (alu0 & 0xFFFF);
	}
	else if (op == JLT) {
		sprn->exec1_aluout = (alu0 >= alu1) ? 0 : 1;
	}
	else if (op == JLE) {
		sprn->exec1_aluout = (alu0 > alu1) ? 0 : 1;
	}
	else if (op == JEQ) {
		sprn->exec1_aluout = (alu0 != alu1) ? 0 : 1;
	}
	else if (op == JNE) {
		sprn->exec1_aluout = (alu0 == alu1) ? 0 : 1;
	}
}

void move_dec1(sp_t* sp) {
	sp_registers_t* spro = sp->spro;
	sp_registers_t* sprn = sp->sprn;

	sprn->dec1_inst = spro->dec0_inst;
	sprn->dec1_pc = spro->dec0_pc;
	sprn->dec1_active = 1;
	//parse instruction into registers
	sprn->dec1_dst = (spro->dec0_inst >> 22) & 0x7;
	sprn->dec1_src0 = (spro->dec0_inst >> 19) & 0x7;
	sprn->dec1_src1 = (spro->dec0_inst >> 16) & 0x7;
	sprn->dec1_immediate = spro->dec0_inst & 0xFFFF;
	sprn->dec1_immediate <<= 20;
	sprn->dec1_immediate >>= 20;
}

void move_exec0(sp_t* sp) {
	sp_registers_t* spro = sp->spro;
	sp_registers_t* sprn = sp->sprn;

	sprn->exec0_pc = spro->dec1_pc;
	sprn->exec0_inst = spro->dec1_inst;
	sprn->exec0_opcode = spro->dec1_opcode;
	sprn->exec0_dst = spro->dec1_dst;
	sprn->exec0_src0 = spro->dec1_src0;
	sprn->exec0_src1 = spro->dec1_src1;
	sprn->exec0_immediate = spro->dec1_immediate;
}

void move_exec1(sp_t* sp, int alu0, int alu1) {
	sp_registers_t* spro = sp->spro;
	sp_registers_t* sprn = sp->sprn;
	sprn->exec1_inst = spro->exec0_inst;
	sprn->exec1_pc = spro->exec0_pc;
	sprn->exec1_opcode = spro->exec0_opcode;
	sprn->exec1_alu0 = alu0;
	sprn->exec1_alu1 = alu1;
	sprn->exec1_dst = spro->exec0_dst;
	sprn->exec1_src0 = spro->exec0_src0;
	sprn->exec1_src1 = spro->exec0_src1;
	sprn->exec1_immediate = spro->exec0_immediate;
}

void handle_flush_pipe(sp_t* sp, int new_pc) {
	sp_registers_t* sprn = sp->sprn;

	sprn->exec0_active = 0;
	sprn->exec1_active = 0;
	sprn->dec0_active = 0;
	sprn->dec1_active = 0;
	sprn->fetch0_active = 1; // loading correct instruction
	sprn->fetch1_active = 0;
	sprn->fetch0_pc = new_pc;
}

void handle_branch(sp_t* sp) {
	sp_registers_t* spro = sp->spro;
	sp_registers_t* sprn = sp->sprn;
	int flush_pipe = 0;
	int new_pc;


	if (brach_op(spro->exec1_opcode)) // if it's a conditional jump inst
	{
		if (spro->exec1_aluout == 1) //condition of jump is met
		{
			sprn->r[7] = spro->exec1_pc;//store old pc
			new_pc = spro->exec1_immediate & 0xFFFF;
			if (branch_predictor < 3)
				branch_predictor++; //jump taken
		}
		else
		{
			new_pc = spro->exec1_pc + 1;
			if (branch_predictor > 0)
				branch_predictor--; //jump not taken
		}
	}
	else
	{
		sprn->r[7] = spro->exec1_pc;//store old pc
		new_pc = spro->exec1_alu0 & 0xFFFF;
	}

	// handle flushes

	if (spro->exec0_active == 1) {
		if (spro->exec0_pc != new_pc) {
			flush_pipe = 1;
		}
	}
	else if (spro->dec1_active == 1) {
		if (spro->dec1_pc != new_pc) {
			flush_pipe = 1;
		}
	}
	else if (spro->dec0_active == 1) {
		if (spro->dec0_pc != new_pc) {
			flush_pipe = 1;
		}
	}
	else if (spro->fetch1_active == 1) {
		if (spro->fetch1_pc != new_pc) {
			flush_pipe = 1;
		}
	}
	else if (spro->fetch0_active == 1) {
		if (spro->fetch0_pc != new_pc) {
			flush_pipe = 1;
		}
	}
	if (flush_pipe == 1)
	{
		handle_flush_pipe(sp, new_pc);
	}

}


void trace(sp_t* sp)
{
	fprintf(inst_trace_fp, "\n");
	fprintf(inst_trace_fp, "--- instruction %d (%04x) @ PC %d (%04x) -----------------------------------------------------------\n",
		instructions, instructions, sp->spro->exec1_pc, sp->spro->exec1_pc);
	fprintf(inst_trace_fp, "pc = %04d, inst = %08x, opcode = %d (%s), dst = %d, src0 = %d, src1 = %d, immediate = %08x\n",
		sp->spro->exec1_pc, sp->spro->exec1_inst, sp->spro->exec1_opcode, opcode_name[sp->spro->exec1_opcode],
		sp->spro->exec1_dst, sp->spro->exec1_src0, sp->spro->exec1_src1, sbs(sp->spro->exec1_inst, 15, 0));
	fprintf(inst_trace_fp, "r[0] = %08x r[1] = %08x r[2] = %08x r[3] = %08x \n",
		0, sp->spro->exec1_immediate, sp->spro->r[2], sp->spro->r[3]);
	fprintf(inst_trace_fp, "r[4] = %08x r[5] = %08x r[6] = %08x r[7] = %08x \n",
		sp->spro->r[4], sp->spro->r[5], sp->spro->r[6], sp->spro->r[7]);
	fprintf(inst_trace_fp, "\n");

	if(sp->spro->exec1_opcode == ADD)
		fprintf(inst_trace_fp, ">>>> EXEC: R[%d] = %d ADD %d <<<<\n", sp->spro->exec1_dst, sp->spro->exec1_alu0, sp->spro->exec1_alu1);

	else if(sp->spro->exec1_opcode == SUB)
		fprintf(inst_trace_fp, ">>>> EXEC: R[%d] = %d SUB %d <<<<\n", sp->spro->exec1_dst, sp->spro->exec1_alu0, sp->spro->exec1_alu1);

	else if (sp->spro->exec1_opcode == AND)
		fprintf(inst_trace_fp, ">>>> EXEC: R[%d] = %d AND %d <<<<\n", sp->spro->exec1_dst, sp->spro->exec1_alu0, sp->spro->exec1_alu1);

	else if (sp->spro->exec1_opcode == OR)
		fprintf(inst_trace_fp, ">>>> EXEC: R[%d] = %d OR %d <<<<\n", sp->spro->exec1_dst, sp->spro->exec1_alu0, sp->spro->exec1_alu1);

	else if (sp->spro->exec1_opcode == XOR)
		fprintf(inst_trace_fp, ">>>> EXEC: R[%d] = %d XOR %d <<<<\n", sp->spro->exec1_dst, sp->spro->exec1_alu0, sp->spro->exec1_alu1);

	else if (sp->spro->exec1_opcode == LHI)
		fprintf(inst_trace_fp, ">>>> EXEC: R[%d][31:16] = 0x%04x <<<<\n", sp->spro->exec1_dst, sp->spro->exec1_immediate & 0xFFFF);

	else if (sp->spro->exec1_opcode == LSF)
		fprintf(inst_trace_fp, ">>>> EXEC: R[%d] = %d LSF %d <<<<\n", sp->spro->exec1_dst, sp->spro->exec1_alu0, sp->spro->exec1_alu1);
		
	else if (sp->spro->exec1_opcode == RSF)
		fprintf(inst_trace_fp, ">>>> EXEC: R[%d] = %d RSF %d <<<<\n", sp->spro->exec1_dst, sp->spro->exec1_alu0, sp->spro->exec1_alu1);
		
	else if (sp->spro->exec1_opcode == LD)
		fprintf(inst_trace_fp, ">>>> EXEC: R[%d] = MEM[%d] = %08x <<<<\n", sp->spro->exec1_dst, sp->spro->exec1_alu1,
			llsim_mem_extract_dataout(sp->sramd, 31, 0));
		
	else if (sp->spro->exec1_opcode == ST)
		fprintf(inst_trace_fp, ">>>> EXEC: MEM[%d] = R[%d] = %08x <<<<\n", sp->spro->exec1_alu1, sp->spro->exec1_src0, sp->spro->exec1_alu0);
		
	else if (sp->spro->exec1_opcode == JIN)
		fprintf(inst_trace_fp, ">>>> EXEC: JIN %d <<<<\n", sp->spro->exec1_alu0 & 0x0000FFFF);
		
	else if (sp->spro->exec1_opcode == HLT)
		fprintf(inst_trace_fp, ">>>> EXEC: HALT at PC %04x<<<<\n", sp->spro->exec1_pc);
		
	else if (sp->spro->exec1_opcode == JLT)
		fprintf(inst_trace_fp, ">>>> EXEC: JLT %d, %d, %d <<<<\n", sp->spro->exec1_alu0, sp->spro->exec1_alu1, sp->spro->exec1_aluout ? sp->spro->exec1_immediate & 0x0000FFFF : sp->spro->exec1_pc + 1);
		
	else if (sp->spro->exec1_opcode == JLE)
		fprintf(inst_trace_fp, ">>>> EXEC: JLE %d, %d, %d <<<<\n", sp->spro->exec1_alu0, sp->spro->exec1_alu1, sp->spro->exec1_aluout ? sp->spro->exec1_immediate & 0x0000FFFF : sp->spro->exec1_pc + 1);
		
	else if (sp->spro->exec1_opcode == JEQ)
		fprintf(inst_trace_fp, ">>>> EXEC: JEQ %d, %d, %d <<<<\n", sp->spro->exec1_alu0, sp->spro->exec1_alu1, sp->spro->exec1_aluout ? sp->spro->exec1_immediate & 0x0000FFFF : sp->spro->exec1_pc + 1);
		
	else if (sp->spro->exec1_opcode == JNE)
		fprintf(inst_trace_fp, ">>>> EXEC: JNE %d, %d, %d <<<<\n", sp->spro->exec1_alu0, sp->spro->exec1_alu1, sp->spro->exec1_aluout ? sp->spro->exec1_immediate & 0x0000FFFF : sp->spro->exec1_pc + 1);
		
	else if (sp->spro->exec1_opcode == CPY) // DMA
		fprintf(inst_trace_fp, ">>>> EXEC: CPY from address %04x to adress %04x with length of %d words <<<\n", sp->spro->dma_src, sp->spro->dma_dst, sp->spro->dma_len);
		
	else if (sp->spro->exec1_opcode == DMS) // DMA
		fprintf(inst_trace_fp, ">>>> EXEC: CPS saved to register %d <<<<\n", sp->spro->exec1_dst);
	
}

void dma_state_machine_copy(sp_t* sp)
{
	sp_registers_t* spro = sp->spro;
	sp_registers_t* sprn = sp->sprn;
	int mem_extract = 0;

	switch (spro->dma_state)
	{
	case DMA_IDLE:

		if (mem_available && start_dma)
		{
			sprn->dma_state = DMA_READ;
			sprn->dma_busy = 1;
		}
		else // if dma already working - wait
		{
			sprn->dma_state = DMA_IDLE;
		}
		break;

	case DMA_READ:

		if (mem_available == 0) // if mem already occupide (LD/SD) - go back to waiting
		{
			sprn->dma_state = DMA_IDLE;
		}
		else
		{
			llsim_mem_read(sp->sramd, spro->dma_src);
			sprn->dma_state = DMA_WRITE;
		}
		break;

	case DMA_WRITE:

		if (mem_available == 0) // if mem already occupied (LD/SD) - go back to waiting
		{
			sprn->dma_state = DMA_IDLE;
			break;
		}
		mem_extract = llsim_mem_extract(sp->sramd, spro->dma_src, 31, 0);
		llsim_mem_set_datain(sp->sramd, mem_extract, 31, 0);
		llsim_mem_write(sp->sramd, spro->dma_dst);
		sprn->dma_src = spro->dma_src + 1; // next read
		sprn->dma_dst = spro->dma_dst + 1; // next write
		sprn->dma_len = spro->dma_len - 1; //current length to copy

		if (spro->dma_len == 1) // the last word to copy
		{
			sprn->dma_state = DMA_IDLE; // wait for next time
			sprn->dma_busy = 0;
			sprn->dma_len = 0;
			start_dma = 0;
		}
		else if (mem_available == 0) {
			sprn->dma_state = DMA_IDLE; // if wasn't last and mem not available
		}

		else if (mem_available == 1) {
			sprn->dma_state = DMA_READ; // if it wasn't the last copy checks if mem available
		}

		break;

	default:
		break;
	}
}




static void sp_ctl(sp_t *sp)
{
	sp_registers_t *spro = sp->spro;
	sp_registers_t *sprn = sp->sprn;
	int i, op, alu0, alu1, diff;

	fprintf(cycle_trace_fp, "cycle %d\n", spro->cycle_counter);
	fprintf(cycle_trace_fp, "cycle_counter %08x\n", spro->cycle_counter);
	for (i = 2; i <= 7; i++)
		fprintf(cycle_trace_fp, "r%d %08x\n", i, spro->r[i]);

	fprintf(cycle_trace_fp, "fetch0_active %08x\n", spro->fetch0_active);
	fprintf(cycle_trace_fp, "fetch0_pc %08x\n", spro->fetch0_pc);

	fprintf(cycle_trace_fp, "fetch1_active %08x\n", spro->fetch1_active);
	fprintf(cycle_trace_fp, "fetch1_pc %08x\n", spro->fetch1_pc);

	fprintf(cycle_trace_fp, "dec0_active %08x\n", spro->dec0_active);
	fprintf(cycle_trace_fp, "dec0_pc %08x\n", spro->dec0_pc);
	fprintf(cycle_trace_fp, "dec0_inst %08x\n", spro->dec0_inst); // 32 bits

	fprintf(cycle_trace_fp, "dec1_active %08x\n", spro->dec1_active);
	fprintf(cycle_trace_fp, "dec1_pc %08x\n", spro->dec1_pc); // 16 bits
	fprintf(cycle_trace_fp, "dec1_inst %08x\n", spro->dec1_inst); // 32 bits
	fprintf(cycle_trace_fp, "dec1_opcode %08x\n", spro->dec1_opcode); // 5 bits
	fprintf(cycle_trace_fp, "dec1_src0 %08x\n", spro->dec1_src0); // 3 bits
	fprintf(cycle_trace_fp, "dec1_src1 %08x\n", spro->dec1_src1); // 3 bits
	fprintf(cycle_trace_fp, "dec1_dst %08x\n", spro->dec1_dst); // 3 bits
	fprintf(cycle_trace_fp, "dec1_immediate %08x\n", spro->dec1_immediate); // 32 bits

	fprintf(cycle_trace_fp, "exec0_active %08x\n", spro->exec0_active);
	fprintf(cycle_trace_fp, "exec0_pc %08x\n", spro->exec0_pc); // 16 bits
	fprintf(cycle_trace_fp, "exec0_inst %08x\n", spro->exec0_inst); // 32 bits
	fprintf(cycle_trace_fp, "exec0_opcode %08x\n", spro->exec0_opcode); // 5 bits
	fprintf(cycle_trace_fp, "exec0_src0 %08x\n", spro->exec0_src0); // 3 bits
	fprintf(cycle_trace_fp, "exec0_src1 %08x\n", spro->exec0_src1); // 3 bits
	fprintf(cycle_trace_fp, "exec0_dst %08x\n", spro->exec0_dst); // 3 bits
	fprintf(cycle_trace_fp, "exec0_immediate %08x\n", spro->exec0_immediate); // 32 bits
	fprintf(cycle_trace_fp, "exec0_alu0 %08x\n", spro->exec0_alu0); // 32 bits
	fprintf(cycle_trace_fp, "exec0_alu1 %08x\n", spro->exec0_alu1); // 32 bits

	fprintf(cycle_trace_fp, "exec1_active %08x\n", spro->exec1_active);
	fprintf(cycle_trace_fp, "exec1_pc %08x\n", spro->exec1_pc); // 16 bits
	fprintf(cycle_trace_fp, "exec1_inst %08x\n", spro->exec1_inst); // 32 bits
	fprintf(cycle_trace_fp, "exec1_opcode %08x\n", spro->exec1_opcode); // 5 bits
	fprintf(cycle_trace_fp, "exec1_src0 %08x\n", spro->exec1_src0); // 3 bits
	fprintf(cycle_trace_fp, "exec1_src1 %08x\n", spro->exec1_src1); // 3 bits
	fprintf(cycle_trace_fp, "exec1_dst %08x\n", spro->exec1_dst); // 3 bits
	fprintf(cycle_trace_fp, "exec1_immediate %08x\n", spro->exec1_immediate); // 32 bits
	fprintf(cycle_trace_fp, "exec1_alu0 %08x\n", spro->exec1_alu0); // 32 bits
	fprintf(cycle_trace_fp, "exec1_alu1 %08x\n", spro->exec1_alu1); // 32 bits
	fprintf(cycle_trace_fp, "exec1_aluout %08x\n", spro->exec1_aluout);

	// DMA
	fprintf(cycle_trace_fp, "dma_state %08x\n", spro->dma_state);
	fprintf(cycle_trace_fp, "dma_busy %08x\n", spro->dma_busy);
	fprintf(cycle_trace_fp, "dma_len %08x\n", spro->dma_len);

	fprintf(cycle_trace_fp, "\n");

	sp_printf("cycle_counter %08x\n", spro->cycle_counter);
	sp_printf("r2 %08x, r3 %08x\n", spro->r[2], spro->r[3]);
	sp_printf("r4 %08x, r5 %08x, r6 %08x, r7 %08x\n", spro->r[4], spro->r[5], spro->r[6], spro->r[7]);
	sp_printf("fetch0_active %d, fetch1_active %d, dec0_active %d, dec1_active %d, exec0_active %d, exec1_active %d\n",
		  spro->fetch0_active, spro->fetch1_active, spro->dec0_active, spro->dec1_active, spro->exec0_active, spro->exec1_active);
	sp_printf("fetch0_pc %d, fetch1_pc %d, dec0_pc %d, dec1_pc %d, exec0_pc %d, exec1_pc %d\n",
		  spro->fetch0_pc, spro->fetch1_pc, spro->dec0_pc, spro->dec1_pc, spro->exec0_pc, spro->exec1_pc);

	sprn->cycle_counter = spro->cycle_counter + 1;

	if (sp->start)
		sprn->fetch0_active = 1;

	// fetch0
	sprn->fetch1_active = 0;
	if (spro->fetch0_active) {
		llsim_mem_read(sp->srami, spro->fetch0_pc); //fetch the instruction from instruction mem
		sprn->fetch0_pc = (spro->fetch0_pc + 1) & 0xFFFF; // use the first 16 bits as the next PC
		sprn->fetch1_pc = spro->fetch0_pc;
		sprn->fetch1_active = 1;
	}
	else {
		sprn->dec0_active = 0;//in case we don't want to fetch new instruction
	}

	// fetch1
	if (spro->fetch1_active) {
		sprn->dec0_pc = spro->fetch1_pc;
		sprn->dec0_inst = llsim_mem_extract(sp->srami, spro->fetch1_pc, 31, 0);//move the inst to next stage
		sprn->dec0_active = 1;
	}
	else {
		sprn->dec0_active = 0;
	}
	
	// dec0
	if (spro->dec0_active) {
		op = (spro->dec0_inst >> 25) & 0x1F; //mask the opcode
		if (brach_op(op)) {//check branch instructions
			if (branch_predictor == 2 || branch_predictor == 3) { //pridction that the branch is taken
				sprn->fetch1_active = 0;
				sprn->fetch0_active = 1;
				sprn->fetch0_pc = spro->dec0_inst & 0xFFFF;
				sprn->dec0_active = 0;
			}
		}
		if (spro->dec1_opcode == ST && op == LD && spro->dec1_active) { //stall
			sprn->dec0_pc = spro->dec0_pc;
			sprn->dec0_inst = spro->dec0_inst;
			sprn->dec0_active = spro->dec0_active;
			sprn->fetch1_active = 0;
			sprn->dec1_active = 0;
			sprn->fetch0_active = spro->fetch1_active;
			sprn->fetch0_pc = spro->fetch1_pc;
		}
		else {
			sprn->dec1_opcode = op;
			move_dec1(sp);
		}
	}
	else {
		sprn->dec1_active = 0;
	}

	// dec1
	if (spro->dec1_active) {
		if ((spro->dec1_src0 != 0) && (spro->dec1_src0 != 1) &&
			(sp->spro->exec0_active && sp->spro->exec0_opcode == LD && sp->spro->exec0_dst == spro->dec1_src0)) {//make sure we dont have RAW in LD op
			stall_dec1(sp);
		}
		else if ((spro->dec1_src1 != 0) && (spro->dec1_src1 != 1) &&
			(sp->spro->exec0_active && sp->spro->exec0_opcode == LD && sp->spro->exec0_dst == spro->dec1_src1)) {//make sure we dont have RAW in ALU ops
			stall_dec1(sp);
		}
		else {
			prepare_alu(sp);
			sprn->exec0_active = 1;
			move_exec0(sp);
		}
	}
	else {
		sprn->exec0_active = 0;
	}

	// exec0
	alu0 = spro->exec0_alu0;
	alu1 = spro->exec0_alu1;
	if (spro->exec0_active) {
		if (spro->exec0_opcode == NOP) {//check if we are in a stall mode
			sprn->exec1_active = 0;
			clone_exec1(sp);
		}
		else {
			if (spro->exec0_src0 != 0 && spro->exec0_src0 != 1) {
				if (spro->exec1_active && spro->exec0_src0 == 7 && ((brach_op(spro->exec1_opcode) && spro->exec1_aluout == 1) || (spro->exec1_opcode == JIN))) {
					alu0 = spro->exec1_pc;
				}
				else if (spro->exec1_active && alu_op(spro->exec1_opcode) && spro->exec1_dst == spro->exec0_src0) {
					alu0 = spro->exec1_aluout;
				}
			}
			if (spro->exec0_src1 != 0 && spro->exec0_src1 != 1) {
				if (spro->exec1_active && spro->exec0_src1 == 7 && ((brach_op(spro->exec1_opcode) && spro->exec1_aluout == 1) || (spro->exec1_opcode == JIN))) {
					alu1 = spro->exec1_pc;
				}
				else if (spro->exec1_active && alu_op(spro->exec1_opcode) && spro->exec1_dst == spro->exec0_src1) {
					alu1 = spro->exec1_aluout;
				}
			}

			alu_execution(sp, alu0, alu1);
			if (spro->exec0_opcode == LD) llsim_mem_read(sp->sramd, alu1);


			// DMA
			int opcheck = spro->exec1_opcode;
			int op_res = 0;
			if (alu_op(opcheck)) op_res = 1;

			if (spro->exec0_opcode == DMS)
			{
				sprn->exec1_aluout = ((spro->exec1_opcode == CPY && spro->exec1_active == 1) || spro->dma_busy == 1);
			}
			else if (spro->exec0_opcode == CPY && (!(spro->exec1_active == 0 || spro->exec1_opcode == CPY) || spro->dma_busy == 1))
			{
				if (op_res && spro->exec1_active && spro->exec1_dst == spro->exec0_src0)
				{
					sprn->dma_src = spro->exec1_aluout;
				}
				else {
					sprn->dma_src = spro->r[spro->exec0_src0];
				}

				if (op_res && spro->exec1_active && spro->exec1_dst == spro->exec0_src1)
				{
					sprn->dma_dst = spro->exec1_aluout;
				}
				else
				{
					sprn->dma_dst = spro->r[spro->exec0_src1];
				}
				//handle overlapping in DMA
				diff = abs(sprn->dma_dst - sprn->dma_dst);
				if (diff < spro->exec0_immediate) {
					sprn->dma_len = diff;
				}
				else {
					sprn->dma_len = spro->exec0_immediate;
				}
			} 

			move_exec1(sp, alu0, alu1);
			sprn->exec1_active = 1;
		}
	}
	else
	{
		sprn->exec1_active = 0;
	}

	// exec1
	if (spro->exec1_active) {
		sp_printf("exec1: pc %d, inst %08x, opcode %d, aluout %d\n", spro->exec1_pc, spro->exec1_inst, spro->exec1_opcode, spro->exec1_aluout);
		trace(sp);
		if (spro->exec1_opcode == HLT) {
			llsim_stop();
			dump_sram(sp, "srami_out.txt", sp->srami);
			dump_sram(sp, "sramd_out.txt", sp->sramd);
		}
		else
		{
			if (spro->exec1_opcode == LD && spro->exec1_dst != 0 && spro->exec1_dst != 1) {
				sprn->r[spro->exec1_dst] = llsim_mem_extract(sp->sramd, spro->exec1_alu1, 31, 0);

			}
			if (spro->exec1_opcode == ST) {
				llsim_mem_set_datain(sp->sramd, spro->exec1_alu0, 31, 0);
				llsim_mem_write(sp->sramd, spro->exec1_alu1);

			}

			if ((spro->exec1_opcode >= JLT && spro->exec1_opcode <= JNE) || spro->exec1_opcode == JIN) {
				handle_branch(sp);

			}
			if (spro->exec1_opcode == DMS) {
				sprn->r[spro->exec1_dst] = spro->exec1_aluout;
			}


			if (alu_op(spro->exec1_opcode) && spro->exec1_dst != 0 && spro->exec1_dst != 1) {
				sprn->r[spro->exec1_dst] = spro->exec1_aluout;
			}
			instructions++;
		}


	}

	// DMA //
	if (!start_dma && spro->exec1_opcode == CPY) {
		start_dma = 1;
	}


	if (sprn->dec1_opcode == LD || sprn->dec1_opcode == ST || sprn->exec0_opcode == LD || sprn->exec0_opcode == ST || sprn->exec1_opcode == LD || sprn->exec1_opcode == ST) {
		mem_available = 0;
	}
	else {
		mem_available = 1;
	}

	dma_state_machine_copy(sp);
	
}

static void sp_run(llsim_unit_t *unit)
{
	sp_t *sp = (sp_t *) unit->private;
	//	sp_registers_t *spro = sp->spro;
	//	sp_registers_t *sprn = sp->sprn;

	//	llsim_printf("-------------------------\n");

	if (llsim->reset) {
		sp_reset(sp);
		return;
	}

	sp->srami->read = 0;
	sp->srami->write = 0;
	sp->sramd->read = 0;
	sp->sramd->write = 0;

	sp_ctl(sp);
}

static void sp_generate_sram_memory_image(sp_t *sp, char *program_name)
{
        FILE *fp;
        int addr, i;

        fp = fopen(program_name, "r");
        if (fp == NULL) {
                printf("couldn't open file %s\n", program_name);
                exit(1);
        }
        addr = 0;
        while (addr < SP_SRAM_HEIGHT) {
                fscanf(fp, "%08x\n", &sp->memory_image[addr]);
                //              printf("addr %x: %08x\n", addr, sp->memory_image[addr]);
                addr++;
                if (feof(fp))
                        break;
        }
	sp->memory_image_size = addr;

        fprintf(inst_trace_fp, "program %s loaded, %d lines\n", program_name, addr);

	for (i = 0; i < sp->memory_image_size; i++) {
		llsim_mem_inject(sp->srami, i, sp->memory_image[i], 31, 0);
		llsim_mem_inject(sp->sramd, i, sp->memory_image[i], 31, 0);
	}
}

void sp_init(char *program_name)
{
	llsim_unit_t *llsim_sp_unit;
	llsim_unit_registers_t *llsim_ur;
	sp_t *sp;

	llsim_printf("initializing sp unit\n");

	inst_trace_fp = fopen("inst_trace.txt", "w");
	if (inst_trace_fp == NULL) {
		printf("couldn't open file inst_trace.txt\n");
		exit(1);
	}

	cycle_trace_fp = fopen("cycle_trace.txt", "w");
	if (cycle_trace_fp == NULL) {
		printf("couldn't open file cycle_trace.txt\n");
		exit(1);
	}

	llsim_sp_unit = llsim_register_unit("sp", sp_run);
	llsim_ur = llsim_allocate_registers(llsim_sp_unit, "sp_registers", sizeof(sp_registers_t));
	sp = llsim_malloc(sizeof(sp_t));
	llsim_sp_unit->private = sp;
	sp->spro = llsim_ur->old;
	sp->sprn = llsim_ur->new;

	sp->srami = llsim_allocate_memory(llsim_sp_unit, "srami", 32, SP_SRAM_HEIGHT, 0);
	sp->sramd = llsim_allocate_memory(llsim_sp_unit, "sramd", 32, SP_SRAM_HEIGHT, 0);
	sp_generate_sram_memory_image(sp, program_name);

	sp->start = 1;
	
	// c2v_translate_end
}
