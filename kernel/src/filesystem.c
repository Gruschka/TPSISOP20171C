/*
 * filesystem.c
 *
 *  Created on: Jul 8, 2017
 *      Author: utnso
 */

#include "filesystem.h"

#include <stdlib.h>
#include <string.h>
#include <commons/log.h>

extern t_log *logger;

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

	fs_processFileTables = list_create();
}

int fs_openFile(int pid, char *path, char *permissionsString) {
	fs_permission_flags permissionFlags = permissions(permissionsString);
	fs_pft *pft = pft_findOrCreate(pid);

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
	log_debug(logger, "fs_readfile. path: %s", path);
	void *buffer = malloc(size);
	memset(buffer, '-', size);
	return buffer;
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

void fs_writeFile(int pid, int fd, int offset, int size, void *buffer) {
	// TODO: Escribir la data en el FS
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

	int idx = list_size(pft->entries);
	entry->fd = idx + 3; // los fd empiezan en 3
	list_add_in_index(pft->entries, idx, entry);

	return idx;
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
