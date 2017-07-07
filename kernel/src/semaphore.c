/*
 * semaphore.c
 *
 *  Created on: Jul 2, 2017
 *      Author: Hernan Canzonetta
 */

#include "semaphore.h"

#include <stdlib.h>
#include <commons/log.h>

extern t_log *logger;

SemaphoreDidBlockProcessFunction _semaphoreDidBlockProcessFunction;
SemaphoreDidWakeupProcessFunction _semaphoreDidWakeupProcessFunction;

void kernel_semaphores_init(
		SemaphoreDidBlockProcessFunction semaphoreDidBlockProcessFunction,
		SemaphoreDidWakeupProcessFunction semaphoreDidWakeupProcessFunction) {
	_semaphoreDidBlockProcessFunction = semaphoreDidBlockProcessFunction;
	_semaphoreDidWakeupProcessFunction = semaphoreDidWakeupProcessFunction;
}

int kernel_semaphore_wait(kernel_semaphore *semaphore, t_PCB *pcb) {
	if (semaphore->count > 0) return semaphore->count--;

	queue_push(semaphore->__waitList, pcb);
	_semaphoreDidBlockProcessFunction(pcb, semaphore->identifier);

	return semaphore->count;
}

int kernel_semaphore_signal(kernel_semaphore *semaphore, t_PCB *pcb) {
	if (semaphore->count == 0) {
		t_PCB *unblockedPCB = queue_pop(semaphore->__waitList);

		if (unblockedPCB != NULL) {
			_semaphoreDidWakeupProcessFunction(unblockedPCB, semaphore->identifier);
			return semaphore->count;
		}

		return semaphore->count++;
	}

	return 0;
}

kernel_semaphore *kernel_semaphore_make(char *identifier, int value) {
	kernel_semaphore *semaphore = malloc(sizeof(kernel_semaphore));

	semaphore->count = value;
	semaphore->identifier = identifier;
	semaphore->__waitList = queue_create();

	return semaphore;
}

int kernel_semaphore_destroy(kernel_semaphore *semaphore){
	semaphore->count = 0;
	if (semaphore->identifier) free(semaphore->identifier);
	int queueIterator = 0;
	// TODO: implement queue_clean_and_destroy_elements(queue,function pointer)
	// with pcb_destroy(pcb)
	while (queueIterator < queue_size(semaphore->__waitList)){
		t_PCB* pcb = queue_pop(semaphore->__waitList);
		free(pcb);
		queueIterator++;
	}
	return EXIT_SUCCESS;
}

void dump_semaphore(kernel_semaphore *sem) {
	log_debug(logger, "[semaphore-dump] identifier: %s. count: %d. waiting q count: %d", sem->identifier, sem->count, queue_size(sem->__waitList));
}
