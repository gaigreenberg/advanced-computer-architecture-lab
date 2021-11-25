/***********************************
 * SRAM testbench
 ***********************************/
module top;

   reg clk;

   reg [15:0] sram_ADDR;
   reg [31:0] sram_DI;
   reg 	      sram_EN;
   reg 	      sram_WE;
   wire [31:0] sram_DO;

   integer    i;

   SRAM SRAM(clk, sram_ADDR, sram_DI, sram_EN, sram_WE, sram_DO);
   
   always #5 clk = ~clk;

   initial
     begin
	clk = 1;
	sram_EN = 0;
	$dumpfile("waves.vcd");
	$dumpvars;
	@(posedge clk);
	for (i = 0; i < 65536; i = i + 1) begin
	   // write mem[i] = i
	   sram_ADDR <= i[15:0];
	   sram_DI <= i[15:0];
	   sram_EN <= 1;
	   sram_WE <= 1;
	   @(posedge clk);

	   // read mem[i]
	   /***********************************
            * TODO: fill here
            **********************************/
	   @(posedge clk)

	   // verify mem[i]
	     #1;

	   if (sram_DO[15:0] !== i[15:0]) begin
	      $display("ERROR: read address %0d expected %0d, got %0d", i, i, sram_DO);
	      $finish;
	   end
	end // for (i = 0; i < 65536; i = i + 1)
	$display("SRAM test finished successfully");
	$finish;
     end // initial begin
   
endmodule // top
