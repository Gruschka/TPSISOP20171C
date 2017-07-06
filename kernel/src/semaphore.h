/*
 * semaphore.h
 *
 *  Created on: Jul 2, 2017
 *      Author: Hernan Canzonetta
 */

#ifndef SEMAPHORE_H_
#define SEMAPHORE_H_

#include <semaphore.h>

#include <pcb/pcb.h>
#include <commons/collections/queue.h>

typedef void (*SemaphoreDidBlockProcessFunction)(t_PCB *pcb);
typedef void (*SemaphoreDidWakeupProcessFunction)(t_PCB *pcb);

typedef struct kernel_semaphore {
	char *identifier;
	int count;
	t_queue *__waitList;
} kernel_semaphore;

void kernel_semaphores_init(
		SemaphoreDidBlockProcessFunction semaphoreDidBlockProcessFunction,
		SemaphoreDidWakeupProcessFunction semaphoreDidWakeupProcessFunction);

kernel_semaphore *kernel_semaphore_make(char *identifier, int value);
int kernel_semaphore_destroy(kernel_semaphore *semaphore);
int kernel_semaphore_wait(kernel_semaphore *semaphore, t_PCB *pcb);
int kernel_semaphore_signal(kernel_semaphore *semaphore, t_PCB *pcb);

#endif /* SEMAPHORE_H_ */
