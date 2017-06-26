/*
 * kernel.h
 *
 *  Created on: Apr 6, 2017
 *      Author: utnso
 */

#ifndef KERNEL_H_
#define KERNEL_H_

#include "scheduling.h"
#include <ipc/ipc.h>

typedef struct t_kernel_config {
	int programsPort;
	int cpuPort;
	char *memoryIP;
	int memoryPort;
	char *filesystemIP;
	int filesystemPort;
	int quantum;
	int quantumSleep;
	t_scheduling_algorithm schedulingAlgorithm; // FIFO/RR
	int multiprogrammingDegree;
	int stackSize;
	//TODO: Agregar semaforos y variables compartidas
} t_kernel_config;

void fetchConfiguration();

void *consolesServer_main(void *args);

void consolesServerSocket_handleDeserializedStruct(int fd, ipc_operationIdentifier operationId, void *buffer);

void consolesServerSocket_handleNewConnection(int fd);

void consolesServerSocket_handleDisconnection(int fd);

void *scheduler_mainFunction(void);
void *dispatcher_mainFunction(void);
void *configurationWatcherThread_mainFunction();

void executeNewProgram(t_PCB *pcb);

#endif /* KERNEL_H_ */
