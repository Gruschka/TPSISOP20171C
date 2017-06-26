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
} t_PCBVariableSize;

typedef struct t_memoryDirection {
	uint32_t page;
	uint32_t offset;
	uint32_t size;
} t_memoryDirection;

typedef struct t_stackVariable {
	char id;
	uint32_t page;
	uint32_t offset;
	uint32_t size;
} t_stackVariable;

typedef struct t_stackIndexRecord {
	t_list *arguments;
	t_list *variables;
	uint32_t returnPosition; // store IP to return
	t_memoryDirection returnVariable; // mem dir for return variable
} t_stackIndexRecord;


typedef struct t_PCB {
	uint32_t pid; // Process ID
	int pc; // Program Counter
	int sp;	// Stack Pointer
	int ec;	// Exit Code
	t_list *stackIndex;
	t_PCBVariableSize variableSize;
	void *filesTable; // Puntero a la tabla de archivos del proceso
} t_PCB;

t_PCB *pcb_createDummy(int32_t processID, int32_t programCounter, int32_t StackPointer,int32_t ExitCode);
void pcb_addStackIndexRecord(t_PCB *PCB, t_stackIndexRecord *record);
void pcb_addStackIndexVariable(t_PCB *PCB, t_stackIndexRecord *stackIndex, t_stackVariable *variable, t_variableAdd type);
uint32_t pcb_getPCBSize(t_PCB *PCB);
void *pcb_serializePCB(t_PCB *PCB);
t_PCB *pcb_deSerializePCB(void *serializedPCB, int totalSize, int stackIndexRecordCount, int stackIndexVariableCount, int stackIndexArgumentCount);
void pcb_dump(t_PCB *PCB);

#endif /* PCB_H_ */
