#include <stdio.h>

#ifndef SIMULATOR_H_
#define SIMULATOR_H_

// R-Type Instructions
#define RTYPEOP 0x00
#define ADD	 0x20
#define SUB	 0x22
#define AND	 0x24
#define OR	 0x25
#define XOR	 0x26
#define NOR	 0x27
#define NAND 0x28
#define SLT	 0x2A
#define SLL	 0x00
#define SRL	 0x02
#define SRA	 0x03
#define JR	 0x08

// I-Type Instructions
#define ADDI 0x08
#define LW	 0x23
#define LH	 0x21
#define LHU  0x25
#define LB   0x20
#define LBU  0x24
#define SW	 0x2B
#define SH	 0x29
#define SB	 0x28
#define LUI	 0x0F
#define ANDI 0x0C
#define ORI	 0x0D
#define NORI 0x0E
#define SLTI 0x0A
#define BEQ	 0x04
#define BNE	 0x05

// J-Type Instructions
#define J	 0x02
#define JAL	 0x03

//Specialized Instruction
#define HALT 0x3F

//Array number
#define REGS 32
#define MemorySize 256
#define NOP 0

//function
unsigned int HtoD( unsigned int H[4] );
int check_overflow(unsigned int A, unsigned int B);
void Print_reg( unsigned int registers[REGS], unsigned int PC, int cycleCount, FILE *fps);
int check_W0( unsigned int instruction, int cycleCount, FILE *fpe);
int check_NO( unsigned int instruction, int cycleCount, FILE *fpe, unsigned int registers[REGS], unsigned int PC);
int check_AO( unsigned int instruction, int cycleCount, FILE *fpe, unsigned int registers[REGS], unsigned int PC);
int check_MA( unsigned int instruction, int cycleCount, FILE *fpe, unsigned int registers[REGS]);
unsigned char getOpcode( unsigned int Instruction);
unsigned char getFunct( unsigned int Instruction);
unsigned char getRs( unsigned int Instruction);
unsigned char getRt( unsigned int Instruction);
unsigned char getRd( unsigned int Instruction);
unsigned char getShamt( unsigned int Instruction);
unsigned int getImmediate( unsigned int Instruction);
unsigned int getAddress(unsigned int Instruction);


#endif /* SIMULATOR_H_ */
