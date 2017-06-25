/*
 * memory.c
 *
 *  Created on: May 9, 2017
 *      Author: utnso
 */
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <commons/log.h>

#include "memory.h"

extern uint32_t pageSize;
extern t_log *logger;

void *memory_createPage(uint32_t size) {
	void *page = malloc(size);
	kernel_heapMetadata metadata = {size - sizeof(kernel_heapMetadata), true};
	memcpy(page, &metadata, sizeof(kernel_heapMetadata));
	return page;
	//TODO: Escribir esto en la memoria, devolver el puntero "virtual" y previamente hacer free()
}

void *memory_addBlock(void *page, uint32_t size) {
	if (size > pageSize) return NULL;

	kernel_heapMetadata *newBlockMetadata = memory_getAvailableBlock(page, size);

	if (newBlockMetadata == NULL) { // No hubo espacio en la página
		return NULL;
	} else { // Devuelvo el puntero al bloque reservado
		uint32_t remainingSize = newBlockMetadata->size - size - sizeof(kernel_heapMetadata);

		newBlockMetadata->isFree = false;
		newBlockMetadata->size = size;

		if (remainingSize >= sizeof(kernel_heapMetadata)) {
			kernel_heapMetadata nextBlockMetadata = { remainingSize, true };

			memcpy((void *)newBlockMetadata + sizeof(kernel_heapMetadata) + size, &nextBlockMetadata, sizeof(kernel_heapMetadata));
			return (void *)newBlockMetadata + sizeof(kernel_heapMetadata);
		}
	}

	return NULL;
}

// Devuelve el puntero a la metadata del bloque disponible
kernel_heapMetadata *memory_getAvailableBlock(void *page, uint32_t size) {
	bool found = false;
	uint32_t offset = 0;
	kernel_heapMetadata metadata;

	while (!found && offset < pageSize) {
		memcpy(&metadata, page + offset, sizeof(kernel_heapMetadata));
		if (metadata.isFree && (metadata.size >= size + sizeof(kernel_heapMetadata) || metadata.size == size)) { // está disponible y tiene espacio
			found = true;
		} else if (metadata.isFree && metadata.size == 0) { // es el ultimo bloque PONELE
			break;
		} else { // no está disponible o no tiene espacio suficiente
			offset += sizeof(kernel_heapMetadata) + metadata.size;
		}
	}

	return found ? page + offset : NULL;
}

void memory_dumpPage(void *page) {
	kernel_heapMetadata metadata;
	uint32_t offset = 0;
	bool exit = false;

	while (!exit) {
		memcpy(&metadata, page + offset, sizeof(kernel_heapMetadata));
		log_debug(logger, "[Metadata] size: %d - isFree: %s", metadata.size, metadata.isFree ? "true" : "false");

		if (offset + metadata.size + sizeof(kernel_heapMetadata) == pageSize) {
			exit = true;
		} else {
			offset += metadata.size + sizeof(kernel_heapMetadata);
		}
	}
}
