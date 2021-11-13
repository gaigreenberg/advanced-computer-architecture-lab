module mux1(a,b,select,result);
   input a;
   input b;
   input select;
   output result;

   assign result = // FILL HERE
endmodule

module mux2(a,b,select,result);
   input a;
   input b;
   input select;
   output result;

   reg result;
   always @(a or b or select)
     begin
        // FILL HERE
     end
endmodule
