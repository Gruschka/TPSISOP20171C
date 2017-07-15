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
	char **semaphoreIDs;
	char **semaphoreValues;
	char **sharedVariableNames;
	//TODO: Agregar semaforos y variables compartidas
} t_kernel_config;

typedef struct active_console {
	int fd;
	int pid;
} t_active_console;

typedef struct t_CPU {
	int fd;
	bool isAvailable;
} t_CPUx;

void fetchConfiguration();

void *consolesServer_main(void *args);
void consolesServerSocket_handleDeserializedStruct(int fd, ipc_operationIdentifier operationId, void *buffer);
void consolesServerSocket_handleNewConnection(int fd);
void consolesServerSocket_handleDisconnection(int fd);

void *cpusServer_main(void *args);
void cpusServerSocket_handleDeserializedStruct(int fd, ipc_operationIdentifier operationId, void *buffer);
void cpusServerSocket_handleNewConnection(int fd);
void cpusServerSocket_handleDisconnection(int fd);

void *scheduler_mainFunction(void);
void *dispatcher_mainFunction(void);
void *configurationWatcherThread_mainFunction();

void semaphoreDidBlockProcess(t_PCB *pcb, char *identifier);
void semaphoreDidWakeProcess(t_PCB *pcb, char *identifier);

void executeNewProgram(t_PCB *pcb);
t_PCB *createPCBFromScript(char *script);
void markCPUAsFree(int fd);
void finishProgram(int pid, int exitCode);

#endif /* KERNEL_H_ */
