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
	sem_init(&newQueue_processCount, 0, 0);
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
	log_debug(logger,"newQueue_addProcess(<PID:%d>)", process->pid);
}

t_list *readyQueue_create()
{
	pthread_mutex_init(&readyQueue_mutex,NULL);
	sem_init(&readyQueue_processCount, 0, 0);
	readyQueue = list_create();
	return readyQueue;
}

t_list *execList_create()
{
	pthread_mutex_init(&execList_mutex,NULL);
	execList = list_create();

	return execList;
}

void execList_addProcess(t_PCB *process)
{
	log_debug(logger,"execList_addProcess(<PID:%d>)", process->pid);
	list_add(execList, process);
}

void readyQueue_addProcess(t_PCB *process)
{
	if (list_is_empty(readyQueue)) {
		list_add(readyQueue, process);
		log_debug(logger,"readyQueue_addProcess(<PID:%d>): Se agregó en la posición 0", process->pid);
	} else {
		int position = list_size(readyQueue);
		list_add_in_index(readyQueue, position, process);
		log_debug(logger,"readyQueue_addProcess(<PID:%d>): Se agregó en la posición %d", process->pid, position);
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
	log_debug(logger, "exitQueue_addProcess: Se agregó el proceso <PID:%d> a la cola EXIT",process->pid);
}

t_queue *blockQueue_create()
{
	pthread_mutex_init(&blockQueue_mutex,NULL);
	blockQueue = queue_create();
	return blockQueue;
}
