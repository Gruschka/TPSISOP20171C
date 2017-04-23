/*
 * serialization.c
 *
 *  Created on: Apr 22, 2017
 *      Author: utnso
 */
#include "serialization.h"

ipc_struct_handshake *ipc_deserialize_handshake(void *buffer) {
	ipc_struct_handshake *handshake = malloc(sizeof(ipc_struct_handshake));
	memcpy(handshake, buffer, sizeof(ipc_struct_handshake));
	return handshake;
}
