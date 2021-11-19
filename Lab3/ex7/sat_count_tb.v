`include "sat_count.v"

module main ;

reg clk, reset, branch, taken , answer;
wire prediction;

sat_count counter(clk, reset, branch, taken, prediction) ;

always #5 clk = ~clk; //taken from previous tb 

initial
     begin
        $display("starting 4 tests ");
        $dumpfile("waves.vcd");
        $dumpvars;
        clk = 0; 

        // TEST 1 
        taken = 0 ; 
        branch = 0 ; 
        reset = 1 ; 
        # 10;
        reset = 0 ; 

        if( prediction == 0)
            $display("First test passed");
        
        #10 ;

        // TEST 2 
        branch = 1;
        taken = 1 ;
        answer = 1; 
        #20 //after 2 cycles prediction should be 0
        answer = answer & (~prediction) ;
        #20 //after 2 cycles prediction should be 1
        answer = answer & (prediction) ;
        if ( answer)
            $display("Second test passed");
        #10 ;

        //TEST 3 

        taken = 0; 
        #40 // after 4 cycles should return 0 again
        if( prediction == 0)
            $display("Third test passed");
        
        #10 ;

        //TEST 4 

        branch = 0 ; 
        #20 // after 2 cycles shouldnt change 
        if( prediction == 0)
            $display("Fourth test passed");
        
        #10 ;

        $finish;
     end
endmodule
