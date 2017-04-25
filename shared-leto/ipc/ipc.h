/*
 * ipc.h
 *
 *  Created on: Apr 9, 2017
 *      Author: utnso
 */

#ifndef IPC_H_
#define IPC_H_

#include "sockets.h"
#include "serialization.h"

typedef void (*EpollDeserializedStructEventHandler)(int fd, ipc_operationIdentifier operationIdentifier, void *buffer);

// Client
void ipc_client_sendHandshake(ipc_processIdentifier processIdentifier, int fd);
ipc_struct_handshake_response *ipc_client_waitHandshakeResponse(int fd);
void ipc_client_sendStartProgram(int fd, uint32_t codeLength, void *code);

// Server
int ipc_createServer(char *port, EpollConnectionEventHandler newConnectionHandler, EpollDisconnectionEventHandler disconnectionHandler, EpollDeserializedStructEventHandler deserializedStructHandler);
void ipc_server_sendHandshakeResponse(int fd, char success);


#endif /* IPC_H_ */
