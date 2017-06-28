/*
 * cpu.h
 *
 *  Created on: Apr 15, 2017
 *      Author: utnso
 */

#ifndef CPU_H_
#define CPU_H_

#include "memory.h"
#include <stdio.h>
#include <commons/collections/list.h>
#include <ipc/ipc.h>
#include <pcb/pcb.h>
#include <sys/types.h>
#include <stdint.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>

//#define KERNEL 0
//#define MEMORY 1

#define DUMMY_MEMORY_PAGE_SIZE 512

typedef enum{
	T_KERNEL,
	T_MEMORY,
	T_D_KERNEL,
	T_D_MEMORY
}t_connectionType;

typedef enum{
	RUNNING,
	WAITING,
	INTERRUPTED,
	CONNECTED,
	DISCONNECTED,
	FINISHED,
	FINISHED_QUANTUM,
}t_CPUStatus;

typedef struct variableReference{
	char tag;
	u_int32_t offset; // from stack start
}t_variableReference;

typedef struct CPUConnection{
	int socketFileDescriptor;
	t_CPUStatus status;
	int portNumber;
	char *host;
	struct sockaddr_in serv_addr;
	struct hostent *server;
}t_CPUConnection;

typedef struct CPU{
	int quantum;
	int variableCounter;
	t_CPUConnection connections[2];
	int instructionPointer;
	char *currentInstruction;
	t_PCB *assignedPCB;
	t_CPUStatus status;
}t_CPU;


extern t_CPU myCPU;
extern void *myMemory;

uint32_t cpu_start(t_CPU *CPU);
uint32_t cpu_connect(t_CPU *CPU, t_connectionType connectionType);
uint32_t cpu_getPCB(t_CPU *CPU, t_PCB PCB);
void *cpu_readMemoryDummy(uint32_t pid, uint32_t page, uint32_t offset, uint32_t size);
uint32_t cpu_writeMemoryDummy(uint32_t pid, uint32_t page, uint32_t offset, uint32_t size, void *buffer);
t_PCB *cpu_createPCBFromScript(char *script);
uint32_t cpu_declareVariable(char variableName);
void cpu_endProgram();
uint32_t cpu_getVariablePosition(char variableName);
int cpu_dereference(uint32_t variableAddress);
void cpu_assignValue(uint32_t variableAddress, int value);
void cpu_gotoLabel(char *label);
void cpu_callNoReturn(char *label);
void cpu_callWithReturn(char *label, uint32_t returnAddress);
void cpu_return(int returnValue);

#endif /* CPU_H_ */
