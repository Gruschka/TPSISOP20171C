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

int ipc_createServer(char *port, EpollConnectionEventHandler newConnectionHandler, EpollDisconnectionEventHandler disconnectionHandler, EpollDeserializedStructEventHandler deserializedStructHandler);

#endif /* IPC_H_ */
