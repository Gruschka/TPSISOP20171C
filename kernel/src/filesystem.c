/*
 * filesystem.c
 *
 *  Created on: Jul 8, 2017
 *      Author: utnso
 */

#include "filesystem.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <commons/log.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <netdb.h>

extern t_log *logger;
int fileSystem_sockfd;

int fs_connectToFileSystem() {
	struct sockaddr_in serv_addr;
	struct hostent *server;

	fileSystem_sockfd = socket(AF_INET, SOCK_STREAM, 0);

	if (fileSystem_sockfd < 0) {
		log_error(logger, "Error opening filesystem socket");
		return -1;
	}

	server = gethostbyname("127.0.0.1");
	bzero((char *) &serv_addr, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	bcopy((char *) server->h_addr,(char *)&serv_addr.sin_addr.s_addr, server->h_length);
	serv_addr.sin_port = htons(5004);


	if (connect(fileSystem_sockfd, (struct sockaddr*) &serv_addr, sizeof(serv_addr)) < 0) {
		log_error(logger, "Error connecting to fs");
		return -1;
	}

	ipc_client_sendHandshake(KERNEL, fileSystem_sockfd);
	ipc_struct_handshake_response *response = ipc_client_waitHandshakeResponse(fileSystem_sockfd);

	if (response->success == 0) {
		log_error(logger, "Error connecting to fs");
		return -1;
	}

	log_debug(logger, "[fileSystem] connected. block size: %d", response->info);

	free(response);
	return 0;
}

//TODO: Cuando agrego una entrada nueva a ambas estructuras, incrementar el anterior y no usar el indice para el fd

void testFS() {
	fs_init();

//	fs_permission_flags crwFlags = { .create = 1, .read = 1, .write = 1 };
//	fs_permission_flags rwFlags = { .create = 0, .read = 1, .write = 1 };
//	fs_permission_flags rFlags = { .create = 0, .read = 1, .write = 0 };

	fs_openFile(1, "/code/tp-sisop.sh", "r");
	fs_openFile(0, "/code/tp-sisop.sh", "rw");
	fs_openFile(0, "/dev/null", "rwc");
	fs_openFile(0, "/notas.txt", "r");
	fs_openFile(1, "/cursos/z1234/alumnos.zip", "r");

	log_debug(logger, "isOperationAllowed(pid: %d, fd: %d, operation: READ): %d", 1, 3, fs_isOperationAllowed(1, 3, READ));
	log_debug(logger, "isOperationAllowed(pid: %d, fd: %d, operation: WRITE): %d", 1, 3, fs_isOperationAllowed(1, 3, WRITE));
	log_debug(logger, "isOperationAllowed(pid: %d, fd: %d, operation: CREATE): %d", 1, 3, fs_isOperationAllowed(1, 3, CREATE));

	log_debug(logger, "isOperationAllowed(pid: %d, fd: %d, operation: READ): %d", 0, 3, fs_isOperationAllowed(0, 3, READ));
	log_debug(logger, "isOperationAllowed(pid: %d, fd: %d, operation: WRITE): %d", 0, 3, fs_isOperationAllowed(0, 3, WRITE));
	log_debug(logger, "isOperationAllowed(pid: %d, fd: %d, operation: CREATE): %d", 0, 3, fs_isOperationAllowed(0, 3, CREATE));

	log_debug(logger, "isOperationAllowed(pid: %d, fd: %d, operation: READ): %d", 0, 4, fs_isOperationAllowed(0, 4, READ));
	log_debug(logger, "isOperationAllowed(pid: %d, fd: %d, operation: WRITE): %d", 0, 4, fs_isOperationAllowed(0, 4, WRITE));
	log_debug(logger, "isOperationAllowed(pid: %d, fd: %d, operation: CREATE): %d", 0, 4, fs_isOperationAllowed(0, 4, CREATE));


	fs_globalFileTable_dump();
	fs_processFileTables_dump();

	fs_closeFile(0, 4);

	fs_globalFileTable_dump();
	fs_processFileTables_dump();

	void *buf = fs_readFile(0, 3, 0, 100);
	buf = fs_readFile(1, 4, 0, 20);
	log_debug(logger, "buf: %s", buf);
}

void fs_init() {
	fs_globalFileTable = malloc(sizeof(fs_gft));
	fs_globalFileTable->entries = list_create();
	pthread_mutex_init(&(fs_globalFileTable->mutex), 0);

	fs_connectToFileSystem();

	fs_processFileTables = list_create();
}

int fs_createFile(int pid, char *path){
	ipc_struct_fileSystem_create_file request;
	request.header.operationIdentifier = FILESYSTEM_CREATE_FILE;
	request.pathLength = strlen(path) +1;
	char *pathCopy = strdup(path);

	int bufferSize = sizeof(ipc_header) + sizeof(int) + request.pathLength;
	char *buffer = malloc(bufferSize);
	int bufferOffset = 0;
	memset(buffer,0,bufferSize);

	memcpy(buffer+bufferOffset,&request.header,sizeof(ipc_header));
	bufferOffset += sizeof(ipc_header);

	memcpy(buffer+bufferOffset,&request.pathLength,sizeof(int));
	bufferOffset += sizeof(int);

	memcpy(buffer+bufferOffset,pathCopy,request.pathLength);
	bufferOffset += sizeof(int);

	send(fileSystem_sockfd,buffer,bufferSize,0);

	free(pathCopy);
	ipc_struct_fileSystem_create_file_response response;
	recv(fileSystem_sockfd, &response, sizeof(ipc_struct_fileSystem_create_file_response), 0);


}


int fs_openFile(int pid, char *path, char *permissionsString) {
	fs_permission_flags permissionFlags = permissions(permissionsString);

	fs_pft *pft = pft_findOrCreate(pid);

	int bufferSize = sizeof(ipc_header) + sizeof(int) + strlen(path) +1 ;
	char *buffer = malloc(bufferSize);
	memset(buffer,0,bufferSize);
	int bufferOffset = 0;
	ipc_struct_fileSystem_validate_file *request = malloc(sizeof(ipc_struct_fileSystem_validate_file));

	request->header.operationIdentifier = FILESYSTEM_VALIDATE_FILE;
	request->pathLength = strlen(path)+1;

	char *pathCopy = strdup(path);

	memcpy(buffer+bufferOffset,&request->header,sizeof(ipc_header));
	bufferOffset += sizeof(ipc_header);

	ipc_header test;
	memcpy(&test,buffer,sizeof(ipc_header));

	memcpy(buffer+bufferOffset,&request->pathLength,sizeof(int));
	bufferOffset += sizeof(int);

	memcpy(buffer+bufferOffset,pathCopy,strlen(path)+1);
	bufferOffset += strlen(path)+1;

	send(fileSystem_sockfd, buffer, bufferSize, 0);

	free(pathCopy);
	free(request);
	free(buffer);

	ipc_struct_fileSystem_validate_file_response response;
	recv(fileSystem_sockfd, &response, sizeof(ipc_struct_fileSystem_validate_file_response), 0);

	if(response.status == EXIT_FAILURE){
		if(permissionFlags.create){
			fs_createFile(pid,path);
		}else{
			return -1;
		}
	}

	return pft_addEntry(pft, pid, path, permissionFlags);
}

int fs_isOperationAllowed(int pid, int fd, fs_operation operation) {
	fs_pft *table = pft_find(pid);

	fs_pft_entry *entry = list_get(table->entries, fd - 3);

	if (entry == NULL) return 0;

	switch (operation) {
		case READ: return entry->flags.read == 1;
		case WRITE: return entry->flags.write == 1;
		case CREATE: return entry->flags.create == 1;
	}

	return 0;
}

void *fs_readFile(int pid, int fd, int offset, int size) {
	// TODO: Pedirle la data al FS
	char *path = fs_getPath(fd, pid);
	fs_pft *pft = pft_find(pid);
	fs_pft_entry *entry = pft_findEntry(pft, fd);
	log_debug(logger, "fs_readfile. path: %s", path);

	ipc_struct_fileSystem_read_file request;
	request.header.operationIdentifier = FILESYSTEM_READ_FILE;
	request.pathLength = strlen(path) + 1;

	request.offset = entry->position + offset;
	request.size = size;

	int bufferSize = sizeof(ipc_header) + (3*sizeof(int)) + request.pathLength;
	char *buffer = malloc(bufferSize);

	send(fileSystem_sockfd,buffer,bufferSize,0);

	free(buffer);

	ipc_struct_fileSystem_read_file_response response;
	recv(fileSystem_sockfd, &response.header, sizeof(ipc_header), 0);

	recv(fileSystem_sockfd, &response.bufferSize, sizeof(int), 0);

	recv(fileSystem_sockfd, &response.buffer, response.bufferSize, 0);

	return response.buffer;
}

void fs_closeFile(int pid, int fd) {
	fs_pft *pft = pft_find(pid);
	int i = 0;

	for (i = 0; i < list_size(pft->entries); i++) {
		fs_pft_entry *entry = list_get(pft->entries, i);

		if (entry->fd == fd) {
			entry = list_remove(pft->entries, i);

			if (--(entry->gftEntry->open) == 0) {
				gft_removeEntry(entry->gftEntry);
			}

			return;
		}
	}
}

void fs_moveCursor(int pid, int fd, int offset) {
	fs_pft *pft = pft_find(pid);
	fs_pft_entry *entry = pft_findEntry(pft, fd);

	if (entry == NULL) return;
	entry->position = offset;
}

void fs_writeFile(int pid, int fd, int offset, int size, void *buffer) {
	char *path = fs_getPath(fd, pid);
	log_debug(logger, "fs_write. path: %s content: %s", path, (char *)buffer);
	fs_pft *pft = pft_find(pid);
	fs_pft_entry *entry = pft_findEntry(pft, fd);

	ipc_struct_fileSystem_write_file request;
	request.header.operationIdentifier = FILESYSTEM_WRITE_FILE;
	request.pathLength = strlen(path) + 1;
	request.offset = entry->position + offset;
	request.size = size;

	char *bufferCopy = malloc(size);
	memset(bufferCopy,0,size);

	memcpy(bufferCopy,buffer,size);


	int bufferSize = sizeof(ipc_header) + (3*sizeof(int)) + request.pathLength + size;
	int bufferOffset = 0;

	char *sendBuffer = malloc(bufferSize);

	memcpy(sendBuffer+bufferOffset,&request.header,sizeof(ipc_header));
	bufferOffset += sizeof(ipc_header);

	memcpy(sendBuffer+bufferOffset,&request.pathLength,sizeof(int));
	bufferOffset += sizeof(int);

	memcpy(sendBuffer+bufferOffset,path,request.pathLength);
	bufferOffset += request.pathLength;

	memcpy(sendBuffer+bufferOffset,&request.offset,sizeof(int));
	bufferOffset += sizeof(int);

	memcpy(sendBuffer+bufferOffset,&request.size,sizeof(int));
	bufferOffset += sizeof(int);

	memcpy(sendBuffer+bufferOffset,bufferCopy,size);
	bufferOffset += size;

	send(fileSystem_sockfd,sendBuffer,bufferSize,0);

	free(buffer);

	ipc_struct_fileSystem_write_file_response response;
	recv(fileSystem_sockfd, &response, sizeof(ipc_struct_fileSystem_write_file_response), 0);

}

// Debug

void fs_globalFileTable_dump() {
	int i;

	log_debug(logger, "Global File Table. # of entries: %d", list_size(fs_globalFileTable->entries));

	for (i = 0; i < list_size(fs_globalFileTable->entries); i++) {
		fs_gft_entry *entry = list_get(fs_globalFileTable->entries, i);
		log_debug(logger, "(%d) path: %s. open: %d", entry->_fd, entry->path, entry->open);
	}
}

void fs_processFileTables_dump() {
	int i;

	log_debug(logger, "Process File Tables. # of tables: %d", list_size(fs_processFileTables));

	for (i = 0; i < list_size(fs_processFileTables); i++) {
		fs_pft *pft = list_get(fs_processFileTables, i);

		log_debug(logger, "Process File Table. pid: %d", pft->pid);

		int j;
		for (j = 0; j < list_size(pft->entries); j++) {
			fs_pft_entry *entry = list_get(pft->entries, j);

			log_debug(logger, "-> fd: %d. flags: %d%d%d. (global fd: %d)", entry->fd, entry->flags.create, entry->flags.read, entry->flags.write, entry->gftEntry->_fd);
		}
	}
}

// Private

fs_pft *pft_findOrCreate(int pid) {
	fs_pft *pft = pft_find(pid);

	if (pft != NULL) return pft;

	pft = pft_make(pid);
	list_add(fs_processFileTables, pft);
	return pft;
}

fs_pft *pft_find(int pid) {
	int i;
	for (i = 0; i < list_size(fs_processFileTables); i++) {
		fs_pft *pft = list_get(fs_processFileTables, i);

		if (pft->pid == pid) return pft;
	}

	return NULL;
}

fs_pft *pft_make(int pid) {
	fs_pft *pft = malloc(sizeof(fs_pft));

	pft->pid = pid;
	pft->entries = list_create();
	pthread_mutex_init(&(pft->mutex), 0);

	return pft;
}

int pft_addEntry(fs_pft *pft, int pid, char *path, fs_permission_flags flags) {
	fs_pft_entry *entry = malloc(sizeof(fs_pft_entry));

	entry->flags = flags;
	entry->gftEntry = gft_findEntry(path);
	entry->gftEntry->open++;
	entry->position = 0;

	int idx = list_size(pft->entries);
	entry->fd = idx + 3; // los fd empiezan en 3
	list_add_in_index(pft->entries, idx, entry);

	return idx + 3;
}

fs_pft_entry *pft_findEntry(fs_pft *pft, int fd) {
	int i = 0;

	for (i = 0; i < list_size(pft->entries); i++) {
		fs_pft_entry *entry = list_get(pft->entries, i);

		if (entry->fd == fd) return entry;
	}

	return NULL;
}

fs_gft_entry *gft_findEntry(char *path) {
	int i = 0;

	for (i = 0; i < list_size(fs_globalFileTable->entries); i++) {
		fs_gft_entry *entry = list_get(fs_globalFileTable->entries, i);

		if (strcmp(entry->path, path) == 0) return entry;
	}

	return gft_addEntry(path);
}

fs_gft_entry *gft_addEntry(char *path) {
	fs_gft_entry *entry = malloc(sizeof(fs_gft_entry));

	entry->path = malloc(strlen(path) + 1);
	memcpy(entry->path, path, strlen(path) + 1);

	entry->open = 0;

	int entriesCount = list_size(fs_globalFileTable->entries);
	entry->_fd = entriesCount;
	list_add_in_index(fs_globalFileTable->entries, entriesCount, entry);
	return entry;
}

void fs_gft_entry_destroy(void *arg) {
	fs_gft_entry *entry = arg;
	free(entry->path);
	free(entry);
}

void gft_removeEntry(fs_gft_entry *entry) {
	log_debug(logger, "gft_removeEntry. entry->path = %s", entry->path);
	int idx = 0;

	for (idx = 0; idx < list_size(fs_globalFileTable->entries); idx++) {
		fs_gft_entry *e = list_get(fs_globalFileTable->entries, idx);

		if (e->_fd == entry->_fd) break;
	}

	list_remove_and_destroy_element(fs_globalFileTable->entries, idx, fs_gft_entry_destroy);
}

fs_permission_flags permissions(char *permissionsString) {
	fs_permission_flags flags = { .create = 0, .read = 0, .write = 0 };

	char *chr = permissionsString;
	while (chr != NULL && *chr != '\0') {
		switch (*chr) {
			case 'r':
				flags.read = 1;
				break;
			case 'w':
				flags.write = 1;
				break;
			case 'c':
				flags.create = 1;
				break;
		}
		++chr;
	}

	return flags;
}

char *fs_getPath(int fd, int pid) {
	fs_pft *pft = pft_find(pid);
	if (pft == NULL) return NULL;

	fs_pft_entry *entry = pft_findEntry(pft, fd);
	if (entry == NULL) return NULL;

	return entry->gftEntry->path;
}


