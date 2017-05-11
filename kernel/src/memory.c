/*
 * memory.c
 *
 *  Created on: May 9, 2017
 *      Author: utnso
 */
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "memory.h"

extern uint32_t page_size;

void *memory_createPage(uint32_t size) {
	void *page = malloc(size);
	kernel_heapMetadata metadata = {size, true};
	memcpy(page, &metadata, sizeof(kernel_heapMetadata));
	return page;
}

void *memory_addBlock(void *page, uint32_t size) {
	if (size > page_size) return NULL;

	kernel_heapMetadata *newBlockMetadata = memory_getAvailableBlock(page, size);

	if (newBlockMetadata == NULL) { // No hubo espacio en la página
		return NULL;
	} else { // Devuelvo el puntero al bloque reservado
		newBlockMetadata->isFree = false;
		return newBlockMetadata + sizeof(kernel_heapMetadata);
	}

	return NULL;
}

// Devuelve el puntero a la metadata del bloque disponible
kernel_heapMetadata *memory_getAvailableBlock(void *page, uint32_t size) {
	bool found = false;
	uint32_t offset = 0;
	kernel_heapMetadata metadata;

	while (!found) {
		memcpy(&metadata, page + offset, sizeof(kernel_heapMetadata));
		if (metadata.isFree && metadata.size >= size) { // está disponible y tiene espacio
			// acá tengo que validar que me quede espacio para el siguiente metadata
			kernel_heapMetadata nextMetadata;
			memcpy(&nextMetadata, page + offset + metadata.size, sizeof(kernel_heapMetadata));
			found = true;
		} else if (metadata.isFree && metadata.size == 0) { // es el ultimo bloque
			break;
		} else { // no está disponible o no tiene espacio suficiente
			offset = sizeof(kernel_heapMetadata) + metadata.size;
		}
	}

	return found ? page + offset : NULL;
}
