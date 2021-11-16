module parity(clk, in, reset, out);

   input clk, in, reset;
   output out;

   reg 	  out;
   reg 	  state;

   localparam zero=0, one=1;

   always @(posedge clk)
     begin
	if (reset)
	  state <= zero;
	else
	  case (state)
	    // FILL HERE - done 
      one: state <= ~ in ;

      zero: state <= in ;

      default:
        state <= one ; //on start there was 0 ones - even number
        
	  endcase
     end

   always @(state) 
     begin
	case (state)
	    // FILL HERE - done 
      one: out <= 1 ;

      zero: out <= 0 ;

      default: out <= 1 ;

	endcase
     end

endmodule
