/*
 * semaphore.c
 *
 *  Created on: Jul 2, 2017
 *      Author: Hernan Canzonetta
 */

#include "semaphore.h"

#include <stdlib.h>

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
	_semaphoreDidBlockProcessFunction(pcb);

	return semaphore->count--;
}

int kernel_semaphore_signal(kernel_semaphore *semaphore, t_PCB *pcb) {
	if (semaphore->count == 0) {
		// aca tengo que desbloquear 1 proceso de la cola
		t_PCB *pcb = queue_pop(semaphore->__waitList);
		_semaphoreDidWakeupProcessFunction(pcb);
		return semaphore->count++;
	}

	return 0;
}

kernel_semaphore *kernel_semaphore_make(char *identifier, int value) {
	kernel_semaphore *semaphore = malloc(sizeof(kernel_semaphore));

	semaphore->count = value;
	semaphore->identifier = identifier;
	semaphore->__waitList = queue_create();
	sem_init(&(semaphore->__sem), 0, 0);

	return semaphore;
}
