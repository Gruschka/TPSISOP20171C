#include "ipc.h"
#include "sockets.h"
#include <stdlib.h>
#include <unistd.h>
#include <commons/log.h>


extern t_log *logger;

int ipc_createServer(char *port, EpollConnectionEventHandler newConnectionHandler, EpollDisconnectionEventHandler disconnectionHandler, EpollDeserializedStructEventHandler deserializedStructHandler) {
	void incomingDataHandler(int fd, ipc_header header) {
		log_info(logger, "incoming data handler");

		switch (header.operationIdentifier) {
			case HANDSHAKE: {
				ipc_struct_handshake *handshake = malloc(sizeof(ipc_struct_handshake));
				read(fd, handshake, sizeof(ipc_struct_handshake));
				deserializedStructHandler(fd, header.operationIdentifier, handshake);
			}
			break;
		default:
				break;
		}
	}

	return createServer(port, newConnectionHandler, incomingDataHandler, disconnectionHandler);
}
