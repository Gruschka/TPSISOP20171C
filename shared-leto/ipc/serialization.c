/*
 * serialization.c
 *
 *  Created on: Apr 22, 2017
 *      Author: utnso
 */
#include "serialization.h"
#include <stdlib.h>
#include <string.h>

ipc_struct_handshake *ipc_deserialize_handshake(void *buffer) {
	ipc_struct_handshake *handshake = malloc(sizeof(ipc_struct_handshake));
	memcpy(handshake, buffer, sizeof(ipc_struct_handshake));
	return handshake;
}

char *processName(ipc_processIdentifier processIdentifier) {
	switch (processIdentifier) {
		case CONSOLE: return "Console"; break;
		case CPU: return "CPU"; break;
		case KERNEL: return "Kernel"; break;
		case MEMORY: return "Memory"; break;
		case FILESYSTEM: return "Filesystem"; break;
	}
	return NULL;
}
