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

void testFS() {
	fs_init();

	fs_permission_flags crwFlags = { .create = 1, .read = 1, .write = 1 };
	fs_permission_flags rwFlags = { .create = 0, .read = 1, .write = 1 };

	fs_openFile(1, "/dev/null", crwFlags);
	fs_openFile(2, "/dev/null", rwFlags);

	fs_globalFileTable_dump();
	fs_processFileTables_dump();
}

void fs_init() {
	fs_globalFileTable = malloc(sizeof(fs_gft));
	fs_globalFileTable->entries = list_create();
	pthread_mutex_init(&(fs_globalFileTable->mutex), 0);

	fs_processFileTables = list_create();
}

void fs_openFile(int pid, char *path, fs_permission_flags flags) {
	fs_pft *pft = pft_find(pid);

	pft_addEntry(pft, pid, path, flags);
}

// Debug

void fs_globalFileTable_dump() {
	int i;

	log_debug(logger, "Global File Table. # of entries: %d", list_size(fs_globalFileTable->entries));

	for (i = 0; i < list_size(fs_globalFileTable->entries); i++) {
		fs_gft_entry *entry = list_get(fs_globalFileTable->entries, i);
		log_debug(logger, "path: %s. open: %d", entry->path, entry->open);
	}
}

void fs_processFileTables_dump() {
	int i;

	log_debug(logger, "Process File Tables. # of tables: %d", list_size(fs_processFileTables));

	for (i = 0; i < list_size(fs_processFileTables); i++) {
		fs_pft *pft = list_get(fs_processFileTables, i);

		log_debug(logger, "Process File Table. pid: %d - # of entries: %d", pft->pid, list_size(pft->entries));

		for (i = 0; i < list_size(pft->entries); i++) {
			fs_pft_entry *entry = list_get(pft->entries, i);

			log_debug(logger, "fd: %d. flags: %d%d%d. (path: %s - open: %d)", entry->fd, entry->flags.create, entry->flags.read, entry->flags.write, entry->gftEntry->path, entry->gftEntry->open);
		}
	}
}

// Private

fs_pft *pft_find(int pid) {
	int i;
	for (i = 0; i < list_size(fs_processFileTables); i++) {
		fs_pft *pft = list_get(fs_processFileTables, i);

		if (pft->pid == pid) return pft;
	}

	fs_pft *pft = pft_make(pid);
	list_add(fs_processFileTables, pft);
	return pft;
}

fs_pft *pft_make(int pid) {
	fs_pft *pft = malloc(sizeof(fs_pft));

	pft->pid = pid;
	pft->entries = list_create();
	pthread_mutex_init(&(pft->mutex), 0);

	return pft;
}

void pft_addEntry(fs_pft *pft, int pid, char *path, fs_permission_flags flags) {
	fs_pft_entry *entry = malloc(sizeof(fs_pft_entry));

	entry->flags = flags;
	entry->gftEntry = gft_findEntry(path);
	entry->gftEntry->open++;

	list_add(pft->entries, entry);
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

	list_add(fs_globalFileTable->entries, entry);
	return entry;
}
