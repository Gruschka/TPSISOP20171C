/*
 * filesystem.h
 *
 *  Created on: Jul 8, 2017
 *      Author: utnso
 */

#ifndef FILESYSTEM_H_
#define FILESYSTEM_H_

#include <commons/collections/list.h>
#include <pthread.h>
#include <stdint.h>

typedef enum operation {
	CREATE, READ, WRITE
} fs_operation;

typedef struct permission_flags {
	char create;
	char read;
	char write;
} fs_permission_flags;

typedef struct gft_entry {
	char *path;
	uint32_t open;
	uint32_t _fd;
} fs_gft_entry;

typedef struct pft_entry {
	uint32_t fd;
	fs_permission_flags flags;
	fs_gft_entry *gftEntry;
} fs_pft_entry ;

typedef struct pft {
	int pid;
	t_list *entries;
	pthread_mutex_t mutex;
} fs_pft;

typedef struct gft {
	t_list *entries;
	pthread_mutex_t mutex;
} fs_gft;

t_list *fs_processFileTables;
fs_gft *fs_globalFileTable;

// Public
void fs_init();
int fs_openFile(int pid, char *path, char *permissionsString);

// Debug
void fs_globalFileTable_dump();
void fs_processFileTables_dump();

// Private
fs_pft *pft_make(int pid);
fs_pft *pft_find(int pid);
fs_gft_entry *gft_findEntry(char *path);
int pft_addEntry(fs_pft *pft, int pid, char *path, fs_permission_flags flags);
fs_gft_entry *gft_addEntry(char *path);
int fs_isOperationAllowed(int pid, int fd, fs_operation operation);
fs_permission_flags permissions(char *permissionsString);

#endif /* FILESYSTEM_H_ */
