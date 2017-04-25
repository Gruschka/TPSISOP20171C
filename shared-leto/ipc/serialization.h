/*
 * serialization.h
 *
 *  Created on: Apr 22, 2017
 *      Author: utnso
 */

#ifndef SERIALIZATION_H_
#define SERIALIZATION_H_
#include <stdint.h>

typedef struct header {
	uint32_t operationIdentifier;
} __attribute__((packed)) ipc_header;

typedef enum {
	HANDSHAKE,
	HANDSHAKE_RESPONSE,
	PROGRAM_START,
	PROGRAM_START_RESPONSE,
} ipc_operationIdentifier;

typedef enum {
	CONSOLE,
	KERNEL,
	MEMORY,
	FILESYSTEM,
	CPU,
} ipc_processIdentifier;
char *processName(ipc_processIdentifier processIdentifier);

typedef struct handshake {
	ipc_header header;
	ipc_processIdentifier processIdentifier;
} __attribute__((packed)) ipc_struct_handshake;

typedef struct handshake_response {
	ipc_header header;
	char success;
} __attribute__((packed)) ipc_struct_handshake_response;

typedef struct program_start {
	ipc_header header;
	uint32_t codeLength;
	void *code;
} __attribute__((packed)) ipc_struct_program_start;

typedef struct program_start_response {
	ipc_header header;
	int pid;
} __attribute__((packed)) ipc_struct_program_start_response;

ipc_struct_handshake *ipc_deserialize_handshake(void *buffer);

#endif /* SERIALIZATION_H_ */
