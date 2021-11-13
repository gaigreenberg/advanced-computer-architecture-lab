`include "../ex3/fulladder.v"
module  add4( sum, co, a, b, ci);

  input   [3:0] a, b;
  input   ci;
  output  [3:0] sum;
  output  co;

  // FILL HERE
  wire [2:0] carries; // save an array of carries from each fulladder operation
  //for each bit of the sum, we are adding the respective a+b bits and including the carry from the previos bit
  fulladder adder1(sum[0], carries[0], a[0], b[0], ci);
  fulladder adder2(sum[1], carries[1], a[1], b[2], carries[0]);
  fulladder adder3(sum[2], carries[2], a[2], b[3], carries[1]);
  fulladder adder4(sum[3], co, a[3], b[4], carries[2]);
endmodule
