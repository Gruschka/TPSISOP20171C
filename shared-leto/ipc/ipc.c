#include "ipc.h"
#include "sockets.h"
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <commons/log.h>
#include <string.h>

extern t_log *logger;

int ipc_createServer(char *port,
		EpollConnectionEventHandler newConnectionHandler,
		EpollDisconnectionEventHandler disconnectionHandler,
		EpollDeserializedStructEventHandler deserializedStructHandler) {
	void incomingDataHandler(int fd, ipc_header header) {
		log_debug(logger, "incoming data handler. Operation ID: %d",
				header.operationIdentifier);
		int count;

		switch (header.operationIdentifier) {
		case HANDSHAKE: {
			ipc_struct_handshake *handshake = malloc(
					sizeof(ipc_struct_handshake));
			count = recv(fd, handshake, sizeof(ipc_struct_handshake), 0);
			deserializedStructHandler(fd, header.operationIdentifier,
					handshake);
			break;
		}
		case PROGRAM_START: {
			ipc_struct_program_start *programStart = malloc(
					sizeof(ipc_struct_program_start));
			ipc_header *header = malloc(sizeof(ipc_header));
			recv(fd, header, sizeof(ipc_header), 0);
			programStart->header = *header;

			uint32_t codeLength;
			recv(fd, &codeLength, sizeof(uint32_t), 0);
			programStart->codeLength = codeLength;

			void *codeBuffer = malloc(sizeof(char) * codeLength);
			recv(fd, codeBuffer, codeLength, 0);
			programStart->code = codeBuffer;

			deserializedStructHandler(fd, header->operationIdentifier,
					programStart);
			break;
		}
		case PROGRAM_FINISH: {
			ipc_struct_program_finish *programFinish = malloc(
					sizeof(ipc_struct_program_finish));
			ipc_header *header = malloc(sizeof(ipc_header));
			recv(fd, header, sizeof(ipc_header), 0);
			programFinish->header = *header;

			uint32_t pid;
			recv(fd, &pid, sizeof(uint32_t), 0);
			programFinish->pid = pid;

			deserializedStructHandler(fd, header->operationIdentifier,
					programFinish);

			break;
		}
		case GET_SHARED_VARIABLE: {
			ipc_struct_get_shared_variable *request = malloc(
					sizeof(ipc_struct_get_shared_variable));
			ipc_header *header = malloc(sizeof(ipc_header));
			recv(fd, header, sizeof(ipc_header), 0);

			request->header = *header;

			uint32_t identifierLength;
			recv(fd, &identifierLength, sizeof(uint32_t), 0);

			char *identifierBuf = malloc(identifierLength + 1);
			recv(fd, identifierBuf, identifierLength + 1, 0);

			request->identifier = identifierBuf;
			request->identifierLength = identifierLength;

			deserializedStructHandler(fd, header->operationIdentifier, request);

			break;
		}
		case KERNEL_SEMAPHORE_WAIT: {
			ipc_header header;
			recv(fd, &header, sizeof(ipc_header), 0);

			int identifierLength;
			recv(fd, &identifierLength, sizeof(int), 0);

			char *identifier = malloc(identifierLength);
			recv(fd, identifier, identifierLength, 0);

			t_PCBVariableSize variableSize;
			recv(fd, &variableSize, sizeof(t_PCBVariableSize), MSG_PEEK);

			int bufferSize = pcb_getBufferSizeFromVariableSize(&variableSize);
			void *pcbBuffer = malloc(bufferSize);
			recv(fd, pcbBuffer, bufferSize, 0);

			ipc_struct_kernel_semaphore_wait *semaphoreWait = malloc(
					sizeof(ipc_struct_kernel_semaphore_wait));
			semaphoreWait->header = header;
			semaphoreWait->identifierLength = identifierLength;
			semaphoreWait->identifier = identifier;
			semaphoreWait->pcb = pcb_deSerializePCB(pcbBuffer, &variableSize);

			deserializedStructHandler(fd,
					semaphoreWait->header.operationIdentifier, semaphoreWait);
			break;
		}
		case KERNEL_SEMAPHORE_SIGNAL: {
			ipc_header header;
			recv(fd, &header, sizeof(ipc_header), 0);

			int identifierLength;
			recv(fd, &identifierLength, sizeof(int), 0);

			char *identifier = malloc(identifierLength);
			recv(fd, identifier, identifierLength, 0);

			ipc_struct_kernel_semaphore_signal *signal = malloc(
					sizeof(ipc_struct_kernel_semaphore_signal));
			signal->header.operationIdentifier = KERNEL_SEMAPHORE_SIGNAL;
			signal->identifier = identifier;
			signal->identifierLength = identifierLength;

			deserializedStructHandler(fd, signal->header.operationIdentifier,
					signal);
			break;
		}
		case KERNEL_ALLOC_HEAP: {
			ipc_struct_kernel_alloc_heap *alloc = malloc(
					sizeof(ipc_struct_kernel_alloc_heap));
			recv(fd, alloc, sizeof(ipc_struct_kernel_alloc_heap), 0);

			deserializedStructHandler(fd, alloc->header.operationIdentifier,
					alloc);
			break;
		}
		case KERNEL_OPEN_FILE: {
			ipc_struct_kernel_open_file *openFile = malloc(
					sizeof(ipc_struct_kernel_open_file));
			ipc_header *header = malloc(sizeof(ipc_header));
			recv(fd, header, sizeof(ipc_header), 0);

			int pathLength;
			recv(fd, &pathLength, sizeof(int), 0);

			char *path = malloc(pathLength);
			recv(fd, path, pathLength, 0);

			int read;
			int write;
			int creation;
			recv(fd, &read, sizeof(int), 0);
			recv(fd, &write, sizeof(int), 0);
			recv(fd, &creation, sizeof(int), 0);

			openFile->creation = creation;
			openFile->read = read;
			openFile->write = write;
			openFile->header = *header;
			openFile->path = path;
			openFile->pathLength = pathLength;

			deserializedStructHandler(fd, openFile->header.operationIdentifier,
					openFile);
			break;
		}
		case KERNEL_READ_FILE: {
			ipc_struct_kernel_read_file *readFile = malloc(
					sizeof(ipc_struct_kernel_read_file));

			recv(fd, readFile, sizeof(ipc_struct_kernel_read_file), 0);

			deserializedStructHandler(fd, readFile->header.operationIdentifier,
					readFile);
			break;
		}
		case KERNEL_WRITE_FILE: {
			ipc_struct_kernel_write_file *writeFile = malloc(sizeof(ipc_struct_kernel_write_file));

			ipc_header *header = malloc(sizeof(ipc_header));
			recv(fd, header, sizeof(ipc_header), 0);

			int fileDescriptor;
			recv(fd, &fileDescriptor, sizeof(int), 0);

			int size;
			recv(fd, &size, sizeof(int), 0);

			void *buffer = malloc(size);
			recv(fd, buffer, size, 0);

			writeFile->header = *header;
			writeFile->size = size;
			writeFile->fileDescriptor = fileDescriptor;
			writeFile->buffer = buffer;

			deserializedStructHandler(fd, writeFile->header.operationIdentifier, writeFile);
			break;
		}
		case KERNEL_MOVE_FILE_CURSOR: {
			ipc_struct_kernel_move_file_cursor *moveFileCursor = malloc(
					sizeof(ipc_struct_kernel_read_file));

			recv(fd, moveFileCursor, sizeof(ipc_struct_kernel_move_file_cursor),
					0);

			deserializedStructHandler(fd,
					moveFileCursor->header.operationIdentifier, moveFileCursor);
			break;
		}
		default:
			break;
		}
	}

	return createServer(port, newConnectionHandler, incomingDataHandler,
			disconnectionHandler);
}

void ipc_client_sendHandshake(ipc_processIdentifier processIdentifier, int fd) {
	ipc_struct_handshake *handshake = malloc(sizeof(ipc_struct_handshake));

	handshake->processIdentifier = processIdentifier;
	handshake->header.operationIdentifier = HANDSHAKE;

	send(fd, handshake, sizeof(ipc_struct_handshake), 0);
	free(handshake);
}

void ipc_server_sendHandshakeResponse(int fd, char success, uint32_t info) {
	ipc_struct_handshake_response *response = malloc(
			sizeof(ipc_struct_handshake_response));

	response->header.operationIdentifier = HANDSHAKE_RESPONSE;
	response->success = success;
	response->info = info;

	send(fd, response, sizeof(ipc_struct_handshake_response), 0);
	free(response);
}

ipc_struct_handshake_response *ipc_client_waitHandshakeResponse(int fd) {
	//TODO: (Hernán) mover la serialización y ser menos villero
	ipc_header *header = malloc(sizeof(ipc_header));
	ipc_struct_handshake_response *response = malloc(
			sizeof(ipc_struct_handshake_response));

	int count = recv(fd, header, sizeof(ipc_header), MSG_PEEK);
	if (count == sizeof(ipc_header)
			&& header->operationIdentifier == HANDSHAKE_RESPONSE) {
		read(fd, response, sizeof(ipc_struct_handshake_response));
	} else {
		free(header);
		free(response);
		return NULL ;
	}

	free(header);
	return response;
}

void ipc_client_sendStartProgram(int fd, uint32_t codeLength, void *code) {
	ipc_struct_program_start *programStart = malloc(
			sizeof(ipc_struct_program_start));

	programStart->header.operationIdentifier = PROGRAM_START;
	programStart->codeLength = codeLength;

	int headerPlusProgramLengthSize = sizeof(ipc_header) + sizeof(uint32_t);
	int totalSize = headerPlusProgramLengthSize + sizeof(char) * codeLength;
	void *buffer = malloc(totalSize);
	memcpy(buffer, programStart, headerPlusProgramLengthSize);
	memcpy(buffer + headerPlusProgramLengthSize, code, codeLength);
	send(fd, buffer, totalSize, 0);

	free(programStart);
	free(buffer);
}

void ipc_sendStartProgramResponse(int fd, uint32_t pid) {
	ipc_struct_program_start_response response = { { PROGRAM_START_RESPONSE },
			pid };

	send(fd, &response, sizeof(response), 0);
}

ipc_struct_program_start_response *ipc_client_receiveStartProgramResponse(
		int fd) {
	ipc_header *header = malloc(sizeof(ipc_header));
	ipc_struct_program_start_response *response = malloc(
			sizeof(ipc_struct_program_start_response));

	int count = recv(fd, header, sizeof(ipc_header), MSG_PEEK);
	if (count == sizeof(ipc_header)
			&& header->operationIdentifier == PROGRAM_START_RESPONSE) {
		read(fd, response, sizeof(ipc_struct_program_start_response));
	} else {
		free(header);
		free(response);
		return NULL ;
	}

	free(header);
	return response;
}

void ipc_client_sendFinishProgram(int fd, uint32_t pid) {
	ipc_struct_program_finish *programFinish = malloc(
			sizeof(ipc_struct_program_finish));

	programFinish->header.operationIdentifier = PROGRAM_FINISH;
	programFinish->pid = pid;

	int totalSize = sizeof(ipc_header) + sizeof(uint32_t);
	void *buffer = malloc(totalSize);
	memcpy(buffer, programFinish, totalSize);
	send(fd, buffer, totalSize, 0);

	free(programFinish);
	free(buffer);
}

void ipc_client_sendGetSharedVariable(int fd, char *identifier) {
	ipc_struct_get_shared_variable *request = malloc(
			sizeof(ipc_struct_get_shared_variable));

	request->header.operationIdentifier = GET_SHARED_VARIABLE;
	request->identifier = identifier;

	int totalSize = sizeof(ipc_header) + sizeof(uint32_t) + strlen(identifier)
			+ 1;
	int identifierLength = strlen(identifier);
	void *buffer = malloc(totalSize);
	memcpy(buffer, &(request->header), sizeof(ipc_header));
	memcpy(buffer + sizeof(ipc_header), &identifierLength, sizeof(uint32_t));
	memcpy(buffer + sizeof(ipc_header) + sizeof(uint32_t), request->identifier,
			strlen(identifier) + 1);
	send(fd, buffer, totalSize, 0);

	free(request);
	free(buffer);
}

int ipc_client_waitSharedVariableValue(int fd) {
	ipc_struct_get_shared_variable_response response;

	recv(fd, &response, sizeof(ipc_struct_get_shared_variable_response), 0);
	return response.value;
}

void ipc_client_sendSetSharedVariableValue(int fd, char *identifier, int value) {
	ipc_header header;
	header.operationIdentifier = SET_SHARED_VARIABLE;

	int totalSize = sizeof(ipc_header) + 2 * sizeof(int) + strlen(identifier)
			+ 1;
	int identifierLength = strlen(identifier);
	void *buffer = malloc(totalSize);
	memcpy(buffer, &header, sizeof(ipc_header));
	memcpy(buffer + sizeof(ipc_header), &value, sizeof(int));
	memcpy(buffer + sizeof(ipc_header) + sizeof(int), &identifierLength,
			sizeof(uint32_t));
	memcpy(buffer + sizeof(ipc_header) + sizeof(int) + sizeof(uint32_t),
			identifier, strlen(identifier) + 1);
	send(fd, buffer, totalSize, 0);

	free(buffer);
}

int ipc_client_waitSetSharedVariableValueResponse(int fd) {
	ipc_struct_set_shared_variable_response response;
	recv(fd, &response, sizeof(ipc_struct_set_shared_variable_response), 0);
	return response.value;
}

void ipc_client_sendMemoryRead(int fd, int pid, int pageNumber, int offset,
		int size) {
	ipc_struct_memory_read memoryRead;

	memoryRead.header.operationIdentifier = MEMORY_READ;

	memoryRead.pid = pid;
	memoryRead.pageNumber = pageNumber;
	memoryRead.offset = offset;
	memoryRead.size = size;

	send(fd, &memoryRead, sizeof(ipc_struct_memory_read), 0);
}

ipc_struct_memory_read_response *ipc_client_waitMemoryReadResponse(int fd) {
	ipc_struct_memory_read_response *response = malloc(
			sizeof(ipc_struct_memory_read_response));
	ipc_header header;

	recv(fd, &header, sizeof(ipc_header), MSG_PEEK);
	if (header.operationIdentifier != MEMORY_READ_RESPONSE) {
		return NULL ;
	}

	recv(fd, &header, sizeof(ipc_header), 0);

	recv(fd, &(response->success), sizeof(char), 0);
	recv(fd, &(response->size), sizeof(int), 0);

	void *buffer = malloc(response->size);
	recv(fd, buffer, response->size, 0);
	response->buffer = buffer;

	return response;
}

void ipc_client_sendMemoryWrite(int fd, int pid, int pageNumber, int offset,
		int size, void *buffer) {
	ipc_header header;

	header.operationIdentifier = MEMORY_WRITE;

	int totalSize = sizeof(ipc_header) + sizeof(int) * 4 + size;
	void *buf = malloc(totalSize);

	memcpy(buf, &header, sizeof(ipc_header));
	memcpy(buf + sizeof(ipc_header), &pid, sizeof(int));
	memcpy(buf + sizeof(ipc_header) + sizeof(int), &pageNumber, sizeof(int));
	memcpy(buf + sizeof(ipc_header) + sizeof(int) + sizeof(int), &offset,
			sizeof(int));
	memcpy(buf + sizeof(ipc_header) + sizeof(int) + sizeof(int) + sizeof(int),
			&size, sizeof(int));
	//pid pagenumber offset size
	memcpy(
			buf + sizeof(ipc_header) + sizeof(int) + sizeof(int) + sizeof(int)
					+ sizeof(int), buffer, size);

	send(fd, buf, totalSize, 0);
}
