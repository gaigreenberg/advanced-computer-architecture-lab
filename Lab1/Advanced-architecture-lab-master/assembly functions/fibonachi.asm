asm_cmd(ADD, 2, 0, 1, 1);       //0: R2 = 1
asm_cmd(ST, 0, 2, 1, 1000);     //1: mem[1000] = 1
asm_cmd(ST, 0, 2, 1, 1001);     //2: mem[1001] = 1
asm_cmd(ADD, 3, 0, 1, 1);       //3: R3 = 1
asm_cmd(ADD, 5, 0, 1, 1002);    //4: R5 = 1002 (memory pointer)
asm_cmd(ADD, 6, 0, 1, 1040);    //5: R6 = 1040
asm_cmd(ADD, 4, 2, 3, 0);       //6: R4 = R2 + R3 (next fibo number )
asm_cmd(ST, 0, 4, 5, 0);        //7: mem[R5] = R4
asm_cmd(ADD, 5, 5, 1, 1);       //8: R5 ++ (prepare next memory pointer)
asm_cmd(ADD, 2, 3, 0, 0);       //9: R2 = R3
asm_cmd(ADD, 3, 4, 0, 0);       //10: R3 = R4
asm_cmd(JNE, 0, 5, 6, 6);       //11: if(R5 != R6) jump to 6 - memory pointer not 1040 yet
asm_cmd(HLT, 0, 0, 0, 0);       //12: HALT