/*
 * memory.h
 *
 *  Created on: May 9, 2017
 *      Author: utnso
 */

#ifndef MEMORY_H_
#define MEMORY_H_

#include <stdint.h>

typedef int bool;
enum { false, true };

typedef struct kernel_heapMetadata {
	uint32_t size;
	bool isFree;
} kernel_heapMetadata;

kernel_heapMetadata *memory_getAvailableBlock(void *page, uint32_t size);
void *memory_addBlock(void *page, uint32_t size);
void *memory_createPage(uint32_t size);

#endif /* MEMORY_H_ */
