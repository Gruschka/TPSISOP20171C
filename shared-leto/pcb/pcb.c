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

t_PCB *pcb_createDummy(int32_t processID, int32_t programCounter, int32_t StackPointer,int32_t ExitCode){
	t_PCB *dummyReturn = malloc(sizeof(t_PCB));

	// set parameter values
	dummyReturn->pid = processID;
	dummyReturn->pc = programCounter;
	dummyReturn->sp = StackPointer;
	dummyReturn->ec = ExitCode;
	dummyReturn->variableSize.stackArgumentCount = 0;
	dummyReturn->variableSize.stackIndexRecordCount = 0;
	dummyReturn->variableSize.stackVariableCount = 0;
	// create stack index list
	dummyReturn->stackIndex = list_create();
	// create current stack
	// simple one level/2 variables a&b
	// create stack entry
	t_stackIndexRecord *dummyStack0 = malloc(sizeof(t_stackIndexRecord));
	dummyStack0->arguments = NULL;
	dummyStack0->variables = NULL;
	dummyStack0->returnPosition = 3;
	dummyStack0->returnVariable.offset = 1;
	dummyStack0->returnVariable.page = 2;
	dummyStack0->returnVariable.size = 3;
	dummyStack0->variables = list_create();
	dummyStack0->arguments = list_create();
	//create a & b
	t_stackVariable *variableA = malloc(sizeof(t_stackVariable));
	variableA->id = 'a';
	variableA->offset = 0;
	variableA->page = 0;
	variableA->size = sizeof(uint32_t);
	t_stackVariable *variableB = malloc(sizeof(t_stackVariable));
	variableB->id = 'b';
	variableB->offset = 3;
	variableB->page = 4;
	variableB->size = 4 + sizeof(uint32_t);
	t_stackVariable *argumentA = malloc(sizeof(t_stackVariable));
	argumentA->id = 'd';
	argumentA->offset = 1;
	argumentA->page = 2;
	argumentA->size = 8 + sizeof(uint32_t);
	//add a &B
	pcb_addStackIndexVariable(dummyReturn,dummyStack0,variableA,VARIABLE);
	pcb_addStackIndexVariable(dummyReturn,dummyStack0,variableB,VARIABLE);
	pcb_addStackIndexVariable(dummyReturn,dummyStack0,argumentA,ARGUMENT);

	//list_add(dummyStack0->variables,variableA);
	pcb_addStackIndexRecord(dummyReturn,dummyStack0);
	//list_add(dummyReturn->stackIndex,dummyStack0);

	//t_stackIndexRecord out = (t_stackIndexRecord *) list_get(dummyReturn.stackIndex,0);
	return dummyReturn;
}

uint32_t pcb_getPCBSize(t_PCB *PCB){
	int bufferSize = 0;

	// pid, pc, sp, ec, filesTable
	int PCBFixedVariablesSize = sizeof(uint32_t) + sizeof(int) + sizeof(int) + sizeof(int) + sizeof(void *);

	bufferSize += PCBFixedVariablesSize;

	// size of memory direction type
	int memoryDirectionSize = 3 * sizeof(uint32_t);

	// count fixed data for each stack index record
	bufferSize += PCB->variableSize.stackIndexRecordCount * (sizeof(uint32_t) + memoryDirectionSize);

	// size of a variable/argument
	int variableSize = sizeof(char) + 3*sizeof(uint32_t);

	// count all variables/arguments regardless of stack index level
	bufferSize += variableSize * (PCB->variableSize.stackVariableCount + PCB->variableSize.stackArgumentCount);

	int administrativeVariableDataSize = sizeof(t_PCBVariableSize);
	bufferSize += administrativeVariableDataSize;

	return bufferSize;
}

void *pcb_serializePCB(t_PCB *PCB){
	int bufferSize = pcb_getPCBSize(PCB);
	int offset = 0;

	void *buffer = malloc(bufferSize);
	memset(buffer,0,bufferSize);

	memcpy(buffer+offset,&(PCB->variableSize),sizeof(t_PCBVariableSize));
	offset += sizeof(t_PCBVariableSize);

	memcpy(buffer+offset,&(PCB->pid),sizeof(uint32_t));
	offset += sizeof(uint32_t);

	memcpy(buffer+offset,&(PCB->pc),sizeof(uint32_t));
	offset += sizeof(uint32_t);

	memcpy(buffer+offset,&(PCB->sp),sizeof(uint32_t));
	offset += sizeof(uint32_t);

	memcpy(buffer+offset,&(PCB->ec),sizeof(uint32_t));
	offset += sizeof(uint32_t);

	int stackIndexRecordIterator = 0;
	int stackIndexVariableIterator = 0;
	int stackIndexArgumentIterator = 0;

	if(PCB->variableSize.stackIndexRecordCount != 0 && PCB->stackIndex != NULL){
		while(stackIndexRecordIterator < PCB->variableSize.stackIndexRecordCount){
			t_stackIndexRecord *stackIndexRecord  = list_get(PCB->stackIndex,stackIndexRecordIterator);
			if(PCB->variableSize.stackArgumentCount != 0 && stackIndexRecord->arguments != NULL){
				while(stackIndexArgumentIterator < PCB->variableSize.stackArgumentCount){
					t_stackVariable *argument = list_get(stackIndexRecord->arguments,stackIndexArgumentIterator);
					memcpy(buffer+offset,&(argument->id),sizeof(char));
					offset += sizeof(char);
					memcpy(buffer+offset,&(argument->offset),sizeof(uint32_t));
					offset += sizeof(uint32_t);
					memcpy(buffer+offset,&(argument->page),sizeof(uint32_t));
					offset += sizeof(uint32_t);
					memcpy(buffer+offset,&(argument->size),sizeof(uint32_t));
					offset += sizeof(uint32_t);
					stackIndexArgumentIterator++;
				}
			}
			memcpy(buffer+offset,&(stackIndexRecord->returnPosition),sizeof(uint32_t));
			offset += sizeof(uint32_t);

			memcpy(buffer+offset,&(stackIndexRecord->returnVariable),sizeof(t_memoryDirection));
			offset += sizeof(t_memoryDirection);

			if(PCB->variableSize.stackVariableCount != 0 && stackIndexRecord->variables != NULL){
				while(stackIndexVariableIterator < PCB->variableSize.stackVariableCount){
					t_stackVariable *variable = list_get(stackIndexRecord->variables,stackIndexVariableIterator);
					memcpy(buffer+offset,&(variable->id),sizeof(char));
					offset += sizeof(char);
					memcpy(buffer+offset,&(variable->offset),sizeof(uint32_t));
					offset += sizeof(uint32_t);
					memcpy(buffer+offset,&(variable->page),sizeof(uint32_t));
					offset += sizeof(uint32_t);
					memcpy(buffer+offset,&(variable->size),sizeof(uint32_t));
					offset += sizeof(uint32_t);
					stackIndexVariableIterator++;
				}
			}

			stackIndexRecordIterator++;
		}
	}

	memcpy(buffer+offset,&(PCB->filesTable),sizeof(void *));
	offset += sizeof(void *);

	return buffer;

}

t_PCB *pcb_deSerializePCB(void *serializedPCB, int stackIndexRecordCount, int stackIndexVariableCount, int stackIndexArgumentCount){
	t_PCB *deSerializedPCB = malloc(sizeof(t_PCB));
	void *buffer;
	int offset = 0;

	memcpy(&(deSerializedPCB->variableSize),serializedPCB+offset,sizeof(t_PCBVariableSize));
	offset += sizeof(t_PCBVariableSize);

	memcpy(&(deSerializedPCB->pid),serializedPCB+offset,sizeof(uint32_t));
	offset += sizeof(uint32_t);

	memcpy(&(deSerializedPCB->pc),serializedPCB+offset,sizeof(uint32_t));
	offset += sizeof(uint32_t);

	memcpy(&(deSerializedPCB->sp),serializedPCB+offset,sizeof(uint32_t));
	offset += sizeof(uint32_t);

	memcpy(&(deSerializedPCB->ec),serializedPCB+offset,sizeof(uint32_t));
	offset += sizeof(uint32_t);

	int stackIndexRecordIterator = 0;
	int stackIndexVariableIterator = 0;
	int stackIndexArgumentIterator = 0;

	if(stackIndexRecordCount != 0){
		deSerializedPCB->stackIndex = list_create();
		while(stackIndexRecordIterator < stackIndexRecordCount){
			t_stackIndexRecord *stackIndexRecord = malloc(sizeof(t_stackIndexRecord));
			if(stackIndexArgumentCount !=0){
				stackIndexRecord->arguments = list_create();
				while(stackIndexArgumentIterator < stackIndexArgumentCount){
					t_stackVariable *argument = malloc(sizeof(t_stackVariable));
					memcpy(&(argument->id),serializedPCB+offset,sizeof(char));
					offset += sizeof(char);
					memcpy(&(argument->offset),serializedPCB+offset,sizeof(uint32_t));
					offset += sizeof(uint32_t);
					memcpy(&(argument->page),serializedPCB+offset,sizeof(uint32_t));
					offset += sizeof(uint32_t);
					memcpy(&(argument->size),serializedPCB+offset,sizeof(uint32_t));
					offset += sizeof(uint32_t);

					list_add(stackIndexRecord->arguments,argument);
					stackIndexArgumentIterator++;
				}
			}
			memcpy(&(stackIndexRecord->returnPosition),serializedPCB+offset,(sizeof(uint32_t)));
			offset += sizeof(uint32_t);

			memcpy(&(stackIndexRecord->returnVariable),serializedPCB+offset,(sizeof(t_memoryDirection)));
			offset += sizeof(t_memoryDirection);


			if(stackIndexVariableCount !=0){
				stackIndexRecord->variables = list_create();
				while(stackIndexVariableIterator < stackIndexVariableCount){
					t_stackVariable *variable = malloc(sizeof(t_stackVariable));
					memcpy(&(variable->id),serializedPCB+offset,sizeof(char));
					offset += sizeof(char);
					memcpy(&(variable->offset),serializedPCB+offset,sizeof(uint32_t));
					offset += sizeof(uint32_t);
					memcpy(&(variable->page),serializedPCB+offset,sizeof(uint32_t));
					offset += sizeof(uint32_t);
					memcpy(&(variable->size),serializedPCB+offset,sizeof(uint32_t));
					offset += sizeof(uint32_t);

					list_add(stackIndexRecord->variables,variable);
					stackIndexVariableIterator++;
				}
			}


			list_add(deSerializedPCB->stackIndex,stackIndexRecord);
			stackIndexRecordIterator++;
		}
	}

	return deSerializedPCB;
}


void pcb_addStackIndexRecord(t_PCB *PCB, t_stackIndexRecord *record){
	if (record != NULL && PCB != NULL) {
		list_add(PCB->stackIndex,record);
		PCB->variableSize.stackIndexRecordCount++;
	}
}
void pcb_addStackIndexVariable(t_PCB *PCB, t_stackIndexRecord *stackIndex, t_stackVariable *variable, t_variableAdd type){
	switch (type) {
		case VARIABLE:
			if (variable != NULL && stackIndex->variables != NULL) {
				list_add(stackIndex->variables,variable);
				PCB->variableSize.stackVariableCount++;
			}
			break;
		case ARGUMENT:
			if (variable != NULL && stackIndex->arguments != NULL) {
				list_add(stackIndex->arguments,variable);
				PCB->variableSize.stackArgumentCount++;
			}
			break;
		default:
			break;
	}
}

void pcb_dump(t_PCB *PCB){
	printf("PCB DUMP \n");
	printf("pid: %d \n",PCB->pid);
	printf("pc: %d \n",PCB->pc);
	printf("sp: %d \n",PCB->sp);
	printf("ec: %d \n",PCB->ec);

	int stackIndexRecordIterator = 0;
	int stackIndexVariableIterator = 0;
	int stackIndexArgumentIterator = 0;

	if(PCB->variableSize.stackIndexRecordCount != 0 && PCB->stackIndex != NULL){
		while(stackIndexRecordIterator < PCB->variableSize.stackIndexRecordCount){
			printf("StackIndex level %d \n", stackIndexRecordIterator);
			t_stackIndexRecord *stackIndexRecord  = list_get(PCB->stackIndex,stackIndexRecordIterator);
			if(PCB->variableSize.stackArgumentCount != 0 && stackIndexRecord->arguments != NULL){
				while(stackIndexArgumentIterator < PCB->variableSize.stackArgumentCount){
					printf("arguments: \n");
					t_stackVariable *argument = list_get(stackIndexRecord->arguments,stackIndexArgumentIterator);
					printf("argument %d id: %c \n",stackIndexArgumentIterator,argument->id);
					printf("argument %d offset: %d \n",stackIndexArgumentIterator,argument->offset);
					printf("argument %d page: %d \n",stackIndexArgumentIterator,argument->page);
					printf("argument %d size: %d \n",stackIndexArgumentIterator,argument->size);
					stackIndexArgumentIterator++;
				}
			}
			printf("return position: %d \n",stackIndexRecord->returnPosition);
			printf("return variable page: %d offset: %d size: %d \n",stackIndexRecord->returnVariable.page,stackIndexRecord->returnVariable.offset,stackIndexRecord->returnVariable.size);

			if(PCB->variableSize.stackVariableCount != 0 && stackIndexRecord->variables != NULL){
				while(stackIndexVariableIterator < PCB->variableSize.stackVariableCount){
					printf("variables: \n");
					t_stackVariable *variable = list_get(stackIndexRecord->variables,stackIndexVariableIterator);
					printf("variable %d id: %c \n",stackIndexVariableIterator,variable->id);
					printf("variable %d offset: %d \n",stackIndexVariableIterator,variable->offset);
					printf("variable %d page: %d \n",stackIndexVariableIterator,variable->page);
					printf("variable %d size: %d \n",stackIndexVariableIterator,variable->size);
					stackIndexVariableIterator++;
				}
			}

			stackIndexRecordIterator++;
		}
	}


}
