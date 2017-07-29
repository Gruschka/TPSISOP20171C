/*
 ============================================================================
 Name        : cpu.c
 Author      : Hernan Canzonetta
 Version     :
 Copyright   : Copyright
 Description : Hello World in C, Ansi-style
 ============================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <ipc/ipc.h>
#include <pcb/pcb.h>
#include <stdio.h>
#include <stdlib.h>
#include <netdb.h>
#include <netinet/in.h>
#include <string.h>
#include <unistd.h>
#include "cpu.h"
#include <commons/log.h>
#include <parser/metadata_program.h>
#include "dummy_ansisop.h"
#include <string.h>
#include <commons/string.h>

t_CPU myCPU;
void *myMemory;
char *program =
//"#!/usr/bin/ansisop\n"
//"\n"
//"begin\n"
//"!Global = !Global + 1\n"
//"end\n";

/*"begin\n"
"	variables a\n"
"	a <- f\n"
"	prints n a\n"
"end\n"
"\n"
"function f\n"
"	variables a\n"
"	a=1\n"
"	prints n a\n"
"	a <- g\n"
"	return a\n"
"end\n"
"\n"
"function g\n"
"	variables a\n"
"	a=0\n"
"	prints n a\n"
"	return a\n"
"end\n";*/


"		begin\n"
"			variables a, t\n"
"			alocar a 50\n"
"		    #Si escribo un poquitin antes?\n"
"		    t = a-1\n"
"		    *t = 522\n"
"\n"
"			liberar a\n"
"		    #Si escribo un sector liberado?\n"
"		    *a = 12444\n"
"		end\n";

t_log *logger;
void *cpu_readMemoryDummy(uint32_t pid, uint32_t page, uint32_t offset, uint32_t size){
	void *buffer = malloc(size);

	memcpy(buffer,myMemory+(page*myCPU.pageSize)+offset,size);

	return buffer;
}
uint32_t cpu_writeMemoryDummy(uint32_t pid, uint32_t page, uint32_t offset, uint32_t size, void *buffer){
	memcpy(myMemory+(page*myCPU.pageSize)+offset,buffer,size);
}
void *cpu_readMemory(int pid, int page, int offset, int size) {
	if(myCPU.connections[T_MEMORY].isDummy) return cpu_readMemoryDummy(pid,page,offset,size);
	int fd = myCPU.connections[T_MEMORY].socketFileDescriptor;

	ipc_client_sendMemoryRead(fd, pid, page, offset, size);
	return ipc_client_waitMemoryReadResponse(fd)->buffer;
}

void cpu_writeMemory(int pid, int page, int offset, int size, void *buffer) {
	if(myCPU.connections[T_MEMORY].isDummy){
		cpu_writeMemoryDummy(pid,page,offset,size,buffer);
	}else{
		int fd = myCPU.connections[T_MEMORY].socketFileDescriptor;

		ipc_client_sendMemoryWrite(fd, pid, page, offset, size, buffer);
		ipc_struct_memory_write_response response;
		recv(myCPU.connections[T_MEMORY].socketFileDescriptor,&response,sizeof(ipc_struct_get_shared_variable_response),0);
	}

}
uint32_t cpu_start(t_CPU *CPU){
	CPU->assignedPCB = NULL;
	CPU->connections[T_KERNEL].host = "127.0.0.1";
	CPU->connections[T_KERNEL].portNumber = 5001;
	CPU->connections[T_KERNEL].server = 0;
	CPU->connections[T_KERNEL].socketFileDescriptor = 0;
	CPU->connections[T_KERNEL].status = DISCONNECTED;
	CPU->connections[T_MEMORY].host = "127.0.0.1";
	CPU->connections[T_MEMORY].portNumber = 5003;
	CPU->connections[T_MEMORY].server = 0;
	CPU->connections[T_MEMORY].socketFileDescriptor = 0;
	CPU->connections[T_MEMORY].status = DISCONNECTED;
	CPU->currentInstruction = 0;
	CPU->instructionPointer = 0;
	CPU->variableCounter = 0;
	CPU->status = WAITING;

	return 0;
}
uint32_t cpu_connect(t_CPU *aCPU, t_connectionType connectionType){
	t_CPUConnection *kernelConnection;
	t_CPUConnection *memoryConnection;
	switch (connectionType) {
		case T_D_MEMORY:
			printf("\nDummy");
			printf("\nConnecting to DummyMemory\n");
			myMemory = malloc(512*4);
			memset(myMemory,0,512*4);
			aCPU->connections[1].portNumber = 9999;
			aCPU->connections[1].status = CONNECTED;
			aCPU->connections[T_MEMORY].isDummy = 1;
			printf("\nConnected to memory\n");
			return 0;
			break;
		case T_KERNEL:
			kernelConnection = &aCPU->connections[T_KERNEL];
			printf("\nServer Ip: %s Port No: %d", kernelConnection->host, kernelConnection->portNumber);
			printf("\nConnecting to Kernel\n");

			kernelConnection->socketFileDescriptor = socket(AF_INET, SOCK_STREAM, 0);
			   if (kernelConnection->socketFileDescriptor < 0) {
				  perror("ERROR opening socket");
				  exit(1);
			   }

			   kernelConnection->server = gethostbyname(kernelConnection->host);
			   bzero((char *) &(kernelConnection->serv_addr), sizeof(kernelConnection->serv_addr));
			   kernelConnection->serv_addr.sin_family = AF_INET;
			   bcopy((char *)kernelConnection->server->h_addr, (char *)&(kernelConnection->serv_addr.sin_addr.s_addr), kernelConnection->server->h_length);
			   kernelConnection->serv_addr.sin_port = htons(kernelConnection->portNumber);

			   // Now connect to the server
			   if (connect(kernelConnection->socketFileDescriptor, (struct sockaddr*)&(kernelConnection->serv_addr), sizeof(kernelConnection->serv_addr)) < 0) {
				  perror("ERROR connecting");
				  exit(1);
			   }

			   // Send handshake and wait for response
			   ipc_client_sendHandshake(CPU, myCPU.connections[T_KERNEL].socketFileDescriptor);
			   ipc_struct_handshake_response *response = ipc_client_waitHandshakeResponse(myCPU.connections[T_KERNEL].socketFileDescriptor);
			   free(response);
			   kernelConnection->status = CONNECTED;
			   aCPU->connections[T_KERNEL].isDummy = 0;
			   printf("CONNECTED TO KERNEL");

			break;
		case T_D_KERNEL:
			printf("\nConnecting to Dummy Kernel\n");
			aCPU->connections[KERNEL].status = CONNECTED;
			aCPU->connections[T_KERNEL].isDummy = 1;
			printf("\nConnected to Dummy Kernel\n");
			break;
		case T_MEMORY:
			memoryConnection = &aCPU->connections[T_MEMORY];
			printf("\nServer Ip: %s Port No: %d", memoryConnection->host, memoryConnection->portNumber);
			printf("\nConnecting to Memory\n");

			memoryConnection->socketFileDescriptor = socket(AF_INET, SOCK_STREAM, 0);
			   if (memoryConnection->socketFileDescriptor < 0) {
				  perror("ERROR opening socket");
				  exit(1);
			   }

			   memoryConnection->server = gethostbyname(memoryConnection->host);
			   bzero((char *) &(memoryConnection->serv_addr), sizeof(memoryConnection->serv_addr));
			   memoryConnection->serv_addr.sin_family = AF_INET;
			   bcopy((char *)memoryConnection->server->h_addr, (char *)&(memoryConnection->serv_addr.sin_addr.s_addr), memoryConnection->server->h_length);
			   memoryConnection->serv_addr.sin_port = htons(memoryConnection->portNumber);

			   // Now connect to the server
			   if (connect(memoryConnection->socketFileDescriptor, (struct sockaddr*)&(memoryConnection->serv_addr), sizeof(memoryConnection->serv_addr)) < 0) {
				  perror("ERROR connecting");
				  exit(1);
			   }

			   ipc_client_sendHandshake(CPU, memoryConnection->socketFileDescriptor);
			   response = ipc_client_waitHandshakeResponse(memoryConnection->socketFileDescriptor);
			   myCPU.pageSize = response->info;
			   free(response);
			   memoryConnection->status = CONNECTED;
			   aCPU->connections[T_MEMORY].isDummy = 0;
			   printf("CONNECTED TO Memory");
			break;
		default:
			break;
	}


}
t_PCB *cpu_createPCBFromScript(char *script){
	t_PCB *PCB = malloc(sizeof(t_PCB));

	// prepare PCB
	int programLength = string_length(script);
	t_metadata_program *program = metadata_desde_literal(script);

	int codePagesCount = programLength / myCPU.pageSize;
	int instructionCount = program->instrucciones_size;

	PCB->variableSize.stackArgumentCount = 0;
	PCB->variableSize.stackIndexRecordCount = 0;
	PCB->variableSize.stackVariableCount = 0;

	PCB->codePages = codePagesCount;

	PCB->variableSize.instructionCount = instructionCount;
	PCB->codeIndex = malloc(instructionCount * sizeof(t_codeIndex));
	memcpy(PCB->codeIndex,program->instrucciones_serializado,instructionCount * sizeof(t_codeIndex));

	PCB->variableSize.labelIndexSize = program->etiquetas_size;
	PCB->labelIndex = malloc(program->etiquetas_size);
	memcpy(PCB->labelIndex,program->etiquetas,program->etiquetas_size);

	PCB->ec = 0;
	PCB->filesTable = NULL;
	PCB->pc = 0;
	PCB->pid = rand();
	PCB->sp = PCB->codePages +1 * myCPU.pageSize;
	PCB->stackIndex = NULL;

	return PCB;
}
uint32_t cpu_declareVariable(char variableName){
	log_debug(logger,"declare variable %c",variableName);
	t_variableAdd type = pcb_getVariableAddTypeFromTag(variableName);
	t_stackVariable *variable = malloc(sizeof(t_stackVariable));
	int dataBasePage = myCPU.assignedPCB->codePages;
	t_PCBVariableSize *variableSize = &myCPU.assignedPCB->variableSize;
	int currentVariableSpaceInMemory = (variableSize->stackVariableCount + variableSize->stackArgumentCount) * sizeof(int);
	int currentVariablePagesInMemory = 0;



	variable->id = variableName;
	variable->page = dataBasePage;
	if(currentVariableSpaceInMemory != 0){
		currentVariablePagesInMemory = currentVariableSpaceInMemory / myCPU.pageSize;
		variable->page += currentVariablePagesInMemory;
		variable->offset = currentVariableSpaceInMemory - (currentVariablePagesInMemory * myCPU.pageSize);
	}else{
		variable->offset = currentVariableSpaceInMemory;
	}
	variable->size = sizeof(int);

	pcb_addStackIndexVariable(myCPU.assignedPCB,variable,type);
	return currentVariableSpaceInMemory;
}
void cpu_endProgram(){
	log_debug(logger,"endProgram");
	myCPU.status = WAITING;
	myCPU.instructionPointer = 0;
	myCPU.variableCounter = 0;
	pcb_dump(myCPU.assignedPCB);
	pcb_destroy(myCPU.assignedPCB);
	myCPU.assignedPCB = NULL;
}
uint32_t cpu_getVariablePosition(char variableName){
	//printf("getVariablePosition\n");

	t_variableAdd type = pcb_getVariableAddTypeFromTag(variableName);

	t_stackVariable *variable = pcb_getVariable(myCPU.assignedPCB,variableName);

	uint32_t returnValue = cpu_getPointerFromVariableReference(*variable);
	log_debug(logger,"referenced address: %d for variable: %c",returnValue,variableName);
	return returnValue;
}
int cpu_dereference(uint32_t variableAddress){
	//rintf("dereference\n");
	t_stackVariable variable;
	cpu_getVariableReferenceFromPointer(variableAddress,&variable);
	int *valueBuffer = cpu_readMemory(myCPU.assignedPCB->pid,variable.page,variable.offset,sizeof(int));
	int value = *valueBuffer;
	free(valueBuffer);
	log_debug(logger,"dereferenced address: %d to value: %d",variableAddress, value);
	return value;
}
void cpu_assignValue(uint32_t variableAddress, int value){
	//printf("assignValue\n");
	log_debug(logger,"assign value: %d to address: %d", value, variableAddress);
	t_stackVariable variable;
	cpu_getVariableReferenceFromPointer(variableAddress,&variable);
	int valueBuffer = value;
	cpu_writeMemory(myCPU.assignedPCB->pid,variable.page,variable.offset,sizeof(int),&valueBuffer);
}
void cpu_gotoLabel(char *label){
	log_debug(logger,"goto: %s",label);
	int labelIndexIteratorSize = 0;
	int labelIndexIterator = 0;
	char *buffer;
	int labelLength = 0;
	int pointer;


	while(labelIndexIteratorSize < myCPU.assignedPCB->variableSize.labelIndexSize){
		printf("label %d - tag: %s\n",labelIndexIterator,(myCPU.assignedPCB->labelIndex)+labelIndexIteratorSize);
		labelLength = strlen(myCPU.assignedPCB->labelIndex) + 1;
		buffer = malloc(labelLength);
		memcpy(buffer,myCPU.assignedPCB->labelIndex,labelLength+1);
		labelIndexIteratorSize += labelLength;

		memcpy(&pointer,myCPU.assignedPCB->labelIndex+labelIndexIteratorSize,sizeof(int));
		printf("dir: %d\n", pointer);
		labelIndexIteratorSize += sizeof(int);

		if(strcmp(buffer,label)){
			myCPU.instructionPointer = pointer-1;
		}

		labelIndexIterator++;
	}
}
void cpu_callNoReturn(char *label){
	log_debug(logger,"callNoReturn");
}
void cpu_callWithReturn(char *label, uint32_t returnAddress){
	log_debug(logger,"call %s with return address: %d", label, returnAddress);
	t_stackIndexRecord *record = malloc(sizeof(t_stackIndexRecord));
	t_stackVariable variable;
	cpu_getVariableReferenceFromPointer(returnAddress,&variable);

	record->returnPosition = myCPU.instructionPointer;
	record->returnVariable.offset = variable.offset;
	record->returnVariable.page = variable.page;
	record->returnVariable.size = sizeof(int);

	myCPU.instructionPointer = pcb_getDirectionFromLabel(myCPU.assignedPCB,label) - 1;

	pcb_addSpecificStackIndexRecord(myCPU.assignedPCB,record);
}
void cpu_return(int returnValue){
	log_debug(logger,"return value: %d",returnValue);
	int returnValueCopy = returnValue;
	int currentStackLevel = myCPU.assignedPCB->variableSize.stackIndexRecordCount -1;

	// get currrent stack
	t_stackIndexRecord *record = list_get(myCPU.assignedPCB->stackIndex,currentStackLevel);

	//set instruction pointer
	myCPU.instructionPointer = record->returnPosition;

	// get current stack size
	t_PCBVariableSize variableSize = pcb_getSizeOfSpecificStack(record);

	//write return value in return variable
	cpu_writeMemory(myCPU.assignedPCB->pid,record->returnVariable.page,record->returnVariable.offset,record->returnVariable.size,&returnValueCopy);

	//decompile stack
	pcb_decompileStack(myCPU.assignedPCB);

	printf("return\n");
}

ipc_struct_kernel_semaphore_wait_response ipc_sendSemaphoreWait(int fd, char *identifier) {
	ipc_struct_kernel_semaphore_wait *wait = malloc(sizeof(ipc_struct_kernel_semaphore_wait));
	char * identifierCopy = strdup(identifier);

	uint32_t pcbSize = pcb_getBufferSizeFromVariableSize(&myCPU.assignedPCB->variableSize);
	wait->header.operationIdentifier = KERNEL_SEMAPHORE_WAIT;
	wait->identifierLength = strlen(identifier)+1;
	wait->identifier = malloc(strlen(identifier) + 1);
	memcpy(wait->identifier, identifier, strlen(identifier) + 1);
	wait->serializedLength = pcbSize;
	wait->pcb = pcb_serializePCB(myCPU.assignedPCB);

	int bufferSize = 0;
	int bufferOffset = 0;
	bufferSize = sizeof(ipc_header) + sizeof(int) + strlen(identifier) + pcbSize;
	char *buffer = malloc(bufferSize);

	memset(buffer,0,bufferSize);

	memcpy(buffer+bufferOffset,&wait->header,sizeof(ipc_header));
	bufferOffset += sizeof(ipc_header);

	memcpy(buffer+bufferOffset,&wait->identifierLength,sizeof(int));
	bufferOffset += sizeof(int);

	memcpy(buffer+bufferOffset,identifierCopy,strlen(identifier)+1);
	bufferOffset += strlen(identifier)+1;

	memcpy(buffer+bufferOffset,wait->pcb,pcbSize);
	bufferOffset += pcbSize;

	send(fd, buffer, bufferSize, 0);

	ipc_struct_kernel_semaphore_wait_response response;
	recv(fd, &response, sizeof(ipc_struct_kernel_semaphore_wait_response), 0);

	return response;
}
ipc_struct_kernel_semaphore_signal ipc_sendSemaphoreSignal(int fd, char *identifier){
	ipc_struct_kernel_semaphore_signal*signal = malloc(sizeof(ipc_struct_kernel_semaphore_signal));
	char * identifierCopy = strdup(identifier);

	signal->header.operationIdentifier = KERNEL_SEMAPHORE_SIGNAL;
	signal->identifierLength = strlen(identifier)+1;
	signal->identifier = malloc(strlen(identifier) + 1);
	memcpy(signal->identifier, identifier, strlen(identifier) + 1);

	int bufferSize = 0;
	int bufferOffset = 0;
	bufferSize = sizeof(ipc_header) + sizeof(int) + strlen(identifier)+1;
	char *buffer = malloc(bufferSize);

	memset(buffer,0,bufferSize);

	memcpy(buffer+bufferOffset,&signal->header,sizeof(ipc_header));
	bufferOffset += sizeof(ipc_header);

	memcpy(buffer+bufferOffset,&signal->identifierLength,sizeof(int));
	bufferOffset += sizeof(int);

	memcpy(buffer+bufferOffset,identifierCopy,strlen(identifier)+1);
	bufferOffset += strlen(identifier)+1;

	send(fd, buffer, bufferSize, 0);

	return *signal;
}
ipc_struct_kernel_alloc_heap_response ipc_sendKernelAlloc(int fd, ipc_struct_kernel_alloc_heap *request){
	int bufferSize = sizeof(ipc_struct_kernel_alloc_heap);
	int bufferOffset = 0;
	char *buffer = malloc(bufferSize);
	memset(buffer,0,bufferSize);

	memcpy(buffer+bufferOffset,&request->header,sizeof(ipc_header));
	bufferOffset += sizeof(ipc_header);

	memcpy(buffer+bufferOffset,&request->processID,sizeof(int));
	bufferOffset += sizeof(int);

	memcpy(buffer+bufferOffset,&request->numberOfBytes,sizeof(int));
	bufferOffset += sizeof(int);

	send(fd, buffer, bufferSize, 0);

	ipc_struct_kernel_alloc_heap_response response;
	recv(fd, &response, sizeof(ipc_struct_kernel_alloc_heap_response), 0);

	return response;

}
ipc_struct_kernel_dealloc_heap_response ipc_sendKernelFree(int fd, uint32_t pointer){
	ipc_struct_kernel_dealloc_heap dealloc;

	dealloc.header.operationIdentifier = KERNEL_DEALLOC_HEAP;
	dealloc.processID = myCPU.assignedPCB->pid;
	t_stackVariable variable;
	cpu_getVariableReferenceFromPointer(pointer,&variable);
	dealloc.pageNumber = variable.page;
	dealloc.offset = variable.offset;

	send(fd, &dealloc, sizeof(ipc_struct_kernel_dealloc_heap), 0);

	ipc_struct_kernel_dealloc_heap_response response;
	recv(fd, &response, sizeof(ipc_struct_kernel_dealloc_heap_response), 0);

	return response;

}

char *cpu_getFlagsStringFromFlags(t_flags flags){
	char *output = malloc(sizeof(char)*4);
	int iterator = 0;

	if(flags.read){
		output[iterator]='r';
		iterator++;
	}

	if(flags.write){
		output[iterator]='w';
		iterator++;
	}

	if(flags.creation){
		output[iterator]='c';
		iterator++;
	}

	output[iterator] = '\0';
	return output;
}

ipc_struct_kernel_open_file_response ipc_sendKernelOpenFile(int fd, char *path, t_flags flags){
	int bufferSize = sizeof(ipc_struct_kernel_open_file) + strlen(path)+1 - (sizeof(int));
	char *buffer = malloc(bufferSize);
	memset(buffer,0,bufferSize);
	char *flagsBuffer = cpu_getFlagsStringFromFlags(flags);

	int bufferOffset = 0;
	ipc_struct_kernel_open_file request;

	request.header.operationIdentifier = KERNEL_OPEN_FILE;
	request.pathLength = strlen(path)+1;

	memcpy(buffer+bufferOffset,&request.header,sizeof(ipc_header));
	bufferOffset += sizeof(ipc_header);

	memcpy(buffer+bufferOffset,&request.pathLength,sizeof(int));
	bufferOffset += sizeof(int);

	char *pathCopy = strdup(path);
	memcpy(buffer+bufferOffset,pathCopy,request.pathLength);
	bufferOffset += request.pathLength;
	free(pathCopy);

	memcpy(buffer+bufferOffset,&myCPU.assignedPCB->pid,sizeof(uint32_t));
	bufferOffset += sizeof(uint32_t);

	memcpy(buffer+bufferOffset,flagsBuffer,4*sizeof(char));
	bufferOffset += 4*sizeof(char);

	send(fd, buffer, bufferSize, 0);

	ipc_struct_kernel_open_file_response response;
	recv(fd, &response, sizeof(ipc_struct_kernel_open_file_response), 0);

	return response;

}
ipc_struct_kernel_delete_file_response ipc_sendKernelDeleteFile(int fd, int fileDescriptor){
	int bufferSize = sizeof(ipc_struct_kernel_delete_file);
	char *buffer = malloc(bufferSize);
	memset(buffer,0,bufferSize);

	int bufferOffset = 0;
	ipc_struct_kernel_delete_file request;

	request.header.operationIdentifier = KERNEL_DELETE_FILE;
	request.fileDescriptor = fileDescriptor;

	memcpy(buffer+bufferOffset,&request.header,sizeof(ipc_header));
	bufferOffset += sizeof(ipc_header);

	memcpy(buffer+bufferOffset,&request.fileDescriptor,sizeof(int));
	bufferOffset += sizeof(int);

	send(fd, buffer, bufferSize, 0);

	ipc_struct_kernel_delete_file_response response;
	recv(fd, &response, sizeof(ipc_struct_kernel_delete_file_response), 0);

	return response;

}
ipc_struct_kernel_close_file_response ipc_sendKernelCloseFile(int fd, int fileDescriptor){
	int bufferSize = sizeof(ipc_struct_kernel_close_file);
	char *buffer = malloc(bufferSize);
	memset(buffer,0,bufferSize);

	int bufferOffset = 0;
	ipc_struct_kernel_close_file request;

	request.header.operationIdentifier = KERNEL_CLOSE_FILE;
	request.fileDescriptor = fileDescriptor;

	memcpy(buffer+bufferOffset,&request.header,sizeof(ipc_header));
	bufferOffset += sizeof(ipc_header);

	memcpy(buffer+bufferOffset,&request.fileDescriptor,sizeof(int));
	bufferOffset += sizeof(int);

	send(fd, buffer, bufferSize, 0);

	ipc_struct_kernel_close_file_response response;

	return response;

}
ipc_struct_kernel_move_file_cursor_response ipc_sendKernelMoveFileCursor(int fd, int fileDescriptor, int position){
	int bufferSize = sizeof(ipc_struct_kernel_move_file_cursor);
	char *buffer = malloc(bufferSize);
	memset(buffer,0,bufferSize);

	int bufferOffset = 0;
	ipc_struct_kernel_move_file_cursor request;

	request.header.operationIdentifier = KERNEL_MOVE_FILE_CURSOR;
	request.fileDescriptor = fileDescriptor;
	request.position = position;
	request.pid = myCPU.assignedPCB->pid;

	memcpy(buffer+bufferOffset,&request.header,sizeof(ipc_header));
	bufferOffset += sizeof(ipc_header);

	memcpy(buffer+bufferOffset,&request.fileDescriptor,sizeof(int));
	bufferOffset += sizeof(int);

	memcpy(buffer+bufferOffset,&request.position,sizeof(int));
	bufferOffset += sizeof(int);

	memcpy(buffer+bufferOffset, &request.pid, sizeof(uint32_t));

	send(fd, buffer, bufferSize, 0);

	ipc_struct_kernel_move_file_cursor_response response;
	recv(fd, &response, sizeof(ipc_struct_kernel_move_file_cursor_response), 0);

	return response;

}
ipc_struct_kernel_write_file_response ipc_sendKernelWriteFile(int fd, int fileDescriptor, int size, char *content){
	int bufferSize = sizeof(ipc_struct_kernel_write_file) + size +1;
	char *buffer = malloc(bufferSize);
	memset(buffer,0,bufferSize);

	int bufferOffset = 0;
	ipc_struct_kernel_write_file request;

	request.header.operationIdentifier = KERNEL_WRITE_FILE;
	request.pid = myCPU.assignedPCB->pid;
	request.fileDescriptor = fileDescriptor;
	request.size = size;
	request.buffer = strdup(content);

	memcpy(buffer+bufferOffset,&request.header,sizeof(ipc_header));
	bufferOffset += sizeof(ipc_header);

	memcpy(buffer+bufferOffset,&request.pid,sizeof(int));
	bufferOffset += sizeof(int);

	memcpy(buffer+bufferOffset,&request.fileDescriptor,sizeof(int));
	bufferOffset += sizeof(int);

	memcpy(buffer+bufferOffset,&request.size,sizeof(int));
	bufferOffset += sizeof(int);

	memcpy(buffer+bufferOffset,&request.buffer,size+1);
	bufferOffset += strlen(content)+1;
	free(request.buffer);

	send(fd, buffer, bufferSize, 0);

	ipc_struct_kernel_write_file_response response;
	recv(fd, &response, sizeof(ipc_struct_kernel_write_file_response), 0);

	return response;

}
ipc_struct_kernel_read_file_response ipc_sendKernelReadFile(int fd, int fileDescriptor, uint32_t valuePointer, int size){
	int bufferSize = sizeof(ipc_struct_kernel_read_file);
	char *buffer = malloc(bufferSize);
	memset(buffer,0,bufferSize);

	int bufferOffset = 0;
	ipc_struct_kernel_read_file request;

	request.header.operationIdentifier = KERNEL_READ_FILE;
	request.pid = myCPU.assignedPCB->pid;
	request.fileDescriptor = fileDescriptor;
	request.size = size;
	request.valuePointer = valuePointer;


	memcpy(buffer+bufferOffset,&request.header,sizeof(ipc_header));
	bufferOffset += sizeof(ipc_header);

	memcpy(buffer+bufferOffset,&request.pid,sizeof(int));
	bufferOffset += sizeof(int);

	memcpy(buffer+bufferOffset,&request.fileDescriptor,sizeof(int));
	bufferOffset += sizeof(int);

	memcpy(buffer+bufferOffset,&request.valuePointer,sizeof(uint32_t));
	bufferOffset += sizeof(uint32_t);

	memcpy(buffer+bufferOffset,&request.size,sizeof(int));
	bufferOffset += sizeof(int);


	send(fd, buffer, bufferSize, 0);

	ipc_struct_kernel_read_file_response response;
	recv(fd, &response, sizeof(ipc_struct_kernel_read_file_response), 0);

	return response;

}

void cpu_kernelWait(char *semaphoreId){
	printf("kernelWait\n");
	fflush(stdout);
	myCPU.assignedPCB->pc = myCPU.instructionPointer;
	ipc_struct_kernel_semaphore_wait_response response = ipc_sendSemaphoreWait(myCPU.connections[T_KERNEL].socketFileDescriptor, semaphoreId);
	if(response.shouldBlock == 1) myCPU.status = WAITING;
}
void cpu_kernelSignal(char *semaphoreId){
	printf("kernelSignal\n");
	ipc_struct_kernel_semaphore_signal response = ipc_sendSemaphoreSignal(myCPU.connections[T_KERNEL].socketFileDescriptor, semaphoreId);
	fflush(stdout);
}
uint32_t cpu_kernelAlloc(int size){
	log_debug(logger,"Requested: KERNEL to ALLOC bytes: %d",size);
	ipc_struct_kernel_alloc_heap request;
	request.header.operationIdentifier = KERNEL_ALLOC_HEAP;
	request.numberOfBytes = size;
	request.processID = myCPU.assignedPCB->pid;
	ipc_struct_kernel_alloc_heap_response response = ipc_sendKernelAlloc(myCPU.connections[T_KERNEL].socketFileDescriptor,&request);
	fflush(stdout);
	t_stackVariable variable;
	variable.page = response.pageNumber;
	variable.offset = response.offset;
	return cpu_getPointerFromVariableReference(variable);
}
void cpu_kernelFree(uint32_t pointer){
	log_debug(logger,"Requested: KERNEL to FREE address: %d",pointer);
	ipc_struct_kernel_dealloc_heap_response response = ipc_sendKernelFree(myCPU.connections[T_KERNEL].socketFileDescriptor,pointer);
	fflush(stdout);
}
uint32_t cpu_kernelOpen(char *address, t_flags flags){
	ipc_struct_kernel_open_file_response response = ipc_sendKernelOpenFile(myCPU.connections[T_KERNEL].socketFileDescriptor, address,flags);

	fflush(stdout);
	log_debug(logger,"Requested: KERNEL to open file: %s with flags(rwc):%d%d%d",address,flags.read,flags.write,flags.creation);
	return response.fileDescriptor;
}
void cpu_kernelDelete(uint32_t fileDescriptor){
	printf("kernelDelete\n");
	ipc_struct_kernel_delete_file_response response = ipc_sendKernelDeleteFile(myCPU.connections[T_KERNEL].socketFileDescriptor,fileDescriptor);
	log_debug(logger,"Requested: KERNEL to delete fd: %d",fileDescriptor);
	fflush(stdout);
}
void cpu_kernelClose(uint32_t fileDescriptor){
	log_debug(logger,"Requested: KERNEL to close fd: %d",fileDescriptor);
	ipc_struct_kernel_close_file_response response = ipc_sendKernelCloseFile(myCPU.connections[T_KERNEL].socketFileDescriptor,fileDescriptor);

	fflush(stdout);
}
void cpu_kernelMoveCursor(uint32_t fileDescriptor, int position){
	printf("kernelMoveCursor\n");
	ipc_struct_kernel_move_file_cursor_response response = ipc_sendKernelMoveFileCursor(myCPU.connections[T_KERNEL].socketFileDescriptor,fileDescriptor,position);
	log_debug(logger,"Requested: KERNEL to move fd: %d to cursor: %d",fileDescriptor, position);
	fflush(stdout);
}
void cpu_kernelWrite(uint32_t fileDescriptor, void* buffer, int size){
	char *output = malloc(size);
	memcpy(output,buffer,size);
	printf("kernelWrite\n",output);
	printf("%s\n",output);
	fflush(stdout);
	free(output);
	log_debug(logger,"Requested: KERNEL to WRITE fd: %d a total of bytes: %d with content: %s",fileDescriptor,size,(char *)buffer);
	ipc_struct_kernel_write_file_response response = ipc_sendKernelWriteFile(myCPU.connections[T_KERNEL].socketFileDescriptor,fileDescriptor,size,buffer);
}
void cpu_kernelRead(uint32_t fileDescriptor, uint32_t value, int size){
	ipc_struct_kernel_read_file_response response = ipc_sendKernelReadFile(myCPU.connections[T_KERNEL].socketFileDescriptor,fileDescriptor,value,size);
	log_debug(logger,"Requested: KERNEL to READ fd: %d, a total of bytes: %d into address: %d",fileDescriptor,size,value);
	fflush(stdout);
}
t_memoryDirection cpu_getMemoryDirectionFromAddress(uint32_t address){
	t_memoryDirection direction;

	direction.page = address / myCPU.pageSize;
	direction.offset = address - (direction.page * myCPU.pageSize);
	direction.size = sizeof(int);

	return direction;
}
int cpu_sharedVariableGet(char *identifier){
	log_debug(logger,"get shared variable: %s",identifier);
	ipc_client_sendGetSharedVariable(myCPU.connections[T_KERNEL].socketFileDescriptor,identifier);
	int value = ipc_client_waitSharedVariableValue(myCPU.connections[T_KERNEL].socketFileDescriptor);
	log_debug(logger,"global variable value: %d", value);
	return value;
}
int cpu_sharedVariableSet(char *identifier, int value){
	log_debug(logger,"sharedVariable: %s Set: %d",identifier,value);
}
int cpu_receivePCB(){
	if(myCPU.connections[T_KERNEL].isDummy){
		char *testProgram = strdup(program);
		printf("writing program: %s", testProgram);
		memcpy(myMemory,testProgram,strlen(testProgram));

		myCPU.assignedPCB = cpu_createPCBFromScript(testProgram);
		myCPU.status = RUNNING;
		myCPU.quantum = 9999;
		return 0;
	}

	ipc_header header;
	recv(myCPU.connections[T_KERNEL].socketFileDescriptor, &header, sizeof(ipc_header), 0);

	int quantum;
	recv(myCPU.connections[T_KERNEL].socketFileDescriptor, &quantum, sizeof(int), 0);

	int serializedSize;
	recv(myCPU.connections[T_KERNEL].socketFileDescriptor, &serializedSize, sizeof(int), 0);

	t_PCBVariableSize *variableSize = malloc(sizeof(t_PCBVariableSize));
	recv(myCPU.connections[T_KERNEL].socketFileDescriptor, variableSize, sizeof(t_PCBVariableSize), MSG_PEEK);

	uint32_t serializedPCBSize = pcb_getBufferSizeFromVariableSize(variableSize);

	void *pcbBuffer = malloc(serializedPCBSize);
	recv(myCPU.connections[T_KERNEL].socketFileDescriptor, pcbBuffer, serializedPCBSize, 0);
	t_PCB *pcbToExecute = pcb_deSerializePCB(pcbBuffer, variableSize);

	myCPU.assignedPCB = pcbToExecute;
	myCPU.instructionPointer = myCPU.assignedPCB->pc;
	myCPU.quantum = quantum;
	myCPU.status = RUNNING;
	return 0;
}

void cpu_getVariableReferenceFromPointer(uint32_t pointer, t_stackVariable *variable){
	variable->offset = (uint16_t) pointer;
	variable->page = pointer >> 16;
}
uint32_t cpu_getPointerFromVariableReference(t_stackVariable variable){
	return ((uint16_t) variable.page << 16) | variable.offset;
}


 AnSISOP_funciones functions = {
 		.AnSISOP_definirVariable		= cpu_declareVariable,
 		.AnSISOP_obtenerPosicionVariable= cpu_getVariablePosition,
 		.AnSISOP_finalizar 				= cpu_endProgram,
 		.AnSISOP_dereferenciar			= cpu_dereference,
 		.AnSISOP_asignar				= cpu_assignValue,
 		.AnSISOP_irAlLabel				= cpu_gotoLabel,
 		.AnSISOP_llamarConRetorno		= cpu_callWithReturn,
 		.AnSISOP_llamarSinRetorno 		= cpu_callNoReturn,
 		.AnSISOP_retornar				= cpu_return,
 		.AnSISOP_obtenerValorCompartida = cpu_sharedVariableGet,
 		.AnSISOP_asignarValorCompartida = cpu_sharedVariableSet,

 };

 AnSISOP_kernel kernel_functions = {
		 .AnSISOP_abrir					= cpu_kernelOpen,
		 .AnSISOP_borrar				= cpu_kernelDelete,
		 .AnSISOP_cerrar				= cpu_kernelClose,
		 .AnSISOP_escribir				= cpu_kernelWrite,
		 .AnSISOP_leer					= cpu_kernelRead,
		 .AnSISOP_liberar				= cpu_kernelFree,
		 .AnSISOP_moverCursor			= cpu_kernelMoveCursor,
		 .AnSISOP_reservar				= cpu_kernelAlloc,
		 .AnSISOP_signal				= cpu_kernelSignal,
		 .AnSISOP_wait					= cpu_kernelWait,
 };

int main() {
	char *logfile = tmpnam(NULL);
	logger = log_create(logfile,"CPU",1,LOG_LEVEL_DEBUG);
	// start CPU
	cpu_start(&myCPU);

	// connect to memory -- DUMMY
	cpu_connect(&myCPU,T_MEMORY);
	cpu_connect(&myCPU,T_KERNEL);

	while (1) {
		myCPU.status = WAITING;
		log_debug(logger,"Cpu waiting PCB");
		//wait for PCB
		cpu_receivePCB();
		log_debug(logger,"PCB received");
		while(myCPU.status == RUNNING && myCPU.quantum > 0){
			   //cpu fetch
			   t_codeIndex currentInstructionIndex = myCPU.assignedPCB->codeIndex[myCPU.instructionPointer];
			   int page = currentInstructionIndex.start / myCPU.pageSize;
			   int offset = currentInstructionIndex.start - (page * myCPU.pageSize);

			   char *instruction = cpu_readMemory(myCPU.assignedPCB->pid,page,offset,currentInstructionIndex.size);
			   instruction[currentInstructionIndex.size-1]='\0';
			   fflush(stdout);
			   log_debug(logger,"fetched instruction is: %s\n",(char *) instruction);
			   fflush(stdout);

			   //cpu decode & execute
			   analizadorLinea(instruction,&functions,&kernel_functions);

			   myCPU.quantum--;
			   myCPU.instructionPointer++;
		   }
	}


	return EXIT_SUCCESS;
}

