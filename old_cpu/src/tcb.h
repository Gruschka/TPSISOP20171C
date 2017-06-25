/*
 * tcb.h
 *
 *  Created on: Sep 29, 2014
 *      Author: utnso
 */

#ifndef TCB_H_
#define TCB_H_

#define DEFAULT_STACK_SIZE 512

typedef struct t_instructionIndex {
	int instructionOffset;
	int instructionSize;
} t_instructionIndex;

typedef struct t_TCB {
int pid; //Identificador del proceso
int tid; //Identificador del hilo
char km; //Indicador de Kernel Mode
int codeSegmentBase; //Base del segmento de código
int codeSegmentSize; //Tamano del segmento de código
int instructionPointer; //Instruction pointer
int stackBase; //Base del stack
int stackCursor; //Cursor del stack
int registers[5];//Registros
} t_TCB;



t_TCB *tcb_createFromScriptWithSize(const char *,int);
void tcb_getSegments(t_TCB *, int, int); // this is done by the kernel, only here as dummy method
void tcb_clearRegisters(t_TCB *);
t_instructionIndex *tcb_generateIndexFromScript(char *, int); // this is done by the kernel, only here as dummy method
int tcb_parameterSizeFromInstruction(char *); // this is done by the kernel, only here as dummy method
char *tcb_getIdentification(t_TCB *);
#endif /* TCB_H_ */
