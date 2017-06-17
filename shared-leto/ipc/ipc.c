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
		log_debug(logger, "incoming data handler. Operation ID: %d", header.operationIdentifier);
		int count;

		switch (header.operationIdentifier) {
			case HANDSHAKE: {
				ipc_struct_handshake *handshake = malloc(sizeof(ipc_struct_handshake));
				count = recv(fd, handshake, sizeof(ipc_struct_handshake), 0);
				deserializedStructHandler(fd, header.operationIdentifier, handshake);
				break;
			}
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
				break;
			}
			case PROGRAM_FINISH: {
				ipc_struct_program_finish *programFinish = malloc(sizeof(ipc_struct_program_finish));
				ipc_header *header = malloc(sizeof(ipc_header));
				recv(fd, header, sizeof(ipc_header), 0);
				programFinish->header = *header;

				uint32_t pid;
				recv(fd, &pid, sizeof(uint32_t), 0);
				programFinish->pid = pid;

				break;
			}
		default:
			break;
		}
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

void ipc_client_sendStartProgram(int fd, uint32_t codeLength, void *code) {
	ipc_struct_program_start *programStart = malloc(sizeof(ipc_struct_program_start));

	programStart->header.operationIdentifier = PROGRAM_START;
	programStart->codeLength = codeLength;

	int headerPlusProgramLengthSize = sizeof(ipc_header) + sizeof(uint32_t);
	int totalSize = headerPlusProgramLengthSize + sizeof(char) * codeLength;
	void *buffer = malloc(totalSize);
	memcpy(buffer, programStart, headerPlusProgramLengthSize);
	memcpy(buffer + headerPlusProgramLengthSize, code, codeLength);
	send(fd, buffer, totalSize, 0);
}

void ipc_sendStartProgramResponse(int fd, uint32_t pid) {
	ipc_struct_program_start_response response = { {PROGRAM_START_RESPONSE}, pid };

	send(fd, &response, sizeof(response), 0);
}

ipc_struct_program_start_response *ipc_client_receiveStartProgramResponse(int fd) {
	ipc_header *header = malloc(sizeof(ipc_header));
	ipc_struct_program_start_response *response = malloc(sizeof(ipc_struct_program_start_response));

	int count = recv(fd, header, sizeof(ipc_header), MSG_PEEK);
	if (count == sizeof(ipc_header) && header->operationIdentifier == PROGRAM_START_RESPONSE) {
		read(fd, response, sizeof(ipc_struct_program_start_response));
	} else {
		free(header);
		free(response);
		return NULL;
	}

	free(header);
	return response;
}

void ipc_client_sendFinishProgram(int fd, uint32_t pid) {
	ipc_struct_program_finish *programFinish = malloc(sizeof(ipc_struct_program_finish));

	programFinish->header.operationIdentifier = PROGRAM_FINISH;
	programFinish->pid = pid;

	int totalSize = sizeof(ipc_header) + sizeof(uint32_t);
	void *buffer = malloc(totalSize);
	memcpy(buffer, programFinish, totalSize);
	send(fd, buffer, totalSize, 0);
}

void ipc_client_requestNewPage(int fd, uint32_t pid) {
	ipc_struct_memory_new_page newPageRequest = { {PROGRAM_FINISH}, pid };

	send(fd, &newPageRequest, sizeof(ipc_struct_memory_new_page), 0);
}

ipc_struct_memory_new_page *ipc_memory_receiveNewPageRequest(int fd) {
	ipc_header *header = malloc(sizeof(ipc_header));
	ipc_struct_memory_new_page *response = malloc(sizeof(ipc_struct_memory_new_page));

	int count = recv(fd, header, sizeof(ipc_header), MSG_PEEK);
	if (count == sizeof(ipc_header) && header->operationIdentifier == MEMORY_NEW_PAGE) {
		read(fd, response, sizeof(ipc_struct_memory_new_page));
	} else {
		free(header);
		free(response);
		return NULL;
	}

	free(header);
	return response;
}
