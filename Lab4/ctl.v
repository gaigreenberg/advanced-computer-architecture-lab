`include "defines.vh"
`include "top.v"
/***********************************
 * CTL module
 **********************************/
module CTL(
	   clk,
	   reset,
	   start,
	   sram_ADDR,
	   sram_DI,
	   sram_EN,
	   sram_WE,
	   sram_DO,
	   opcode,
	   alu0,
	   alu1,
	   aluout_wire
	   );

   // inputs
   input clk;
   input reset;
   input start;
   input [31:0] sram_DO;
   input [31:0] aluout_wire;

   // outputs
   output [15:0] sram_ADDR;
   output [31:0] sram_DI;
   output 	 sram_EN;
   output 	 sram_WE;
   output [31:0] alu0;
   output [31:0] alu1;
   output [4:0]  opcode;

   // registers
   reg [31:0] 	 r2;
   reg [31:0] 	 r3;
   reg [31:0] 	 r4;
   reg [31:0] 	 r5;
   reg [31:0] 	 r6;
   reg [31:0] 	 r7;
   reg [15:0] 	 pc;
   reg [31:0] 	 inst;
   reg [4:0] 	 opcode;
   reg [2:0] 	 dst;
   reg [2:0] 	 src0;
   reg [2:0] 	 src1;
   reg [31:0] 	 alu0;
   reg [31:0] 	 alu1;
   reg [31:0] 	 aluout;
   reg [31:0] 	 immediate;
   reg [31:0] 	 cycle_counter;
   reg [2:0] 	 ctl_state;

   integer 	 verilog_trace_fp, rc;

   initial
     begin
	verilog_trace_fp = $fopen("verilog_trace.txt", "w");
     end

   /***********************************
    * set up sram inputs (outputs from sp)*/

	reg sram_EN;
	reg sram_WE;
	reg[31:0] sram_DI;
	reg[15:0] sram_ADDR;


   // synchronous instructions
   always@(posedge clk) begin
	if (reset) begin
	   // registers reset
	   r2 <= 0;
	   r3 <= 0;
	   r4 <= 0;
	   r5 <= 0;
	   r6 <= 0;
	   r7 <= 0;
	   pc <= 0;
	   inst <= 0;
	   opcode <= 0;
	   dst <= 0;
	   src0 <= 0;
	   src1 <= 0;
	   alu0 <= 0;
	   alu1 <= 0;
	   aluout <= 0;
	   immediate <= 0;
	   cycle_counter <= 0;
	   ctl_state <= 0;
	   
	end else begin
	   // generate cycle trace
	   $fdisplay(verilog_trace_fp, "cycle %0d", cycle_counter);
	   $fdisplay(verilog_trace_fp, "r2 %08x", r2);
	   $fdisplay(verilog_trace_fp, "r3 %08x", r3);
	   $fdisplay(verilog_trace_fp, "r4 %08x", r4);
	   $fdisplay(verilog_trace_fp, "r5 %08x", r5);
	   $fdisplay(verilog_trace_fp, "r6 %08x", r6);
	   $fdisplay(verilog_trace_fp, "r7 %08x", r7);
	   $fdisplay(verilog_trace_fp, "pc %08x", pc);
	   $fdisplay(verilog_trace_fp, "inst %08x", inst);
	   $fdisplay(verilog_trace_fp, "opcode %08x", opcode);
	   $fdisplay(verilog_trace_fp, "dst %08x", dst);
	   $fdisplay(verilog_trace_fp, "src0 %08x", src0);
	   $fdisplay(verilog_trace_fp, "src1 %08x", src1);
	   $fdisplay(verilog_trace_fp, "immediate %08x", immediate);
	   $fdisplay(verilog_trace_fp, "alu0 %08x", alu0);
	   $fdisplay(verilog_trace_fp, "alu1 %08x", alu1);
	   $fdisplay(verilog_trace_fp, "aluout %08x", aluout);
	   $fdisplay(verilog_trace_fp, "cycle_counter %08x", cycle_counter);
	   $fdisplay(verilog_trace_fp, "ctl_state %08x\n", ctl_state);

	   cycle_counter <= cycle_counter + 1;
	   case (ctl_state)

	     	`CTL_STATE_IDLE: begin
                pc <= 0;
                if (start)
                  ctl_state <= `CTL_STATE_FETCH0;
             end

        	`CTL_STATE_FETCH0: begin
			 	ctl_state <= `CTL_STATE_FETCH1;
		 	end

			`CTL_STATE_FETCH1: begin
				inst <= sram_DO; //load the next operation to inst register
				ctl_state <= `CTL_STATE_DEC0;
			end

			`CTL_STATE_DEC0: begin
				//parse opcode to the registers according to specs in lab1
				opcode <= inst[29:25];
				dst <= inst[24:22];
				src0 <= inst[21:19];
				src1 <= inst[18:16];
				//handle immediate
				if(inst[15] == 1) begin// if the immediate is negative
					immediate <= inst|((~(32'b0))<<16); //do sign extension to 32 bits in 2's complement
				end 
				else
					immediate <= inst[15:0];
				ctl_state <= `CTL_STATE_DEC1;
			end

			`CTL_STATE_DEC1: begin
				//load values into alu0
				if(src0==0)
					alu0 <= 0;
				if(src0==1)
					alu0 <= immediate;
				if(src0==2)
					alu0 <= r2;
				if(src0==3)
					alu0 <= r3;
				if(src0==4)
					alu0 <= r4;
				if(src0==5)
					alu0 <= r5;
				if(src0==6)
					alu0 <= r6;
				if(src0==7)
					alu0 <= r7;
				//load value into alu1
				if(src1==0)
					alu1 <= 0;
				if(src1==1)
					alu1 <= immediate;
				if(src1==2)
					alu1 <= r2;
				if(src1==3)
					alu1 <= r3;
				if(src1==4)
					alu1 <= r4;
				if(src1==5)
					alu1 <= r5;
				if(src1==6)
					alu1 <= r6;
				if(src1==7)
					alu1 <= r7;
				ctl_state <= `CTL_STATE_EXEC0;
			end

			`CTL_STATE_EXEC0: begin
				if (opcode!=`LD && opcode!=`ST && opcode!=`HLT) //handle all alu operations
	     			aluout <= aluout_wire;
	     		ctl_state <= `CTL_STATE_EXEC1;
			end

			`CTL_STATE_EXEC1: begin
				pc <= pc + 1;
			  	if(opcode == `LD) begin
					if(dst==2)
						r2 <= sram_DO;
					if(dst==3)
						r3 <= sram_DO;	
					if(dst==4)
						r4 <= sram_DO;				  			     		
					if(dst==5)
						r5 <= sram_DO;	
					if(dst==6)
						r6 <= sram_DO;	
					if(dst==7)
						r7 <= sram_DO;	
				end

				else if(opcode == `ST)begin
				  //nothing
				end

				else if(opcode == `HLT) begin
					ctl_state <= `CTL_STATE_IDLE;
					$fclose(verilog_trace_fp);
					$writememh("verilog_sram_out.txt", top.SP.SRAM.mem);
					$finish;
				end

				else if(opcode == `ADD || opcode == `SUB || opcode == `AND || opcode == `OR || opcode == `XOR || opcode == `LSF || opcode == `RSF || opcode == `LHI) begin
					if(dst==2)
						r2 <= aluout_wire;
					if(dst==3)
						r3 <= aluout_wire;
					if(dst==4)
						r4 <= aluout_wire;
					if(dst==5)
						r5 <= aluout_wire;	
					if(dst==6)
						r6 <= aluout_wire;	
					if(dst==7)
						r7 <= aluout_wire;	
				end

				else if(opcode == `JEQ || opcode == `JLE || opcode == `JNE || opcode == `JIN) begin //handle jump
					if(aluout == 1) begin
					  r7 <= pc; //store old pc
					  pc <= immediate;
					end
				end	

				ctl_state <= `CTL_STATE_FETCH0;
			end
	   endcase
	end // !reset
end // @posedge(clk)

	//handle sram inputs for every state switch
	always @(ctl_state) begin

		sram_EN = 1;
		sram_WE = 0;
		sram_ADDR = 0;
		sram_DI = 0;

		case(ctl_state)

			`CTL_STATE_FETCH0: begin
				sram_EN = 1;
				sram_WE = 0;
				sram_ADDR = pc[15:0];
				sram_DI = 0;
			end

			`CTL_STATE_EXEC0: begin
				if (opcode == `LD) begin //handle load operation
					sram_EN = 1;
					sram_WE = 0;
					sram_ADDR = alu1[15:0];
					sram_DI = 0;
				end
			end

			`CTL_STATE_EXEC1: begin
				if (opcode == `ST) begin //handle store operation
					sram_EN = 1;
					sram_WE = 1;
					sram_ADDR = alu1[15:0];
					sram_DI = alu0;
				end
			end

			default: begin
				sram_EN = 0;
				sram_WE = 0;
				sram_ADDR = 0;
				sram_DI = 0;
			end
		endcase
	end
		



endmodule // CTL
