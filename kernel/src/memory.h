/*
 * memory.h
 *
 *  Created on: May 9, 2017
 *      Author: utnso
 */

#ifndef MEMORY_H_
#define MEMORY_H_

#include <stdint.h>
#include <stdbool.h>

typedef struct kernel_heapMetadata {
	uint32_t size;
	bool isFree;
} __attribute__((packed)) kernel_heapMetadata;

kernel_heapMetadata *memory_getAvailableBlock(void *page, uint32_t size);
void *memory_addBlock(void *page, uint32_t size);
void *memory_createPage(int pid);
void memory_dumpPage(void *page);

#endif /* MEMORY_H_ */
