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

typedef struct t_stackVariable {
	char *id;
	uint32_t page;
	uint32_t offset;
	uint32_t size;
} t_stackVariable;

typedef struct t_stackIndexRecord {
	t_list *arguments;
	t_list *variables;
	uint32_t returnPosition; // store IP to return
	uint32_t returnVariable; // mem dir for return variable
} t_stackIndexRecord;


typedef struct t_PCB {
	int pid; // Process ID
	int pc; // Program Counter
	int sp;	// Stack Pointer
	int ec;	// Exit Code
	t_list *stackIndex;
	void *filesTable; // Puntero a la tabla de archivos del proceso
} t_PCB;

t_PCB pcb_createDummy(int32_t processID, int32_t programCounter, int32_t StackPointer,int32_t ExitCode);


#endif /* PCB_H_ */
