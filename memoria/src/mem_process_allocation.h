/*
 * mem_process_allocation.h
 *
 *  Created on: Apr 18, 2017
 *      Author: utnso
 */

#ifndef MEM_PROCESS_ALLOCATION_H_
#define MEM_PROCESS_ALLOCATION_H_

#include <stdio.h>
#include <stdlib.h>

int mem_process_allocation_isProcessAlreadyInitialized(int32_t processID);
void mem_process_allocation_initProcessWithPages(int32_t processID, int32_t numberOfPages);
void mem_process_allocation_allocPagesToExistingProcess(int32_t processID, int32_t numberOfPages);
void mem_process_allocation_deinitProcess(int32_t processID, int32_t numberOfPages);

#endif /* MEM_PROCESS_ALLOCATION_H_ */
