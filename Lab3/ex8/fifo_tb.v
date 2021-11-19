`include "fifo.v"

module main;
   reg clk, reset, push, pop;
   reg [1:0] in;
   wire [1:0] out;
   wire full;
   integer failure = 0;

   // Correct the parameter assignment
   fifo #(4,2) f (clk, reset, in, push, pop, out, full);

   always #5 clk = ~clk;

   // Fill Here

   initial begin
      $dumpfile("waves.vcd");
      $dumpvars;
      clk = 0 ;
      #10

      //init - check the reset 
      reset = 1;
      in = 0;
      push = 0;
      pop = 0; 
      #10
      if (full != 0 || out != 0) 
         failure <= 1;

      #10 
      reset =0;
      #10 
      
      // push 1
      push = 1;
      in = 2'd1;
      #10;
      push=0;
      #10
      if (full != 0 || out != 2'd1 )
         failure <= 2;

      //push 2
      #10;
      push =1;
      in = 2'd2;
      #10;
      push=0;
      #10
      if (full != 0 || out != 2'd1)
         failure <= 3;

      // push 3
      #10;
      push =1;
      in = 2'd3;
      #10;
      push=0;
      #10
      if (full != 0 || out != 2'd1 )
         failure <= 4;

      // push 3
      #10;
      push =1;
      in = 2'd3;
      #10;
      push=0;
      #10
      if (full != 1 || out != 2'd1 ) 
         failure <= 5;

      //push when full - should do nothing 
      #10;
      push =1;
      in = 2'd3;
      #10;
      push=0;
      #10
      if (full != 1 || out != 2'd1 )
         failure <= 6;

      // pop (1)
      #10;
      pop =1;
      in = 0;
      #10;
      pop=0;
      #10
      if (full != 0 || out != 2'd1 ) 
         failure <= 7;

      // pop (2)
      #10;
      pop =1;
      #10;
      pop=0;
      #10
      if (full != 0 || out != 2'd2 )
         failure <= 8;

      // pop (3) & push (2)
      #10;
      pop =1;
      push = 1;
      in = 2'd2;
      #10;
      pop=0;
      push=0;
      #10
      if (full != 0 || out != 2'd3 ) 
         failure <= 9;

      #10 
      if(failure == 0)
         $display("PASSED ALL TESTS");
      else
         $display("DID NOT PASS ALL TESTS. last failureed in %d:", failure);

      $finish;
   end

endmodule
