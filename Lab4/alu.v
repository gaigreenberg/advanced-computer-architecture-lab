`include "defines.vh"

/***********************************
 * ALU module
 **********************************/
module ALU(opcode, alu0, alu1, aluout);
   
   input [4:0] opcode;
   input [31:0] alu0, alu1;
   output [31:0] aluout;
   reg [31:0] 	 aluout;

   always@(alu0 or alu1 or opcode)
     begin
	case (opcode)
	  `ADD: aluout = alu0 + alu1;
	  /***********************************
           * TODO: fill here
           **********************************/

	endcase
     end
endmodule // alu
