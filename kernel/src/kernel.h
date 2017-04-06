/*
 * kernel.h
 *
 *  Created on: Apr 6, 2017
 *      Author: utnso
 */

#ifndef KERNEL_H_
#define KERNEL_H_

#include "scheduling.h"

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

#endif /* KERNEL_H_ */
