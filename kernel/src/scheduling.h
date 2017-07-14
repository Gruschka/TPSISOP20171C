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
t_list *blockQueue;
t_list *execList;

//Mutexes
pthread_mutex_t newQueue_mutex;
pthread_mutex_t readyQueue_mutex;
pthread_mutex_t exitQueue_mutex;
pthread_mutex_t blockQueue_mutex;
pthread_mutex_t execList_mutex;

//Semaphores
sem_t newQueue_programsCount;
sem_t readyQueue_availableSpaces;
sem_t readyQueue_programsCount;
sem_t availableCPUs;
sem_t exec_spaces;
sem_t exec_programs;

//Functions
t_queue *newQueue_create();
t_PCB *newQueue_popProcess();
void newQueue_addProcess(t_PCB *process);

t_list *readyQueue_create();
void readyQueue_addProcess(t_PCB *process);
void readyQueue_dump();
t_PCB *readyQueue_popProcess();

t_queue *exitQueue_create();
void exitQueue_addProcess(t_PCB *process);

t_list *blockQueue_create();
void blockQueue_addProcess(t_PCB *pcb);
t_PCB *blockQueue_popProcess(int pid);

t_list *execList_create();
void execList_addProcess(t_PCB *process);

void dump_list(char *listName, t_list *list);
t_PCB *findPCB(t_list *list, int pid);
void removePCB(t_list *list, t_PCB *pcb);

#endif /* SCHEDULING_H_ */
