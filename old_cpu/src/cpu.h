/*
 * variables.h
 *
 *  Created on: 27/04/2014
 *      Author: utnso
 */

#ifndef CPU_H_
#define CPU_H_

#include "memory.h"
#include <stdio.h>
#include <commons/collections/list.h>
#include <commons/config.h>
#include <commons/tcb.h>
#include <sys/types.h>
#include <commons/log.h>

#define PREDEFINED_QUANTUM 5

typedef enum{
	RUNNING,
	WAITING,
	INTERRUPTED,
}CPU_STATUS;

typedef enum{
	VERBOSE,
	QUIET,
}CPU_TALK;

typedef struct variableReference{
	char tag;
	u_int32_t offset; // from stack start
}variableReference;

typedef struct CPU{
	int instructionPointer;
	char *currentInstruction;
	int registers[5];
	int contextRegisters[4];
	int dirty;
	int stackBase;
	int stackCursor;
	int codeBase;
	t_TCB *assignedTCB;
	int quantum;
	CPU_STATUS status;
	CPU_TALK talk;
}CPU;

extern CPU myCPU;
extern t_config *configFile;
extern t_log *logFile;
//a
void cpu_create(CPU *,CPU_TALK);
void cpu_dump(CPU *);
void cpu_printAssignedTCB(CPU *);
void cpu_spendQuantumUnit(CPU *);
void cpu_clearRegisters(CPU *);
void cpu_setRunningMode(CPU *);
void cpu_setWaitingMode(CPU *);
void cpu_setInterruptedMode(CPU *);
void cpu_getTCB(CPU *,t_TCB *);
int cpu_parameterSizeFromInstruction(char *); // this is done by the kernel, only here as dummy method
void cpu_getRegistersFromTCB(CPU *);
void cpu_setRegistersToTCB(CPU *);
int cpu_fetch(CPU *);
void cpu_executeInstructionWithParameters(char*,char*);
void cpu_incrementInstructionPointer(CPU *,int);
int cpu_executeInstruction(CPU *, char *);
int cpu_setValueInRegister(CPU *,int, char *);
int cpu_getValueFromRegister(CPU *, char *);
void cpu_updateAssignedTCB(CPU *);
void cpu_setDirty(CPU *);
void cpu_setClean(CPU *);
void cpu_incrementStackCursor(CPU *, int);
void cpu_idecrementStackCursor(CPU *, int);
void cpu_abortWithError(CPU *, char*);
void kernel_sendInterruption(CPU *, int, int);
#endif /* VARIABLES_H_ */
