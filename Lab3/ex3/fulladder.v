module fulladder( sum, co, a, b, ci);

  input   a, b, ci;
  output  sum, co;

  // FILL HERE
  wire s1,c2,c3,c4;
  xor (s1,a,b); //s1 = a + b
  xor (sum, s1, ci); // sum = s1 + ci

  // find the carry by testing all the possible combinations of 2 input params - if one of the options is '1' - carry is '1'

  and(c1,a,ci);
  and(c2, b, ci);
  and(c3, a, b);
  or (co, c1, c2, c3);

endmodule
