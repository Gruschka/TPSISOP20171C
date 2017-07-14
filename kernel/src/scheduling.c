/*
 * scheduling.c
 *
 *  Created on: Apr 6, 2017
 *      Author: utnso
 */

#include "scheduling.h"

#include <pthread.h>
#include <commons/log.h>

//Logger definido en kernel.c
extern t_log *logger;

t_queue *newQueue_create()
{
	pthread_mutex_init(&newQueue_mutex, NULL);
	sem_init(&newQueue_programsCount, 0, 0);
	newQueue = queue_create();
	return newQueue;
}

t_PCB *newQueue_popProcess()
{
	t_PCB *pcb = queue_pop(newQueue);
	return pcb;
}

void newQueue_addProcess(t_PCB *process)
{
	queue_push(newQueue, process);
}

t_list *readyQueue_create()
{
	pthread_mutex_init(&readyQueue_mutex,NULL);
	sem_init(&readyQueue_availableSpaces, 0, 0);
	sem_init(&readyQueue_programsCount, 0, 0);
	readyQueue = list_create();
	return readyQueue;
}

t_list *execList_create()
{
	pthread_mutex_init(&execList_mutex,NULL);
	sem_init(&exec_spaces, 0, 0);
	sem_init(&exec_programs, 0, 0);
	execList = list_create();

	return execList;
}

void execList_addProcess(t_PCB *process)
{
	list_add(execList, process);
}

void readyQueue_addProcess(t_PCB *process)
{
	if (list_is_empty(readyQueue)) {
		list_add(readyQueue, process);
	} else {
		int position = list_size(readyQueue);
		list_add_in_index(readyQueue, position, process);
	}
}

t_PCB *readyQueue_popProcess()
{
	t_PCB *process = list_remove(readyQueue, list_size(readyQueue) - 1);

	return process;
}

void readyQueue_dump()
{
	log_debug(logger,"readyQueue_dump()");
	int index = 0;
	int listCount = list_size(readyQueue);
	while (index < listCount) {
		t_PCB *pcb = list_get(readyQueue, index);
		log_debug(logger,"readyQueue_dump: readyQueue[%d]: PID:%d", index, pcb->pid);
		index++;
	}
}

t_queue *exitQueue_create()
{
	pthread_mutex_init(&exitQueue_mutex,NULL);
	exitQueue = queue_create();
	return exitQueue;
}

void exitQueue_addProcess(t_PCB *process)
{
	queue_push(exitQueue, process);
}

t_list *blockQueue_create()
{
	pthread_mutex_init(&blockQueue_mutex,NULL);
	blockQueue = list_create();
	return blockQueue;
}

void blockQueue_addProcess(t_PCB *pcb) {
	list_add(blockQueue, pcb);
}

t_PCB *blockQueue_popProcess(int pid) {
	int i;

	for (i = 0; i < list_size(blockQueue); i++) {
		if (((t_PCB *)list_get(blockQueue, i))->pid == pid) {
			return list_remove(blockQueue, i);
		}
	}

	return NULL;
}

void dump_list(char *listName, t_list *list) {
	int i;

	if (list_size(list) == 0) {
		printf("[%s] no processes\n", listName);
		return;
	}

	for (i = 0; i < list_size(list); i++) {
		t_PCB *pcb = list_get(list, i);
		printf("[%s] process<PID: %d>", listName, pcb->pid);
	}
}

t_PCB *findPCB(t_list *list, int pid) {
	int i;

	for (i = 0; i < list_size(list); i++) {
		t_PCB *pcb = list_get(list, i);
		if (pcb->pid == pid) {
			return pcb;
		}
	}

	return NULL;
}

void removePCB(t_list *list, t_PCB *pcb) {
	int i;

	for (i = 0; i < list_size(list); i++) {
		t_PCB *pcbx = list_get(list, i);
		if (pcbx->pid == pcb->pid) break;
	}

	list_remove(list, i);
}
