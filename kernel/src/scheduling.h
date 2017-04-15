/*
 * scheduling.h
 *
 *  Created on: Apr 6, 2017
 *      Author: utnso
 */

#ifndef SCHEDULING_H_
#define SCHEDULING_H_

#include <semaphore.h>

#include <commons/collections/list.h>
#include <commons/collections/queue.h>
#include <pcb/pcb.h>

typedef enum t_scheduling_algorithm {
	ROUND_ROBIN,
	FIFO
} t_scheduling_algorithm;

//Queues
t_queue *newQueue;
t_list *readyQueue;
t_queue *exitQueue;
t_queue *blockQueue;
t_list *execList;

//Mutexes
pthread_mutex_t newQueue_mutex;
pthread_mutex_t readyQueue_mutex;
pthread_mutex_t exitQueue_mutex;
pthread_mutex_t blockQueue_mutex;
pthread_mutex_t execList_mutex;

//Semaphores
sem_t newQueue_processCount;
sem_t readyQueue_processCount;
sem_t availableCPUs;

//Functions
t_queue *newQueue_create();
t_PCB *newQueue_popThread();
void newQueue_addProcess(t_PCB *process);

t_list *readyQueue_create();
void readyQueue_addProcess(t_PCB *process);
void readyQueue_dump();
t_PCB *readyQueue_popThread();

t_queue *exitQueue_create();
void exitQueue_addProcess(t_PCB *process);

t_queue *blockQueue_create();

t_list *execList_create();
void execList_addProcess(t_PCB *process);

#endif /* SCHEDULING_H_ */
