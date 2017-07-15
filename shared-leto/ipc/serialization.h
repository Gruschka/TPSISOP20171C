/*
 * serialization.h
 *
 *  Created on: Apr 22, 2017
 *      Author: utnso
 */

#ifndef SERIALIZATION_H_
#define SERIALIZATION_H_

#include <stdint.h>
#include "../pcb/pcb.h"

typedef struct header {
	uint32_t operationIdentifier;
} __attribute__((packed)) ipc_header;

typedef enum {
	HANDSHAKE,
	HANDSHAKE_RESPONSE,
	PROGRAM_START,
	PROGRAM_START_RESPONSE,
	PROGRAM_FINISH,
	GET_SHARED_VARIABLE,
	GET_SHARED_VARIABLE_RESPONSE,
	SET_SHARED_VARIABLE,
	SET_SHARED_VARIABLE_RESPONSE,
	PRINT_CONSOLE_MESSAGE,
	MEMORY_INIT_PROGRAM,
	MEMORY_INIT_PROGRAM_RESPONSE,
	MEMORY_READ,
	MEMORY_READ_RESPONSE,
	MEMORY_WRITE,
	MEMORY_WRITE_RESPONSE,
	MEMORY_REQUEST_MORE_PAGES,
	MEMORY_REQUEST_MORE_PAGES_RESPONSE,
	MEMORY_REMOVE_PAGE_FROM_PROGRAM,
	MEMORY_REMOVE_PAGE_FROM_PROGRAM_RESPONSE,
	MEMORY_DEINIT_PROGRAM,
	MEMORY_DEINIT_PROGRAM_RESPONSE,
	CPU_EXECUTE_PROGRAM,
	KERNEL_SEMAPHORE_WAIT,
	KERNEL_SEMAPHORE_WAIT_RESPONSE
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
	uint32_t info;
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

typedef struct memory_init_program {
	ipc_header header;
	int pid;
	int numberOfPages;
} __attribute__((packed)) ipc_struct_memory_init_program;

//FIXME cambiar esto para q me mande el pid y evitar que sea bloqueante la inicializacion
typedef struct memory_init_program_response {
	ipc_header header;
	char success;
} __attribute__((packed)) ipc_struct_memory_init_program_response;

typedef struct memory_read {
	ipc_header header;
	int pid;
	int pageNumber;
	int offset;
	int size;
} __attribute__((packed)) ipc_struct_memory_read;

typedef struct memory_read_program_response {
	ipc_header header;
	char success;
	int size;
	void *buffer;
} __attribute__((packed)) ipc_struct_memory_read_response;

typedef struct memory_write {
	ipc_header header;
	int pid;
	int pageNumber;
	int offset;
	int size;
	void *buffer;
} __attribute__((packed)) ipc_struct_memory_write;

typedef struct memory_write_program_response {
	ipc_header header;
	char success;
} __attribute__((packed)) ipc_struct_memory_write_response;

typedef struct memory_request_more_pages {
	ipc_header header;
	int pid;
	int numberOfPages;
} __attribute__((packed)) ipc_struct_memory_request_more_pages;

typedef struct memory_request_more_pages_response {
	ipc_header header;
	char success;
} __attribute__((packed)) ipc_struct_memory_request_more_pages_response;

typedef struct memory_remove_page_from_program {
	ipc_header header;
	int pid;
	int pageNumber;
} __attribute__((packed)) ipc_struct_memory_remove_page_from_program;

typedef struct memory_remove_page_from_program_response {
	ipc_header header;
	char success;
} __attribute__((packed)) ipc_struct_memory_remove_page_from_program_response;

typedef struct memory_deinit_program {
	ipc_header header;
	int pid;
} __attribute__((packed)) ipc_struct_memory_deinit_program;

typedef struct memory_deinit_program_response {
	ipc_header header;
	char success;
} __attribute__((packed)) ipc_struct_memory_deinit_program_response;

typedef struct kernel_semaphore_wait {
	ipc_header header;
	int identifierLength;
	char *identifier;
	int serializedLength;
	void *pcb;
} __attribute__((packed)) ipc_struct_kernel_semaphore_wait;

typedef struct kernel_semaphore_wait_response {
	ipc_header header;
	char shouldBlock;
} __attribute__((packed)) ipc_struct_kernel_semaphore_wait_response;

typedef struct cpu_execute_program {
	ipc_header header;
	int quantum;
	int serializedSize;
	void *serializedPCB;
} __attribute__((packed)) ipc_struct_cpu_execute_program;

ipc_struct_handshake *ipc_deserialize_handshake(void *buffer);

#endif /* SERIALIZATION_H_ */
