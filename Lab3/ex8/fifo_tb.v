module main;
   reg clk, reset, push, pop;
   reg [2:0] in;
   wire [2:0] out;
   wire full;

   // Correct the parameter assignment
   fifo uut #(?,?) (clk, reset, in, push, pop, out, full);

   always #5 clk = ~clk;

   // Fill Here
endmodule
