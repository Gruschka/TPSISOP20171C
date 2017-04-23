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
#include <commons/config.h>
#include <commons/log.h>

//"""Private"""
static t_config *__config;

//Globals
t_kernel_config *configuration;
t_log *logger;

int main(int argc, char **argv) {
	char *logFile = tmpnam(NULL);
	logger = log_create(logFile, "KERNEL", 1, LOG_LEVEL_DEBUG);
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
			log_info(logger, "Handshake received. Process identifier: %d", handshake->processIdentifier);
			ipc_server_sendHandshakeResponse(fd, 1);
			break;
		}
		case PROGRAM_START: {
			ipc_struct_program_start *programStart = buffer;
			log_info(logger, "Program received. Code length: %d. Code: %s", programStart->codeLength, programStart->code);
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
