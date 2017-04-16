/*
 * pcb.h
 *
 *  Created on: Apr 6, 2017
 *      Author: utnso
 */

#ifndef PCB_H_
#define PCB_H_

#include <commons/collections/list.h>

typedef struct t_stackVariable {
	char *id;
	uint32_t page;
	uint32_t offset;
	uint32_t size;
} t_stackVariable;

typedef struct t_stackIndexRecord {
	t_list arguments;
	t_list variables;
	uint32_t returnPosition; // store IP to return
	uint32_t returnVariable; // mem dir for return variable
} t_stackIndexRecord;

typedef struct t_stackIndex {
	t_list records;
} t_stackIndex;

typedef struct t_PCB {
	int pid; // Process ID
	int pc; // Program Counter
	int sp;	// Stack Pointer
	int ec;	// Exit Code
	t_stackIndex stackIndex;
	void *filesTable; // Puntero a la tabla de archivos del proceso
} t_PCB;


#endif /* PCB_H_ */
