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
	PROGRAM_FINISH,
	MEMORY_NEW_PAGE,
	MEMORY_NEW_PAGE_RESPONSE,
	GET_SHARED_VARIABLE,
	GET_SHARED_VARIABLE_RESPONSE,
	SET_SHARED_VARIABLE,
	SET_SHARED_VARIABLE_RESPONSE,
	PRINT_CONSOLE_MESSAGE
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
	uint32_t pid;
} __attribute__((packed)) ipc_struct_program_start_response;

typedef struct program_finish {
	ipc_header header;
	uint32_t pid;
} __attribute__((packed)) ipc_struct_program_finish;

typedef struct memory_new_page {
	ipc_header header;
	uint32_t pid;
} __attribute__((packed)) ipc_struct_memory_new_page;

typedef struct memory_new_page_response {
	ipc_header header;
	char success;
} __attribute__((packed)) ipc_struct_memory_new_page_response;

typedef struct get_shared_variable {
	ipc_header header;
	uint32_t identifierLength;
	char *identifier;
} __attribute__((packed)) ipc_struct_get_shared_variable;

typedef struct get_shared_variable_response {
	ipc_header header;
	int value;
} __attribute__((packed)) ipc_struct_get_shared_variable_response;

typedef struct set_shared_variable {
	ipc_header header;
	int value;
	int identifierLength;
	char *identifier;
} __attribute__((packed)) ipc_struct_set_shared_variable;

typedef struct set_shared_variable_response {
	ipc_header header;
	int value;
} __attribute__((packed)) ipc_struct_set_shared_variable_response;

ipc_struct_handshake *ipc_deserialize_handshake(void *buffer);

#endif /* SERIALIZATION_H_ */
