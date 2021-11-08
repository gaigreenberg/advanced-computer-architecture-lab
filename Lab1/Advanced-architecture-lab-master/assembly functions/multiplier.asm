asm_cmd(ADD, 4, 0, 0, 0);      // 1:  set R4 = 0 - our counter
asm_cmd(LD,  2, 0, 1, 1000);   // 2: load R2 = mem[1000] (X)
asm_cmd(LD,  3, 0, 1, 1001);   // 3: load R3 = mem[1001] (Y)
asm_cmd(JLT, 0, 0, 2, 7);      // 4: if ( 0 < R2 ) : jump to line 7
asm_cmd(ADD, 4, 0, 1, 1);      // 5: R4 ++ (remember one was negative )
asm_cmd(SUB, 2, 0, 2, 0);      // 6: R2 = -R2 (make it abs )
asm_cmd(JLT, 0, 0, 3, 10);     // 7: if ( 0 < R3 ) : jump to line 10
asm_cmd(ADD, 4, 0, 1, 1);      // 8: R4 ++ (remember second was negative )
asm_cmd(SUB, 3, 0, 3, 0);      // 9: R3 = -R3 (make it abs )
asm_cmd(ADD, 6, 0, 0, 0);      // 10: R6 = 0 (result)
asm_cmd(JEQ, 0, 0, 3, 21);     // 11: if(R3 ==0 ) jump to the storing line 21
asm_cmd(AND, 5, 3, 1, 1);      // 12:  R5 = R3 & 0x1 (R5 == 1 if R3 is odd)  LOOP
asm_cmd(JEQ, 0, 5, 0, 16);     // 13:  if ( R5 == 0  {R3 is even}) jump to 16
asm_cmd(ADD, 6, 6, 2, 0);      // 14: R6 = R6 + R2 (result += X )
asm_cmd(SUB, 3, 3, 1, 1);      // 15: R3 -= 1 (Y--)
asm_cmd(LSF, 6, 6, 1, 1);      // 16: R6 = R6 << 1 ( result * 2 )
asm_cmd(RSF, 3, 3, 1, 1);      // 17: R3 = R3 >> 1 ( Y / 2 )
asm_cmd(JNE, 0, 0, 3, 12);     // 18: if( R3 != 0 ) jump to loop 12
asm_cmd(ADD, 5, 0, 1, 1);      // 0: R5 = 1 constant
asm_cmd(JNE, 0, 4, 5, 21);     // 19: if( R4 != R5 ) jump to storing ( else means the output should be negative)
asm_cmd(SUB, 6, 0, 3, 0);      // 20: R6 = - R6
asm_cmd(ST,  0, 6, 1, 1002);   // 21: store R6 (result) ro mem[1002]
asm_cmd(HLT, 0, 0, 0, 0);      // 22: END
