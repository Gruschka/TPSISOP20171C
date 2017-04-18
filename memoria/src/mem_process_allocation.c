/*
 * mem_process_allocation.c
 *
 *  Created on: Apr 18, 2017
 *      Author: Federico Trimboli
 */

#include "mem_process_allocation.h"
#include "mem_process_allocation_list.h"

static mem_process_allocation_list *list;

void mem_process_allocation_configure() {
	if (list != NULL) { return; }
	list = mem_process_allocation_list_createEmptyList();
}

int mem_process_allocation_isProcessAlreadyInitialized(int32_t processID) {
	void *node = mem_process_allocation_list_getNode(processID, list);
	return node != NULL;
}

int mem_process_allocation_initProcessWithPages(int32_t processID, int32_t numberOfPages) {
	if (mem_process_allocation_isProcessAlreadyInitialized(processID)) { return 0; }
	mem_process_allocation_entry entry = { processID, numberOfPages };
	mem_process_allocation_list_add(entry, list);
}

void mem_process_allocation_allocPagesToExistingProcess(int32_t processID, int32_t numberOfPages) {

}

void mem_process_allocation_deinitProcess(int32_t processID, int32_t numberOfPages) {

}
