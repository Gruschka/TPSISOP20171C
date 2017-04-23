/*
 * serialization.h
 *
 *  Created on: Apr 22, 2017
 *      Author: utnso
 */

#ifndef SERIALIZATION_H_
#define SERIALIZATION_H_

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

typedef struct handshake {
	ipc_processIdentifier processIdentifier;
} __attribute__((packed)) ipc_struct_handshake;

ipc_struct_handshake *ipc_deserialize_handshake(void *buffer);

#endif /* SERIALIZATION_H_ */
