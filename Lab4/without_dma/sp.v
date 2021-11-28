
/***********************************
 * SP module
 **********************************/

 //`include "alu.v"
 //`include "ctl.v"
 //`include "sram.v"

module SP(clk, reset, start);
   input clk;
   input reset;
   input start;

   wire [15:0] sram_ADDR;
   wire [31:0] sram_DI;
   wire        sram_EN;
   wire        sram_WE;
   wire [31:0] sram_DO;
   wire [4:0]  opcode;
   wire [31:0] alu0;
   wire [31:0] alu1;
   wire [31:0] aluout_wire;

   // instantiate ALU, CTL and SRAM modules
   ALU ALU(opcode, alu0, alu1, aluout_wire);
   CTL CTL(clk, reset, start, sram_ADDR, sram_DI, sram_EN, sram_WE, sram_DO, opcode, alu0, alu1, aluout_wire);
   SRAM SRAM(clk, sram_ADDR, sram_DI, sram_EN, sram_WE, sram_DO);
endmodule  
