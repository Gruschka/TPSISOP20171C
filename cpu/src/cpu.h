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

typedef enum{
	T_KERNEL,
	T_MEMORY,
}t_connectionType;

typedef enum{
	RUNNING,
	WAITING,
	INTERRUPTED,
	CONNECTED,
	DISCONNECTED,
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
	int variableCounter;
	t_CPUConnection connections[2];
	int instructionPointer;
	char *currentInstruction;
	t_PCB *assignedPCB;
	t_CPUStatus status;
}t_CPU;

extern t_CPU myCPU;

uint32_t cpu_start(t_CPU *CPU);
uint32_t cpu_connect(t_CPU *CPU, t_connectionType connectionType);
uint32_t cpu_getPCB(t_CPU *CPU, t_PCB PCB);

#endif /* CPU_H_ */
