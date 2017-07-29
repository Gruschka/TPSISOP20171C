/*
 * pcb.c
 *
 *  Created on: Apr 6, 2017
 *      Author: utnso
 */

#include <stdio.h>
#include <stdlib.h>
#include <commons/collections/list.h>
#include <commons/string.h>
#include <stdint.h>
#include <string.h>
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
	dummyReturn->filesTable = NULL;
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
	pcb_addStackIndexVariable(dummyReturn,variableA,VARIABLE);
	pcb_addStackIndexVariable(dummyReturn,variableB,VARIABLE);
	pcb_addStackIndexVariable(dummyReturn,argumentA,ARGUMENT);

	//list_add(dummyStack0->variables,variableA);
	pcb_addStackIndexRecord(dummyReturn);
	//list_add(dummyReturn->stackIndex,dummyStack0);

	//t_stackIndexRecord out = (t_stackIndexRecord *) list_get(dummyReturn.stackIndex,0);
	return dummyReturn;
}


uint32_t pcb_getPCBSize(t_PCB *PCB){
	int bufferSize = 0;

	// pid, pc, sp, ec, filesTable, codePages
	int PCBFixedVariablesSize = sizeof(uint32_t) + sizeof(int) + sizeof(int) + sizeof(int) + sizeof(void *) + sizeof(uint32_t);

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

	int instructionIndexSize = PCB->variableSize.instructionCount * sizeof(t_codeIndex);
	bufferSize += instructionIndexSize;

	bufferSize += PCB->variableSize.labelIndexSize;

	return bufferSize;
}
void *pcb_serializePCB(t_PCB *PCB){
	int bufferSize = pcb_getPCBSize(PCB);
	int test = pcb_getBufferSizeFromVariableSize(&PCB->variableSize);
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

	memcpy(buffer+offset,&(PCB->codePages),sizeof(uint32_t));
	offset += sizeof(uint32_t);

	int codeIndexSize = PCB->variableSize.instructionCount * sizeof(t_codeIndex);
	memcpy(buffer+offset,PCB->codeIndex,codeIndexSize);
	offset += codeIndexSize;

	memcpy(buffer+offset,PCB->labelIndex,PCB->variableSize.labelIndexSize);
	offset += PCB->variableSize.labelIndexSize;

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

			memcpy(buffer+offset,&(stackIndexRecord->returnPosition),sizeof(uint32_t));
			offset += sizeof(uint32_t);

			memcpy(buffer+offset,&(stackIndexRecord->returnVariable),sizeof(t_memoryDirection));
			offset += sizeof(t_memoryDirection);

			stackIndexRecordIterator++;
		}
	}

	memcpy(buffer+offset,&(PCB->filesTable),sizeof(void *));
	offset += sizeof(void *);

	return buffer;

}
t_PCB *pcb_deSerializePCB(void *serializedPCB, t_PCBVariableSize *variableSize){
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

	memcpy(&(deSerializedPCB->codePages),serializedPCB+offset,sizeof(uint32_t));
	offset += sizeof(uint32_t);

	int codeIndexSize = variableSize->instructionCount * sizeof(t_codeIndex);
	deSerializedPCB->codeIndex = malloc(codeIndexSize);
	memcpy(deSerializedPCB->codeIndex,serializedPCB+offset,codeIndexSize);
	offset += codeIndexSize;

	deSerializedPCB->labelIndex = malloc(variableSize->labelIndexSize);
	memcpy(deSerializedPCB->labelIndex,serializedPCB+offset,variableSize->labelIndexSize);
	offset += variableSize->labelIndexSize;

	int stackIndexRecordIterator = 0;
	int stackIndexVariableIterator = 0;
	int stackIndexArgumentIterator = 0;

	if(variableSize->stackIndexRecordCount != 0){
		deSerializedPCB->stackIndex = list_create();
		while(stackIndexRecordIterator < variableSize->stackIndexRecordCount){
			t_stackIndexRecord *stackIndexRecord = malloc(sizeof(t_stackIndexRecord));
			if(variableSize->stackArgumentCount !=0){
				stackIndexRecord->arguments = list_create();
				while(stackIndexArgumentIterator < variableSize->stackArgumentCount){
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

			if(variableSize->stackVariableCount !=0){
				stackIndexRecord->variables = list_create();
				while(stackIndexVariableIterator < variableSize->stackVariableCount){
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
			memcpy(&(stackIndexRecord->returnPosition),serializedPCB+offset,(sizeof(uint32_t)));
			offset += sizeof(uint32_t);

			memcpy(&(stackIndexRecord->returnVariable),serializedPCB+offset,(sizeof(t_memoryDirection)));
			offset += sizeof(t_memoryDirection);

			list_add(deSerializedPCB->stackIndex,stackIndexRecord);
			stackIndexRecordIterator++;
		}
	}else{
		deSerializedPCB->stackIndex = NULL;
	}

	return deSerializedPCB;
}
void pcb_addStackIndexRecord(t_PCB *PCB){
	t_stackIndexRecord *record = malloc(sizeof(t_stackIndexRecord));
	record->arguments = NULL;
	record->variables = NULL;
	record->returnPosition = 0;
	record->returnVariable.offset = 0;
	record->returnVariable.page = 0;
	record->returnVariable.size = 0;

	if (PCB != NULL) {
		if(PCB->stackIndex == NULL){
			PCB->stackIndex = list_create();
		}
		list_add(PCB->stackIndex,record);
		PCB->variableSize.stackIndexRecordCount++;
	}
}
void pcb_addStackIndexVariable(t_PCB *PCB, t_stackVariable *variable, t_variableAdd type){
	if(PCB->stackIndex == NULL){
		pcb_addStackIndexRecord(PCB);
	}
	t_stackIndexRecord *stackIndex = list_get(PCB->stackIndex,PCB->variableSize.stackIndexRecordCount-1);
	switch (type) {
		case VARIABLE:
			if (variable != NULL) {
				if (stackIndex->variables == NULL){
					stackIndex->variables = list_create();
				}
				list_add(stackIndex->variables,variable);
				PCB->variableSize.stackVariableCount++;
			}
			break;
		case ARGUMENT:
			if (variable != NULL) {
				if (stackIndex->arguments == NULL){
					stackIndex->arguments = list_create();
				}
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
	printf("codePages: %d \n",PCB->codePages);

	int stackIndexRecordIterator = 0;
	int stackIndexVariableIterator = 0;
	int stackIndexArgumentIterator = 0;
	int codeIndexIterator = 0;
	int labelIndexIterator = 0;
	int labelIndexIteratorSize = 0;

	printf("code index\n");
	while(codeIndexIterator < PCB->variableSize.instructionCount){
		printf("instruction %d - start: %d size: %d\n", codeIndexIterator,PCB->codeIndex[codeIndexIterator].start,PCB->codeIndex[codeIndexIterator].size);
		codeIndexIterator++;
	}

	printf("label index\n");
	int test = 0;
	while(labelIndexIteratorSize < PCB->variableSize.labelIndexSize){
		printf("label %d - tag: %s ",labelIndexIterator,(PCB->labelIndex)+labelIndexIteratorSize);
		labelIndexIteratorSize += strlen(PCB->labelIndex) + 1;
		memcpy(&test,PCB->labelIndex+labelIndexIteratorSize,sizeof(int));
		printf("dir: %d\n", test);
		labelIndexIteratorSize += sizeof(int);
		labelIndexIterator++;
	}

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
					printf("variable %d page: %d \n",stackIndexVariableIterator,variable->page);
					printf("variable %d offset: %d \n",stackIndexVariableIterator,variable->offset);
					printf("variable %d size: %d \n",stackIndexVariableIterator,variable->size);
					stackIndexVariableIterator++;
				}
			}

			stackIndexRecordIterator++;
		}
	}


}
uint32_t pcb_getBufferSizeFromVariableSize(t_PCBVariableSize *variableSize){
	uint32_t totalSize = 0;

	// variable size
	totalSize += sizeof(t_PCBVariableSize);

	// pid, pc, sp, ec, filesTable, codePages
	totalSize += sizeof(uint32_t) + sizeof(int) + sizeof(int) + sizeof(int) + sizeof(void *) + sizeof(uint32_t);

	// fixed data for each stack index record
	totalSize += variableSize->stackIndexRecordCount * (sizeof(uint32_t) + sizeof(t_memoryDirection));

	// variables and arguments
	totalSize += (variableSize->stackVariableCount + variableSize->stackArgumentCount) * sizeof(t_stackVariable);

	// label and instruction indexes
	totalSize += (variableSize->instructionCount * sizeof(t_codeIndex)) + variableSize->labelIndexSize;

	return totalSize;

}
void pcb_destroy(t_PCB *PCB){
	if(PCB->codeIndex != NULL){
		free(PCB->codeIndex);
		PCB->codeIndex = NULL;
	}

	if(PCB->labelIndex != NULL){
		free(PCB->labelIndex);
		PCB->labelIndex = NULL;
	}

	if(PCB->filesTable != NULL){
		free(PCB->filesTable);
		PCB->filesTable = NULL;
	}

	PCB->codePages = 0;
	PCB->ec = 0;
	PCB->pc = 0;
	PCB->pid = 0;
	PCB->sp = 0;

	int stackIndexRecordIterator = 0;
	int stackIndexVariableIterator = 0;
	int stackIndexArgumentIterator = 0;
	int codeIndexIterator = 0;

	if(PCB->stackIndex != NULL){
		while(stackIndexRecordIterator < PCB->variableSize.stackIndexRecordCount){
			t_stackIndexRecord *record = list_get(PCB->stackIndex,stackIndexRecordIterator);
			if(record->arguments != NULL){
				while(stackIndexArgumentIterator < PCB->variableSize.stackArgumentCount){
					t_stackVariable *argument = list_get(record->arguments,stackIndexArgumentIterator);
					free(argument);
					stackIndexArgumentIterator++;
				}
				list_destroy(record->arguments);
				if(record->arguments != NULL){
					record->arguments = NULL;
				}
			}
			if(record->variables !=NULL){
				while(stackIndexVariableIterator < PCB->variableSize.stackVariableCount){
					t_stackVariable *variable = list_get(record->variables,stackIndexVariableIterator);
					free(variable);
					stackIndexVariableIterator++;
				}
				list_destroy(record->variables);
				if(record->variables != NULL){
					record->variables = NULL;
				}
			}
			record->returnPosition = 0;
			record->returnVariable.offset = 0;
			record->returnVariable.page = 0;
			record->returnVariable.size = 0;
			free(record);
			stackIndexRecordIterator++;
		}
		list_destroy(PCB->stackIndex);
		if(PCB->stackIndex != NULL){
			PCB->stackIndex = NULL;
		}
	}
	PCB->variableSize.instructionCount = 0;
	PCB->variableSize.labelIndexSize = 0;
	PCB->variableSize.stackArgumentCount = 0;
	PCB->variableSize.stackIndexRecordCount = 0;
	PCB->variableSize.stackVariableCount = 0;

	free(PCB);
}
t_stackVariable *pcb_getVariable(t_PCB *PCB, char variableName){
	int variableIterator = 0;
	if(PCB->stackIndex != NULL){
		t_stackIndexRecord *record = list_get(PCB->stackIndex,PCB->variableSize.stackIndexRecordCount-1);
		if(record->variables != NULL){
			while(variableIterator < PCB->variableSize.stackVariableCount){
				t_stackVariable *variable = list_get(record->variables,variableIterator);
				if(variableName == variable->id){
					return variable;
				}
				variableIterator++;
			}
		}

	}

}
void pcb_addSpecificStackIndexRecord(t_PCB *PCB,t_stackIndexRecord *record){

	if (PCB != NULL) {
		if(PCB->stackIndex == NULL){
			PCB->stackIndex = list_create();
		}
		list_add(PCB->stackIndex,record);
		PCB->variableSize.stackIndexRecordCount++;
	}
}
int pcb_getDirectionFromLabel(t_PCB *PCB, char *label){
	int labelIndexIteratorSize = 0;
	int labelIndexIterator = 0;
	char *buffer;
	int labelLength = 0;
	int pointer;
	char *compareLabel = strdup(label);

	while(labelIndexIteratorSize < PCB->variableSize.labelIndexSize){
		labelLength = strlen(PCB->labelIndex+labelIndexIteratorSize) + 1;
		buffer = malloc(labelLength);
		memcpy(buffer,PCB->labelIndex+labelIndexIteratorSize,labelLength+1);

		labelIndexIteratorSize += labelLength;

		memcpy(&pointer,PCB->labelIndex+labelIndexIteratorSize,sizeof(int));
		labelIndexIteratorSize += sizeof(int);

		if(labelLength == 2){
			compareLabel = string_substring(compareLabel,0,1);
		}
		if(!strcmp(buffer,compareLabel)){
			free(buffer);
			return pointer;
		}

		labelIndexIterator++;
	}
}
t_PCBVariableSize pcb_getSizeOfSpecificStack(t_stackIndexRecord *record){
	t_PCBVariableSize variableSize;
	variableSize.stackArgumentCount = 0;
	variableSize.stackVariableCount = 0;

	if(record->arguments !=NULL){
		variableSize.stackArgumentCount = list_size(record->arguments);
	}
	if(record->variables !=NULL){
		variableSize.stackVariableCount = list_size(record->variables);
	}

	variableSize.stackIndexRecordCount = 1;

	return variableSize;
}
void pcb_decompileStack(t_PCB *PCB){
	int stackLevelToDecompile = PCB->variableSize.stackIndexRecordCount -1;

	t_stackIndexRecord *record = list_get(PCB->stackIndex,stackLevelToDecompile);

	t_PCBVariableSize variableSize = pcb_getSizeOfSpecificStack(record);

	list_remove(PCB->stackIndex,stackLevelToDecompile);

	PCB->variableSize.stackIndexRecordCount--;
	PCB->variableSize.stackArgumentCount -= variableSize.stackArgumentCount;
	PCB->variableSize.stackVariableCount -= variableSize.stackVariableCount;
}
t_variableAdd pcb_getVariableAddTypeFromTag(char tag){
	if (tag=='0' || tag=='1' || tag=='2' || tag=='3' || tag=='4' || tag=='5' || tag=='6' || tag=='7' || tag=='8' || tag=='9'){
		return ARGUMENT;
	}else{
		return VARIABLE;
	}
}
