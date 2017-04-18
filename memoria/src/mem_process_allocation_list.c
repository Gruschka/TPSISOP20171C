/*
 * mem_process_allocation_list.c
 *
 *  Created on: Apr 18, 2017
 *      Author: Federico Trimboli
 */

#include "mem_process_allocation_list.h"

mem_process_allocation_list_node *createNode(mem_process_allocation_entry processAllocation) {
	mem_process_allocation_list_node *newNode = malloc(sizeof(mem_process_allocation_list_node));
	newNode->processAllocationEntry = processAllocation;
	newNode->next = NULL;
	return newNode;
}

mem_process_allocation_list *mem_process_allocation_list_createEmptyList() {
	mem_process_allocation_list *list = malloc(sizeof(mem_process_allocation_list));
	list->head = NULL;
	return list;
}

void mem_process_allocation_list_add(mem_process_allocation_entry processAllocationEntry, mem_process_allocation_list *list) {
	mem_process_allocation_list_node *current = NULL;
	if (list->head == NULL) {
		list->head = createNode(processAllocationEntry);
	} else {
		current = list->head;
		while (current->next != NULL) {
			current = current->next;
		}
		current->next = createNode(processAllocationEntry);
	}
}

mem_process_allocation_list_node *mem_process_allocation_list_getNode(int32_t processID, mem_process_allocation_list *list) {
	if (list == NULL) { return NULL; }

	mem_process_allocation_list_node *current = list->head;
	while (current != NULL) {
		if (current->processAllocationEntry.processID == processID) {
			return current;
		}
		current = current->next;
	}

	return NULL;
}

void mem_process_allocation_list_delete(mem_process_allocation_entry processAllocationEntry, mem_process_allocation_list *list) {
	mem_process_allocation_list_node *current = list->head;
	mem_process_allocation_list_node *previous = current;
	while (current != NULL) {
    if (current->processAllocationEntry.processID == processAllocationEntry.processID) {
    	previous->next = current->next;
    	if (current == list->head) {
    		list->head = current->next;
    	}
    	free(current);
    	return;
    }
    previous = current;
    current = current->next;
  }
}

void mem_process_allocation_list_destroy(mem_process_allocation_list *list){
	mem_process_allocation_list_node *current = list->head;
	mem_process_allocation_list_node *next = current;
	while (current != NULL) {
    next = current->next;
    free(current);
    current = next;
  }
  free(list);
}
