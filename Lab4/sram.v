/*****************************************
 * SRAM module: synchronus SRAM, 65536x16
 * 
 * inputs:
 * 
 * clk: system clock
 * addr[15:0]: 16 bit address
 * di[31:0]: 32 bit input data
 * en: enable signal
 * we: write enable (1 write, 0 read)
 ****************************************/
module SRAM(clk, addr, di, en, we, do);

   input clk;
   input [15:0] addr;
   input [31:0] di;
   input 	en;
   input 	we;

   output [31:0] do;
   
   reg [31:0] 	 do;
   reg [31:0] 	 mem[0:65535];
   
   always @(posedge clk)
     begin
	if (en) begin
	   if (we) begin
	      /***********************************
               * TODO: fill here
               **********************************/
	      $display("time %0d: write %08x -> mem[%04x]", $time, di, addr);
	   end
	   else begin
	      /***********************************
               * TODO: fill here
               **********************************/
	      $display("time %0d: read mem[%04x] -> %08x", $time, addr, mem[addr]);
	   end
	end
     end
endmodule // sram
