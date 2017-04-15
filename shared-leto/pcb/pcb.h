/*
 * pcb.h
 *
 *  Created on: Apr 6, 2017
 *      Author: utnso
 */

#ifndef PCB_H_
#define PCB_H_

typedef struct t_PCB {
	int pid; // Process ID
	int pc; // Program Counter
	int sp;	// Stack Pointer
	int ec;	// Exit Code
	void *filesTable; // Puntero a la tabla de archivos del proceso
} t_PCB;


#endif /* PCB_H_ */
