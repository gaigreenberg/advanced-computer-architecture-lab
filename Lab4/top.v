/***********************************
 * TOP module
 **********************************/
module top;

   reg clk, reset, start;

   SP SP(clk, reset, start);

   always #5 clk = ~clk;
   
   initial
     begin
	$readmemh("example.bin", top.SP.SRAM.mem);
	clk = 1;     
	reset = 1;
	start = 0;
	$dumpfile("waves.vcd");
	$dumpvars;
	#100;
	reset = 0;
	start = 1;
     end
endmodule // top
