asm_cmd(JEQ, 0, 0, 0, 2)			// 0: if(0 == 0)	jump to line 2 (START) 
asm_cmd(HLT, 0, 0, 0, 0)			// 1: HALT 
asm_cmd(ADD, 2, 0, 1, 150)			// 2: R2 = 150 (counter for the loop)
asm_cmd(ADD, 6, 0, 1, 200)			// 3: R6 = 200 (dest addres for the CPY)
asm_cmd(ADD, 5, 0, 1, 100)			// 4: R5 = 100 (src address for CPY)
asm_cmd(CPY, 0, 5, 6, 150)			// 5: copy from 100 to 200 with length 150 - overlapping
asm_cmd(DMS, 0, 0, 0, 0)			// 6: checking DMS command works 
									   
asm_cmd(LD, 3, 0, 5, 0)				// 7: R3 = Mem[R5]	(LOOP)
asm_cmd(LD, 4, 0, 6, 0)				// 8: R4 = Mem[R6]	(also checks structural hazard)
asm_cmd(JNE, 0, 3, 4, 16)			// 9: if(R3 != R4)	jump to failed code line 
asm_cmd(ADD, 5, 5, 1, 1)			//10: R5++
asm_cmd(ADD, 6, 6, 1, 1)			//11: R6++
asm_cmd(SUB, 2, 2, 1, 1)			//12: R2--
asm_cmd(JNE, 0, 0, 2, 7)			//13: if(R2 != 0) -> got to loop again 
asm_cmd(ADD, 2, 0, 1, 1)			//14: R2 = 1 - success
asm_cmd(HLT, 0, 0, 0, 0)			//15: HALT 
asm_cmd(ADD, 2, 0, 0, 0)			//16: R2 = 0 - FAIL 
asm_cmd(HLT, 0, 0, 0, 0)			//17: HALT 

