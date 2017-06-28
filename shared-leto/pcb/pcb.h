/*
 * pcb.h
 *
 *  Created on: Apr 6, 2017
 *      Author: utnso
 */

#ifndef PCB_H_
#define PCB_H_

#include <commons/collections/list.h>
#include <stdint.h>

typedef enum{
	VARIABLE,
	ARGUMENT,
}t_variableAdd;

typedef struct t_PCBVariableSize {
	uint32_t stackIndexRecordCount;
	uint32_t stackVariableCount;
	uint32_t stackArgumentCount;
	uint32_t instructionCount;
	uint32_t labelIndexSize;
} __attribute__((packed)) t_PCBVariableSize;

typedef struct t_memoryDirection {
	uint32_t page;
	uint32_t offset;
	uint32_t size;
} __attribute__((packed)) t_memoryDirection;

typedef struct t_stackVariable {
	char id;
	uint32_t page;
	uint32_t offset;
	uint32_t size;
} __attribute__((packed)) t_stackVariable;

typedef struct t_stackIndexRecord {
	t_list *arguments;
	t_list *variables;
	uint32_t returnPosition; // store IP to return
	t_memoryDirection returnVariable; // mem dir for return variable
} __attribute__((packed)) t_stackIndexRecord;

typedef struct codeIndex {
	uint32_t start;
	uint32_t size;
} __attribute__((packed)) t_codeIndex;

typedef struct t_PCB {
	t_PCBVariableSize variableSize;
	uint32_t pid; // Process ID
	int pc; // Program Counter
	int sp;	// Stack Pointer
	int ec;	// Exit Code
	uint32_t codePages;
	t_codeIndex *codeIndex;
	char *labelIndex;
	t_list *stackIndex;
	void *filesTable; // Puntero a la tabla de archivos del proceso
} __attribute__((packed)) t_PCB;

t_PCB *pcb_createDummy(int32_t processID, int32_t programCounter, int32_t StackPointer,int32_t ExitCode);
void pcb_addStackIndexRecord(t_PCB *PCB);
void pcb_addStackIndexVariable(t_PCB *PCB, t_stackVariable *variable, t_variableAdd type);
uint32_t pcb_getPCBSize(t_PCB *PCB);
void *pcb_serializePCB(t_PCB *PCB);
t_PCB *pcb_deSerializePCB(void *serializedPCB, t_PCBVariableSize *variableSize);
void pcb_dump(t_PCB *PCB);
uint32_t pcb_getBufferSizeFromVariableSize(t_PCBVariableSize *variableSize);
void pcb_destroy(t_PCB *PCB);
t_stackVariable *pcb_getVariable(t_PCB *PCB, char variableName);
void pcb_addSpecificStackIndexRecord(t_PCB *PCB,t_stackIndexRecord *record);
int pcb_getDirectionFromLabel(t_PCB *PCB, char *label);
t_PCBVariableSize pcb_getSizeOfSpecificStack(t_stackIndexRecord *record);
void pcb_decompileStack(t_PCB *PCB);
t_variableAdd pcb_getVariableAddTypeFromTag(char tag);

#endif /* PCB_H_ */
