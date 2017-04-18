/*
 * mem_process_allocation_list.h
 *
 *  Created on: Apr 18, 2017
 *      Author: Federico Trimboli
 */

#ifndef MEM_PROCESS_ALLOCATION_LIST_H_
#define MEM_PROCESS_ALLOCATION_LIST_H_

#include <stdio.h>
#include <stdlib.h>

typedef struct mem_process_allocation_entry {
	int32_t processID;
	int32_t numberOfAllocatedPages;
} mem_process_allocation_entry;

typedef struct mem_process_allocation_list_node {
  mem_process_allocation_entry processAllocationEntry;
  struct mem_process_allocation_list_node *next;
} mem_process_allocation_list_node;

typedef struct mem_process_allocation_list {
	mem_process_allocation_list_node *head;
} mem_process_allocation_list;

mem_process_allocation_list *mem_process_allocation_list_createEmptyList();
void mem_process_allocation_list_add(mem_process_allocation_entry processAllocationEntry, mem_process_allocation_list *list);
mem_process_allocation_list_node *mem_process_allocation_list_getNode(int32_t processID, mem_process_allocation_list *list);
void mem_process_allocation_list_delete(mem_process_allocation_entry processAllocationEntry, mem_process_allocation_list *list);
void mem_process_allocation_list_destroy(mem_process_allocation_list *list);

#endif /* MEM_PROCESS_ALLOCATION_H_ */
