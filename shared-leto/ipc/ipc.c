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
				count = recv(fd, handshake, sizeof(ipc_struct_handshake), 0);
				log_debug(logger, "Leí ipc_struct_handshake. size: %d", sizeof(ipc_struct_handshake));
				deserializedStructHandler(fd, header.operationIdentifier, handshake);
			}
			break;
			case PROGRAM_START: {
				ipc_struct_program_start *programStart = malloc(sizeof(ipc_struct_program_start));
				ipc_header *header = malloc(sizeof(ipc_header));
				recv(fd, header, sizeof(ipc_header), 0);
				programStart->header = *header;

				uint32_t codeLength;
				recv(fd, &codeLength, sizeof(uint32_t), 0);
				programStart->codeLength = codeLength;

				void *codeBuffer = malloc(sizeof(char) * codeLength);
				recv(fd, codeBuffer, codeLength, 0);
				programStart->code = codeBuffer;

				deserializedStructHandler(fd, header->operationIdentifier, programStart);
			}
		default:
			break;
		}

//		do {
//			/* Esto es porque no se estaba leyendo toda la data disponible en el fd
//			 * entonces no se dispara el evento de desconexion (EOF) */
//			void *buffer = malloc(sizeof(char));
//			count = read(fd, buffer, sizeof(char));
//			log_debug(logger, "Leí un char más: %c", buffer);
//		} while (count > 0);
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
	//TODO: (Hernán) mover la serialización y ser menos villero
	ipc_header *header = malloc(sizeof(ipc_header));
	ipc_struct_handshake_response *response = malloc(sizeof(ipc_struct_handshake_response));

	int count = recv(fd, header, sizeof(ipc_header), MSG_PEEK);
	if (count == sizeof(ipc_header) && header->operationIdentifier == HANDSHAKE_RESPONSE) {
		read(fd, response, sizeof(ipc_struct_handshake_response));
	} else {
		free(header);
		free(response);
		return NULL;
	}

	free(header);
	return response;
}

void ipc_client_sendStartProgram(int fd, uint32_t codeLength, char *code) {
	ipc_struct_program_start *programStart = malloc(sizeof(ipc_struct_program_start));

	programStart->header.operationIdentifier = PROGRAM_START;
	programStart->codeLength = codeLength;
	programStart->code = malloc(sizeof(char) * codeLength);

	memcpy(programStart->code, code, codeLength);
	send(fd, programStart, sizeof(ipc_header) + sizeof(uint32_t) + codeLength, 0);
}
