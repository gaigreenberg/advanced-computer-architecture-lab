`include "fulladder.v"
module main;
//FILL HERE
reg a,b,ci;
wire sum, carry;
fulladder fulladder_test(sum,carry,a,b,ci);

always@(sum or carry or b)
begin
$display("time=%d: %b + %b + %b = %b, carry = %b\n", $time, a, b, ci, sum, carry);
end

initial
begin
$dumpfile("waves.vcd");
$dumpvars;
// we are testing all the 8 possible inputs with 5 ns wait between each attempt
a = 0; b = 0; ci=0;
#5
a = 0; b = 0; ci=1;
#5
a = 0; b = 1; ci=0;
#5
a = 0; b = 1; ci=1;
#5
a = 1; b = 0; ci=0;
#5
a = 1; b = 0; ci=1;
#5
a = 1; b = 1; ci=0;
#5
a = 1; b = 1; ci=1;
#5
$finish;
end
endmodule
