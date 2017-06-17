/*
 ============================================================================
 Name        : kernel.c
 Author      : Hernan Canzonetta
 Version     :
 Copyright   : Copyright
 Description : Hello World in C, Ansi-style
 ============================================================================
 */

#include "kernel.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#include "memory.h"
#include "scheduling.h"
#include <commons/config.h>
#include <commons/log.h>

//"""Private"""
static t_config *__config;

//Globals
t_kernel_config *configuration;
t_log *logger;

pthread_t serverThread;
pthread_t schedulerThread;
pthread_t dispatcherThread;

uint32_t lastPID = 1;
uint32_t pageSize = 512;

void testMemory() {
	void *page = memory_createPage(pageSize);
//	void *block = memory_addBlock(page, 50);
	void *firstBlock = memory_addBlock(page, 200);
	void *secondBlock = memory_addBlock(page, 35);
	void *thirdBlock = memory_addBlock(page, 157);
	void *fourthBlock = memory_addBlock(page, 43);

	log_debug(logger, "fourthBlock: %p", fourthBlock);

	memory_dumpPage(page);
}

int main(int argc, char **argv) {
	char *logFile = tmpnam(NULL);

#ifdef DEBUG
	logger = log_create(logFile, "KERNEL", 1, LOG_LEVEL_DEBUG);
#else
	logger = log_create(logFile, "KERNEL", 1, LOG_LEVEL_INFO);
#endif

	log_info(logger, "Comenzó la ejecución");
	log_info(logger, "Log file: %s", logFile);

	if (argc < 2) {
		log_error(logger, "Falta pasar la ruta del archivo de configuracion");
		return EXIT_FAILURE;
	} else {
		__config = config_create(argv[1]);
		configuration = malloc(sizeof(t_kernel_config));
	}

	fetchConfiguration();

	newQueue = newQueue_create();
	readyQueue = readyQueue_create();
	execList = execList_create();

	testMemory();

	int i;
	for (i = 0; i < configuration->multiprogrammingDegree; i++) {
		sem_post(&readyQueue_availableSpaces);
	}

	pthread_create(&serverThread, NULL, consolesServer_main, NULL);
	pthread_create(&schedulerThread, NULL, (void *)scheduler_mainFunction, NULL);
	pthread_create(&dispatcherThread, NULL, (void *)dispatcher_mainFunction, NULL);

	pthread_join(serverThread, NULL);
	pthread_join(schedulerThread, NULL);
	pthread_join(dispatcherThread, NULL);

	return EXIT_SUCCESS;
}

void *consolesServer_main(void *args) {
	//TODO: Pasarle el puerto por archivo de config
	ipc_createServer("5000", consolesServerSocket_handleNewConnection, consolesServerSocket_handleDisconnection, consolesServerSocket_handleDeserializedStruct);
	return EXIT_SUCCESS;
}

void fetchConfiguration() {
	configuration->programsPort = config_get_int_value(__config, "PUERTO_PROG");
	configuration->cpuPort = config_get_int_value(__config, "PUERTO_CPU");
	configuration->memoryIP = config_get_string_value(__config, "IP_MEMORIA");
	configuration->memoryPort = config_get_int_value(__config, "PUERTO_MEMORIA");
	configuration->filesystemIP = config_get_string_value(__config, "IP_FS");
	configuration->filesystemPort = config_get_int_value(__config, "PUERTO_FS");
	configuration->quantum = config_get_int_value(__config, "QUANTUM");
	configuration->quantumSleep = config_get_int_value(__config, "QUANTUM_SLEEP");
	configuration->schedulingAlgorithm = ROUND_ROBIN; //TODO: Levantar string y crear enum a partir del valor
	configuration->multiprogrammingDegree = config_get_int_value(__config, "GRADO_MULTIPROG");
	configuration->stackSize = config_get_int_value(__config, "STACK_SIZE");
}

void consolesServerSocket_handleDeserializedStruct(int fd, ipc_operationIdentifier operationId, void *buffer) {
	switch (operationId) {
		case HANDSHAKE: {
			ipc_struct_handshake *handshake = buffer;
			log_info(logger, "Handshake received. Process identifier: %s", processName(handshake->processIdentifier));
			ipc_server_sendHandshakeResponse(fd, 1);
			break;
		}
		case PROGRAM_START: {
			ipc_struct_program_start *programStart = buffer;
			char *codeString = malloc(sizeof(char) * (programStart->codeLength + 1));
			memcpy(codeString, programStart->code, programStart->codeLength);
			codeString[programStart->codeLength] = '\0';
			log_info(logger, "Program received. Code length: %d. Code: %s", programStart->codeLength, codeString);
			t_PCB *newProgram = malloc(sizeof(t_PCB));
			newProgram->pid = ++lastPID;
			ipc_sendStartProgramResponse(fd, newProgram->pid);
			executeNewProgram(newProgram);
			break;
		}
		default:
			break;
	}
}

void consolesServerSocket_handleNewConnection(int fd) {
	log_info(logger, "New connection. fd: %d", fd);
}

void consolesServerSocket_handleDisconnection(int fd) {
	log_info(logger, "New disconnection. fd: %d", fd);
}

void *scheduler_mainFunction(void)
{
	while (1) {
		log_debug(logger, "[scheduler] waiting for programs in new q");
		sem_wait(&newQueue_programsCount);
		log_debug(logger, "[scheduler] waiting for spaces in ready q");
		sem_wait(&readyQueue_availableSpaces);
		pthread_mutex_lock(&readyQueue_mutex);
		pthread_mutex_lock(&newQueue_mutex);
			t_PCB *program = newQueue_popProcess();
			readyQueue_addProcess(program);
			log_debug(logger, "[scheduler] new process <PID: %d> in ready q", program->pid);
		pthread_mutex_unlock(&newQueue_mutex);
		pthread_mutex_unlock(&readyQueue_mutex);
		sem_post(&readyQueue_programsCount);
	}

	return 0;
}

void *dispatcher_mainFunction(void)
{
	while (1) {
		log_debug(logger, "[dispatcher] waiting for programs in ready q");
		sem_wait(&readyQueue_programsCount);
		log_debug(logger, "[dispatcher] waiting for free cpu's");
		sem_wait(&exec_spaces);
		pthread_mutex_lock(&execList_mutex);
		pthread_mutex_lock(&readyQueue_mutex);
			t_PCB *program = readyQueue_popProcess();
//			t_CPU *cpu = queue_pop(cpus libres);
//			cpu->program = program;
			//TODO: pasarle el pcb al cpu
			log_debug(logger, "[dispatcher] new process <PID:%d> in exec", program->pid);
		pthread_mutex_unlock(&readyQueue_mutex);
		pthread_mutex_unlock(&execList_mutex);
		sem_post(&exec_programs);
	}

	return 0;
}

void executeNewProgram(t_PCB *pcb) {
	pthread_mutex_lock(&newQueue_mutex);
	newQueue_addProcess(pcb);
	pthread_mutex_unlock(&newQueue_mutex);
	sem_post(&newQueue_programsCount);
}

