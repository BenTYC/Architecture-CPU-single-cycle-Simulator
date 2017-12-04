#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include "simulator.h"

unsigned int HtoD( unsigned int H[4] )
{
	H[0] <<= 24;
	H[1] <<= 16;
	H[2] <<= 8;
	return H[0] + H[1] + H[2] + H[3];
}

int check_overflow(unsigned int A, unsigned int B)
{
	int a = A; 
	int b = B;
	int sum = a + b;
	if ( (sum ^ a) >= 0 || (sum ^ b) >= 0 ){ //use xor to check
		return 0;
	}else{ 
		return 1; //overflow happen
	}
}

void Print_reg(unsigned int registers[REGS], unsigned int PC, int cycleCount, FILE *fps)
{
	int i;
	
	fprintf(fps, "cycle %d\n", cycleCount);
	for(i = 0; i < REGS; i++){
		fprintf(fps, "$%02d: 0x%08X\n", i, registers[i]);
	}
	fprintf(fps, "PC: 0x%08X\n\n\n", PC);
	return;
}

int check_W0( unsigned int instruction, int cycleCount, FILE *fpe)
{
	unsigned char opcode = getOpcode(instruction);
	unsigned char funct = getFunct(instruction);
	unsigned char rd = getRd(instruction);
	unsigned char rt = getRt(instruction);
	unsigned char shamt = getShamt(instruction);
	
	//NOP sll, $0, $0, 0
	if( opcode==0 && funct==0 && rd==0 && rt==0 && shamt==0 ){
		return 0;
	}
	
	if(opcode == RTYPEOP){
		if( funct == JR){
			return 0;
		}
		if ( rd == 0 ){
			fprintf( fpe , "In cycle %d: Write $0 Error\n", cycleCount); 
			return 1;
		}
	}else{
		if(opcode == SW || opcode == SH || opcode == SB || opcode == BEQ || opcode == BNE || opcode == J || opcode == JAL ){
			return 0;
		}
		if ( rt == 0 ){
			fprintf( fpe , "In cycle %d: Write $0 Error\n", cycleCount); 
			return 1;
		}
	}
	return 0;
}

int check_NO( unsigned int instruction, int cycleCount, FILE *fpe, unsigned int registers[REGS], unsigned int PC)
{
	unsigned char opcode = getOpcode(instruction);
	unsigned char funct = getFunct(instruction);
	unsigned char rt = getRt(instruction);
	unsigned char rs = getRs(instruction);
	unsigned int immediate = getImmediate(instruction);
	int signedImmediate = immediate;
	signedImmediate <<= 16;
	signedImmediate >>= 16;
		
	if(opcode == RTYPEOP){
		if( funct == ADD || funct == SUB ){ 
			unsigned int a = registers[rs];
			unsigned int b = registers[rt];
			if(funct == SUB){
				b = ~b + 1; //b=-b , a-b==a+(-b)
			}
			if ( check_overflow(a, b) ){ 
				fprintf( fpe, "In cycle %d: Number Overflow\n", cycleCount); 
				return 1;
			}
		}
	}else if(opcode == BEQ ){
		if ( check_overflow( PC, signedImmediate*4) ){ 
			fprintf( fpe, "In cycle %d: Number Overflow\n", cycleCount); 
			return 1;
		}
	}else if(opcode == BNE){
		if ( check_overflow( PC, signedImmediate*4) ){ 
			fprintf( fpe, "In cycle %d: Number Overflow\n", cycleCount); 
			return 1;
		}
	}else if(opcode == ADDI || opcode == LW || opcode == LH || opcode == LHU || opcode == LB || opcode == LBU || opcode == SW || opcode == SH || opcode == SB ){
		if ( check_overflow( registers[rs], signedImmediate) ){ 
			fprintf( fpe, "In cycle %d: Number Overflow\n", cycleCount); 
			return 1;
		}
	}
	return 0;
}

int check_AO( unsigned int instruction, int cycleCount, FILE *fpe, unsigned int registers[REGS], unsigned int PC)
{
	unsigned char opcode = getOpcode(instruction);
	unsigned char rs = getRs(instruction);
	unsigned char rt = getRt(instruction);
	int signedImmediate = getImmediate(instruction); //signed
	signedImmediate <<= 16;
	signedImmediate >>= 16;
	int n ;
	if(opcode == LW || opcode == LH || opcode == LHU || opcode == LB || opcode == LBU || opcode == SW || opcode == SH || opcode == SB ){
		n = registers[rs]+ signedImmediate;
		if( n < 0 || n > 1023){
			fprintf( fpe, "In cycle %d: Address Overflow\n", cycleCount);  
			return 1;
		}
		
		if( opcode == LW || opcode == SW){
			n = registers[rs] + signedImmediate + 3;
		}else if( opcode == LH || opcode == LHU || opcode == SH ){
			n = registers[rs] + signedImmediate + 1;
		}else{
			n = registers[rs] + signedImmediate;
		}
		if( n < 0 || n > 1023){
			
			fprintf( fpe, "In cycle %d: Address Overflow\n", cycleCount);  
			return 1;
		}
	}else if(opcode == BEQ){
		if( registers[rs] == registers[rt] ){
			n = PC + signedImmediate*4;
			if( n < 0 || n > 1023){
				fprintf( fpe, "In cycle %d: Address Overflow\n", cycleCount);  
				return 1;
			}
		}
	}else if(opcode == BNE){
		if( registers[rs] != registers[rt] ){
			n = PC + signedImmediate*4;
			if( n < 0 || n > 1023){
				fprintf( fpe, "In cycle %d: Address Overflow\n", cycleCount);  
				return 1;
			}
		}
	}
	return 0;
}

int check_MA( unsigned int instruction, int cycleCount, FILE *fpe, unsigned int registers[REGS])
{
	unsigned char opcode = getOpcode(instruction);
	unsigned char rs = getRs(instruction);
	int signedImmediate = getImmediate(instruction); //signed
	signedImmediate <<= 16;
	signedImmediate >>= 16;
	if(opcode == LW || opcode == SW ){
		if( ( registers[rs] + signedImmediate ) % 4 != 0){
			fprintf( fpe, "In cycle %d: Misalignment Error\n", cycleCount);  
			return 1;
		}
	}else if(opcode == LH || opcode == LHU || opcode == SH){
		if( ( registers[rs] + signedImmediate ) % 2 != 0){
			fprintf( fpe, "In cycle %d: Misalignment Error\n", cycleCount); 
			return 1;
		}		
	}else{
		return 0;
	}
}

unsigned char getOpcode( unsigned int Instruction)
{
	return (Instruction >> 26) & 0x3F;
}

unsigned char getFunct( unsigned int Instruction)
{
	return Instruction & 0x3F;
}

unsigned char getRs( unsigned int Instruction)
{
	return (Instruction >> 21) & 0x1F;
}

unsigned char getRt(unsigned int Instruction)
{
	return (Instruction >> 16) & 0x1F;
}

unsigned char getRd( unsigned int Instruction)
{
	return (Instruction >> 11) & 0x1F;
}

unsigned char getShamt( unsigned int Instruction)
{
	return (Instruction >> 6) & 0x1F;
}

unsigned int getImmediate( unsigned int Instruction)
{
	return Instruction & 0xFFFF;
}

unsigned int getAddress(unsigned int Instruction)
{
	return Instruction & 0x3FFFFFF;
}

int main(void) 
{
	FILE *fp, *fps, *fpe;
	int PCa[4], SPa[4], wordsI[4], wordsD[4], temp[4];
	int iNum, dNum, cycleCount = 0;
	unsigned int PC, SP, PC_origin, InsNow, Instruction;
	unsigned int Imem[MemorySize], Dmem[MemorySize], registers[REGS];
	unsigned char opcode, funct, rs, rt, rd, shamt;
	unsigned int address, immediate;
	int checkW0, checkNO, checkAO, checkMA;
	int i, j, tempn, temp1, temp2, signedImmediate;
		
	for(i = 0; i < MemorySize; i++) {
		Imem[i] = 0;
		Dmem[i] = 0;
	}
	for(i = 0; i < REGS; i++) {
		registers[i] = 0x0;
	}
		
	fp = fopen("iimage.bin","rb");
	assert(fp != NULL);
	for(i = 0; i < 4; i++) {
		PCa[i]=fgetc(fp);
	}
	for(i = 0; i < 4; i++) {
		wordsI[i]=fgetc(fp);
	}
	
	iNum = HtoD(wordsI);
	
	for(i = 0; i < iNum; i++) {
		for( j = 0; j < 4; j++) {
			temp[j]=fgetc(fp);
		}
		Imem[i] = HtoD(temp);
	}
	
	fclose(fp);
	
	
	fp = fopen("dimage.bin","rb");
	assert(fp != NULL);
	for(i = 0; i < 4; i++) {
		SPa[i]=fgetc(fp);
	}
	for(i = 0; i < 4; i++) {
		wordsD[i]=fgetc(fp);
	}
	
	dNum = HtoD(wordsD);
	
	for(i = 0; i < dNum; i++) {
		for( j = 0; j < 4; j++) {
			temp[j] = fgetc(fp);
		}
		Dmem[i] = HtoD(temp);
	}
	
	fclose(fp);
		
	PC = HtoD(PCa);
	SP = HtoD(SPa);
	PC_origin = PC;
	registers[29] = SP;
	
	fps = fopen("snapshot.rpt", "w");
	fpe = fopen("error_dump.rpt", "w");
	
	Print_reg(registers, PC, cycleCount, fps);
	opcode = getOpcode(Imem[0]);
	
	while( opcode != HALT ) {
		
		cycleCount++;
		
		if( PC >= PC_origin ){
			Instruction = Imem[(PC - PC_origin)/4];
		}else{
			Instruction = 0;
		}
		opcode = getOpcode( Instruction );
		PC += 4;
		
		checkW0 = check_W0(Instruction, cycleCount, fpe);
		checkNO = check_NO(Instruction, cycleCount, fpe, registers, PC);
		checkAO = check_AO(Instruction, cycleCount, fpe, registers, PC);
		checkMA = check_MA(Instruction, cycleCount, fpe, registers);
		
		if( checkAO == 1 || checkMA == 1){
			break;
		}
		if( checkW0 == 1 ){
			Print_reg(registers, PC, cycleCount, fps);
		    if( PC >= PC_origin ){
				opcode = getOpcode( Imem[(PC - PC_origin)/4] );
			}else{
				opcode = 0;
			}
			continue;
		}
		
		if( opcode == RTYPEOP ) {
			funct = getFunct(Instruction);
			shamt = getShamt(Instruction);
			rd = getRd(Instruction); 
			rt = getRt(Instruction);
			rs = getRs(Instruction);
			
			switch (funct) {
				case ADD:
					registers[rd] = registers[rs] + registers[rt];	
					break;
				case SUB:
					registers[rd] = registers[rs] - registers[rt];	
					break;
				case AND:
					registers[rd] = registers[rs] & registers[rt];	
					break;
				case OR:
					registers[rd] = registers[rs] | registers[rt];	
					break;
				case XOR:
					registers[rd] = registers[rs] ^ registers[rt];	
					break;
				case NOR:
					registers[rd] = ~(registers[rs] | registers[rt]);	
					break;
				case NAND:
					registers[rd] = ~(registers[rs] & registers[rt]);	
					break;
				case SLT:
					temp1 = registers[rs];
					temp2 = registers[rt];
					registers[rd] = (temp1 < temp2);
					break;
				case SLL:
					registers[rd] = registers[rt] << shamt;	
					break;
				case SRL:
					registers[rd] = registers[rt] >> shamt;	
					break;
				case SRA:
					tempn = registers[rt];
					registers[rd] = tempn >> shamt;	
					break;
				case JR:
					PC = registers[rs];
					break;
				default:
					printf("opcodeR wrong\n");	
					break;			
			}
			
		}else if( opcode == J || opcode == JAL ){
			address = getAddress(Instruction);
			switch (opcode) {
				case J:
					PC = (PC & 0xF0000000) | (address << 2);
					break;
				case JAL:
					registers[31] = PC;	
					PC = (PC & 0xF0000000) | (address << 2);
					break;
				default:
					//printf("opcodeJ wrong\n");
					break;					
			}
		}else if( opcode == LUI || opcode == ANDI || opcode == ORI || opcode == NORI){ //unsigned
			immediate = getImmediate(Instruction); 
			rt = getRt(Instruction);
			rs = getRs(Instruction);
			switch (opcode) {
				case LUI:
					registers[rt] = immediate << 16;	
					break;
				case ANDI:
					registers[rt] = registers[rs] & immediate;	
					break;	
				case ORI:
					registers[rt] = registers[rs] | immediate;	
					break;
				case NORI:
					registers[rt] = ~(registers[rs] | immediate);	
					break;				
			}
		}else{ 
			rs = getRs(Instruction);
			rt = getRt(Instruction);
			immediate = getImmediate(Instruction);
			signedImmediate = immediate;
			signedImmediate <<= 16;
			signedImmediate >>= 16;			
			switch (opcode) {
				case ADDI:					
					registers[rt] = registers[rs] + signedImmediate;
					break;
				case LW:
					registers[rt] = Dmem[(registers[rs] + signedImmediate)/4];	
					break;
				case LH:
					if( (registers[rs] + signedImmediate) % 4 == 0){
						tempn = Dmem[(registers[rs] + signedImmediate)/4];
						tempn >>= 16;
						registers[rt] = tempn;
					}else{
						tempn = Dmem[(registers[rs] + signedImmediate)/4];
						tempn <<= 16;
						tempn >>= 16;
						registers[rt] = tempn;
					}	
					break;
				case LHU:
					if( (registers[rs] + signedImmediate) % 4 == 0){
						registers[rt] = Dmem[(registers[rs] + signedImmediate)/4] >> 16;
					}else{
						registers[rt] = Dmem[(registers[rs] + signedImmediate)/4] & 0xFFFF;
					}	
					break;
				case LB:
					if( (registers[rs] + signedImmediate) % 4 == 0){
						tempn = Dmem[(registers[rs] + signedImmediate)/4];
						tempn >>= 24;
						registers[rt] = tempn;
					}else if( (registers[rs] + signedImmediate) % 4 == 1){
						tempn = Dmem[(registers[rs] + signedImmediate)/4];
						tempn <<= 8;
						tempn >>= 24;
						registers[rt] = tempn;
					}else if( (registers[rs] + signedImmediate) % 4 == 2){
						tempn = Dmem[(registers[rs] + signedImmediate)/4];
						tempn <<= 16;
						tempn >>= 24;
						registers[rt] = tempn;
					}else {
						tempn = Dmem[(registers[rs] + signedImmediate)/4];
						tempn <<= 24;
						tempn >>= 24;
						registers[rt] = tempn;
					}	
					break;
				case LBU:
					if( (registers[rs] + signedImmediate) % 4 == 0){
						registers[rt] = Dmem[(registers[rs] + signedImmediate)/4] >> 24;
					}else if( (registers[rs] + signedImmediate) % 4 == 1){
						registers[rt] = (Dmem[(registers[rs] + signedImmediate)/4] & 0xFF0000) >> 16;
					}else if( (registers[rs] + signedImmediate) % 4 == 2){
						registers[rt] = (Dmem[(registers[rs] + signedImmediate)/4] & 0xFF00) >> 8;
					}else {
						registers[rt] = Dmem[(registers[rs] + signedImmediate)/4] & 0xFF;
					}	
					break;
				case SW:
					Dmem[(registers[rs] + signedImmediate)/4] = registers[rt];	
					break;
				case SH:
					if( (registers[rs] + signedImmediate) % 4 == 0){
						Dmem[(registers[rs] + signedImmediate)/4] = (Dmem[(registers[rs] + signedImmediate)/4] & 0xFFFF) | (registers[rt] << 16);
					}else{
						Dmem[(registers[rs] + signedImmediate)/4] = (Dmem[(registers[rs] + signedImmediate)/4] & 0xFFFF0000) | (registers[rt] & 0xFFFF);	
					}
					break;
				case SB:
					if( (registers[rs] + signedImmediate) % 4 == 0){
						Dmem[(registers[rs] + signedImmediate)/4] = (Dmem[(registers[rs] + signedImmediate)/4] & 0x00FFFFFF) | (registers[rt] << 24);
					}else if( (registers[rs] + signedImmediate) % 4 == 1){
						Dmem[(registers[rs] + signedImmediate)/4] = (Dmem[(registers[rs] + signedImmediate)/4] & 0xFF00FFFF) | ((registers[rt] << 16) & 0xFF0000);
					}else if( (registers[rs] + signedImmediate) % 4 == 2){
						Dmem[(registers[rs] + signedImmediate)/4] = (Dmem[(registers[rs] + signedImmediate)/4] & 0xFFFF00FF) | ((registers[rt] << 8) & 0xFF00);
					}else {
						Dmem[(registers[rs] + signedImmediate)/4] = (Dmem[(registers[rs] + signedImmediate)/4] & 0xFFFFFF00) | (registers[rt] & 0xFF);
					}break;
				
				case SLTI:
					tempn = registers[rs];
					registers[rt] = (tempn < signedImmediate);
					break;
				case BEQ:
					if (registers[rs] == registers[rt]) {
						PC = PC + signedImmediate*4;
					}	
					break;	
				case BNE:
					if (registers[rs] != registers[rt]) {
						PC = PC + signedImmediate*4;
					}	
					break;		
				default:
					//printf("opcode wrong\n");
					break;				
			}
		}
		
		Print_reg(registers, PC, cycleCount, fps);
		
		if( PC >= PC_origin ){
			opcode = getOpcode( Imem[(PC - PC_origin)/4] );
		}else{
			opcode = 0;
		}
		
	}
	
	fclose(fps);
	fclose(fpe);
	
	return 0;
}
