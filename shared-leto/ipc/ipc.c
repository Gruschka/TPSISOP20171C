#include "ipc.h"
#include "sockets.h"
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <commons/log.h>
#include <string.h>

extern t_log *logger;

int ipc_createServer(char *port, EpollConnectionEventHandler newConnectionHandler, EpollDisconnectionEventHandler disconnectionHandler, EpollDeserializedStructEventHandler deserializedStructHandler) {
	void incomingDataHandler(int fd, ipc_header header) {
		log_info(logger, "incoming data handler. Operation ID: %d", header.operationIdentifier);
		int count;

		switch (header.operationIdentifier) {
			case HANDSHAKE: {
				ipc_struct_handshake *handshake = malloc(sizeof(ipc_struct_handshake));
				count = read(fd, handshake, sizeof(ipc_struct_handshake));
				log_debug(logger, "Leí ipc_struct_handshake. size: %d", sizeof(ipc_struct_handshake));
				deserializedStructHandler(fd, header.operationIdentifier, handshake);
			}
			break;
		default:
			break;
		}

		do {
			/* Esto es porque no se estaba leyendo toda la data disponible en el fd
			 * entonces no se dispara el evento de desconexion (EOF) */
			void *buffer = malloc(sizeof(char));
			count = read(fd, buffer, sizeof(char));
			log_debug(logger, "Leí un char más: %c", buffer);
		} while (count > 0);
	}

	return createServer(port, newConnectionHandler, incomingDataHandler, disconnectionHandler);
}


void ipc_client_sendHandshake(ipc_processIdentifier processIdentifier, int fd) {
	ipc_struct_handshake *handshake = malloc(sizeof(ipc_struct_handshake));

	handshake->processIdentifier = processIdentifier;
	handshake->header.operationIdentifier = HANDSHAKE;

	log_debug(logger, "ipc_client_sendHandshake. size: %d", sizeof(ipc_struct_handshake));
	send(fd, handshake, sizeof(ipc_struct_handshake), 0);
	free(handshake);
}

void ipc_server_sendHandshakeResponse(int fd, char success) {
	ipc_struct_handshake_response *response = malloc(sizeof(ipc_struct_handshake_response));

	response->header.operationIdentifier = HANDSHAKE_RESPONSE;
	response->success = success;

	send(fd, response, sizeof(ipc_struct_handshake_response), 0);
	free(response);
}

ipc_struct_handshake_response *ipc_client_waitHandshakeResponse(int fd) {
	ipc_header *header = malloc(sizeof(ipc_header));
	ipc_struct_handshake_response *response = malloc(sizeof(ipc_struct_handshake_response));

	response->header = *header;

	int count = recv(fd, header, sizeof(ipc_header), MSG_PEEK);
	if (count == sizeof(ipc_header) && header->operationIdentifier == HANDSHAKE_RESPONSE) {
		log_debug(logger, "client: recibí una respuesta de handshake");
		void *responseBuffer = malloc(sizeof(ipc_struct_handshake_response));
		read(fd, responseBuffer, sizeof(ipc_struct_handshake_response));
		return responseBuffer;
	} else {
		free(header);
		free(response);
		return NULL;
	}

	free(header);
	return response;
}
