`include "../ex5/addsub.v"

module sat_count(clk, reset, branch, taken, prediction);
   parameter N=2;
   input clk, reset, branch, taken;
   output prediction;

   // FILL HERE

   reg   [3:0] counter;
   reg   [3:0] e = {N{1'b1}} ;
   wire signed   [3:0] nxt_counter  ; 
   reg p ;

   addsub addsub(nxt_counter, counter, 4'b1, ~taken) ; // sets next counter to be ++ / -- depends on taken 
   
   always @(posedge clk)
      begin
      if (reset)  // on reset = 1 we init the counter
      counter <= 4'd0;
      else
         begin

            if(branch)
               begin

                  if(taken)
                     begin
                        if(nxt_counter > e )
                           counter <= e ; 
                        else 
                           counter <= nxt_counter;
                     end    
                  else  // taken = 0 
                     begin
                        if(nxt_counter < 0 )
                           counter <= 4'd0 ;
                        else 
                           counter <= nxt_counter ;
                     end
               end
            else 
               counter <= counter ; 
         end
         if(counter >= (2**(N-1)))
            assign p = 1 ;
         else
            assign p = 0 ;
      end
   
   assign prediction = p ;
   

endmodule
