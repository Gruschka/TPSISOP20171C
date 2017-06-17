/*
 * pcb.c
 *
 *  Created on: Apr 6, 2017
 *      Author: utnso
 */

#include <stdio.h>
#include <stdlib.h>
#include <commons/collections/list.h>
#include <stdint.h>
#include "pcb.h"

t_PCB pcb_createDummy(int32_t processID, int32_t programCounter, int32_t StackPointer,int32_t ExitCode){
	t_PCB dummyReturn;

	// set parameter values
	dummyReturn.pid = processID;
	dummyReturn.pc = programCounter;
	dummyReturn.sp = StackPointer;
	dummyReturn.ec = ExitCode;
	// create stack index list
	dummyReturn.stackIndex = list_create();
	// create current stack
	// simple one level/2 variables a&b
	// create stack entry
	t_stackIndexRecord dummyStack0;
	dummyStack0.variables = list_create();
	//create a & b
	t_stackVariable variableA,variableB;
	variableA.id = 'a';
	variableA.offset = 0;
	variableA.page = 0;
	variableA.size = sizeof(uint32_t);
	variableA.id = 'b';
	variableA.offset = variableA.size;
	variableA.page = 0;
	variableA.size = sizeof(uint32_t);
	//add a &B
	list_add(dummyStack0.variables,&variableA);
	list_add(dummyStack0.variables,&variableB);

	list_add(dummyReturn.stackIndex,&dummyStack0);
	//t_stackIndexRecord out = (t_stackIndexRecord *) list_get(dummyReturn.stackIndex,0);
	return dummyReturn;
}

