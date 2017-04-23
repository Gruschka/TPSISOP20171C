/*
 * sockets.h
 *
 *  Created on: Apr 22, 2017
 *      Author: utnso
 */

#ifndef SOCKETS_H_
#define SOCKETS_H_

#include <stdint.h>
#include "serialization.h"

typedef struct header {
	uint32_t operationIdentifier;
} ipc_header;

typedef void (*EpollConnectionEventHandler)(int fd);
typedef EpollConnectionEventHandler EpollDisconnectionEventHandler;
typedef void (*EpollIncomingDataEventHandler)(int fd, ipc_header header);

int createServer(char *port, EpollConnectionEventHandler newConnectionHandler, EpollIncomingDataEventHandler incomingDataHandler, EpollDisconnectionEventHandler disconnectionHandler);

#endif /* SOCKETS_H_ */
