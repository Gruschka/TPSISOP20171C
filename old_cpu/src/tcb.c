/*
 * tcb.c
 *
 *  Created on: Sep 29, 2014
 *      Author: utnso
 */


#include "tcb.h"
#include "memory.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

t_TCB *tcb_createFromScriptWithSize(const char *script, int size){
	t_TCB *newTCB = malloc(sizeof(t_TCB));

	newTCB->pid = rand(); //TODO: change with pid generator
	newTCB->tid = 1; //TODO: change with tid generator
	newTCB->km = 0; // TODO: change with corresponding km
	newTCB->codeSegmentSize = size;
	tcb_clearRegisters(newTCB);
	tcb_getSegments(newTCB,DEFAULT_STACK_SIZE,newTCB->codeSegmentSize);
	newTCB->instructionPointer = newTCB->codeSegmentBase;
	newTCB->stackCursor = newTCB->stackBase;

	return newTCB;
}

void tcb_clearRegisters(t_TCB *myTCB){
	int iterator;
	for (iterator = 0; iterator < 5; ++iterator) {
		myTCB->registers[iterator] = 0;
	}
}

void tcb_getSegments(t_TCB *myTCB, int stackSize, int codeSize){
	myTCB->codeSegmentBase = memory_requestSegment(codeSize);
	myTCB->stackBase = memory_requestSegment(stackSize);
}

int tcb_parameterSizeFromInstruction(char *instruction){
	char *buffer = malloc(sizeof(int)+sizeof(char));
	memcpy(buffer,instruction,sizeof(int));
	buffer[sizeof(int)] = 0;

	if(!strcmp(buffer,"LOAD")){
		return (sizeof(char) + sizeof(int));
	}

	if(!strcmp(buffer,"MULR")){
		return (sizeof(char) + sizeof(char));
	}

	if(!strcmp(buffer,"ADDR")){
		return (sizeof(char) + sizeof(char));
	}

	if(!strcmp(buffer,"DECR")){
		return (sizeof(char));
	}

	if(!strcmp(buffer,"DIVR")){
		return (sizeof(char) + sizeof(char));
	}

	if(!strcmp(buffer,"MODR")){
		return (sizeof(char) + sizeof(char));
	}

	if(!strcmp(buffer,"SUBR")){
		return (sizeof(char) + sizeof(char));
	}

	if(!strcmp(buffer,"INCR")){
		return (sizeof(char));
	}

	if(!strcmp(buffer,"XXXX")){
		return 0;
	}

	return 0;
}

t_instructionIndex *tcb_generateIndexFromScript(char *script, int size){
	int offset = 0;
	int parameterSize = 0;

	char *buffer = malloc(sizeof(int));
	void *result = malloc(sizeof(t_instructionIndex));
	t_instructionIndex test;
	int numberOfInstructions = 0;


	while (offset < size) {
		memcpy(buffer,script + offset,sizeof(int));

		parameterSize = tcb_parameterSizeFromInstruction(buffer);

		test.instructionOffset = offset;
		test.instructionSize = sizeof(int) + parameterSize;

		offset+= test.instructionSize;

		memcpy(result+(sizeof(t_instructionIndex) * numberOfInstructions),&test,sizeof(t_instructionIndex));
		numberOfInstructions++;

		if(offset < size){
			result = realloc(result,sizeof(t_instructionIndex) * (numberOfInstructions+1));
		}
	}

	return (t_instructionIndex *) result;
}

