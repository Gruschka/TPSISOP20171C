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

t_log *logger;
void *cpu_readMemoryDummy(uint32_t pid, uint32_t page, uint32_t offset, uint32_t size){
	void *buffer = malloc(size);

	memcpy(buffer,myMemory+(page*DUMMY_MEMORY_PAGE_SIZE)+offset,size);

	return buffer;
}
uint32_t cpu_writeMemoryDummy(uint32_t pid, uint32_t page, uint32_t offset, uint32_t size, void *buffer){
	memcpy(myMemory+(page*DUMMY_MEMORY_PAGE_SIZE)+offset,buffer,size);
}
void *cpu_readMemory(int pid, int page, int offset, int size) {
	int fd = myCPU.connections[T_MEMORY].socketFileDescriptor;

	ipc_client_sendMemoryRead(fd, pid, page, offset, size);
	return ipc_client_waitMemoryReadResponse(fd)->buffer;
}

void cpu_writeMemory(int pid, int page, int offset, int size, void *buffer) {
	int fd = myCPU.connections[T_MEMORY].socketFileDescriptor;

	ipc_client_sendMemoryWrite(fd, pid, page, offset, size, buffer);
}
uint32_t cpu_start(t_CPU *CPU){
	CPU->assignedPCB = NULL;
	CPU->connections[T_KERNEL].host = "127.0.0.1";
	CPU->connections[T_KERNEL].portNumber = 5001;
	CPU->connections[T_KERNEL].server = 0;
	CPU->connections[T_KERNEL].socketFileDescriptor = 0;
	CPU->connections[T_KERNEL].status = DISCONNECTED;
	CPU->connections[T_MEMORY].host = "127.0.0.1";
	CPU->connections[T_MEMORY].portNumber = 8888;
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
			   printf("CONNECTED TO KERNEL");

			break;
		case T_D_KERNEL:
			printf("\nConnecting to Dummy Kernel\n");
			aCPU->connections[KERNEL].status = CONNECTED;
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

//			   ipc_client_sendHandshake(CPU, memoryConnection->socketFileDescriptor);
//			   response = ipc_client_waitHandshakeResponse(memoryConnection->socketFileDescriptor);
			   free(response);
			   memoryConnection->status = CONNECTED;
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

	int codePagesCount = programLength / DUMMY_MEMORY_PAGE_SIZE;
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
	PCB->sp = PCB->codePages +1 * DUMMY_MEMORY_PAGE_SIZE;
	PCB->stackIndex = NULL;

	return PCB;
}
uint32_t cpu_declareVariable(char variableName){
//	printf("declareVariable\n");
	t_variableAdd type = pcb_getVariableAddTypeFromTag(variableName);
	t_stackVariable *variable = malloc(sizeof(t_stackVariable));
	int dataBasePage = myCPU.assignedPCB->codePages +1;
	t_PCBVariableSize *variableSize = &myCPU.assignedPCB->variableSize;
	int currentVariableSpaceInMemory = (variableSize->stackVariableCount + variableSize->stackArgumentCount) * sizeof(int);
	int currentVariablePagesInMemory = 0;



	variable->id = variableName;
	variable->page = dataBasePage;
	if(currentVariableSpaceInMemory != 0){
		currentVariablePagesInMemory = currentVariableSpaceInMemory / DUMMY_MEMORY_PAGE_SIZE;
		variable->page += currentVariablePagesInMemory;
		variable->offset = currentVariableSpaceInMemory - (currentVariablePagesInMemory * DUMMY_MEMORY_PAGE_SIZE);
	}else{
		variable->offset = currentVariableSpaceInMemory;
	}
	variable->size = sizeof(int);

	pcb_addStackIndexVariable(myCPU.assignedPCB,variable,type);
	return currentVariableSpaceInMemory;
}
void cpu_endProgram(){
	printf("endProgram\n");
	myCPU.status = FINISHED;
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

	int position = (variable->page * DUMMY_MEMORY_PAGE_SIZE) + variable->offset;
	return position;
}
int cpu_dereference(uint32_t variableAddress){
	//rintf("dereference\n");
	int *valueBuffer = cpu_readMemoryDummy(myCPU.assignedPCB->pid,0,variableAddress,sizeof(int));
	int value = *valueBuffer;
	free(valueBuffer);
	return value;
}
void cpu_assignValue(uint32_t variableAddress, int value){
	//printf("assignValue\n");
	int valueBuffer = value;
	cpu_writeMemoryDummy(myCPU.assignedPCB->pid,0,variableAddress,sizeof(int),&valueBuffer);
}
void cpu_gotoLabel(char *label){
	printf("gotoLabel\n");
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
	printf("callNoReturn\n");
}
void cpu_callWithReturn(char *label, uint32_t returnAddress){
	t_stackIndexRecord *record = malloc(sizeof(t_stackIndexRecord));


	record->returnPosition = myCPU.instructionPointer;
	record->returnVariable = cpu_getMemoryDirectionFromAddress(returnAddress);

	myCPU.instructionPointer = pcb_getDirectionFromLabel(myCPU.assignedPCB,label) - 1;

	pcb_addSpecificStackIndexRecord(myCPU.assignedPCB,record);
}
void cpu_return(int returnValue){
	int returnValueCopy = returnValue;
	int currentStackLevel = myCPU.assignedPCB->variableSize.stackIndexRecordCount -1;

	// get currrent stack
	t_stackIndexRecord *record = list_get(myCPU.assignedPCB->stackIndex,currentStackLevel);

	//set instruction pointer
	myCPU.instructionPointer = record->returnPosition;

	// get current stack size
	t_PCBVariableSize variableSize = pcb_getSizeOfSpecificStack(record);

	//write return value in return variable
	cpu_writeMemoryDummy(myCPU.assignedPCB->pid,record->returnVariable.page,record->returnVariable.offset,record->returnVariable.size,&returnValueCopy);

	//decompile stack
	pcb_decompileStack(myCPU.assignedPCB);

	printf("return\n");
}
void cpu_kernelWait(char *semaphoreId){
	printf("kernelWait\n");
	fflush(stdout);
}
void cpu_kernelSignal(char *semaphoreId){
	printf("kernelSignal\n");
	fflush(stdout);
}
uint32_t cpu_kernelAlloc(int size){
	printf("kernelAlloc\n");
	fflush(stdout);
}
void cpu_kernelFree(uint32_t pointer){
	printf("kernelFree\n");
	fflush(stdout);
}
uint32_t cpu_kernelOpen(char *address, t_flags flags){
	printf("kernelOpen\n");
	fflush(stdout);
}
void cpu_kernelDelete(uint32_t fileDescriptor){
	printf("kernelDelete\n");
	fflush(stdout);
}
void cpu_kernelClose(uint32_t fileDescriptor){
	printf("kernelClose\n");
	fflush(stdout);
}
void cpu_kernelMoveCursor(uint32_t fileDescriptor, int position){
	printf("kernelMoveCursor\n");
	fflush(stdout);
}
void cpu_kernelWrite(uint32_t fileDescriptor, void* buffer, int size){
	char *output = malloc(size);
	memcpy(output,buffer,size);
	printf("%s\n",output);
	fflush(stdout);
	free(output);
}
void cpu_kernelRead(uint32_t fileDescriptor, uint32_t value, int size){
	printf("kernelRead\n");
	fflush(stdout);
}
t_memoryDirection cpu_getMemoryDirectionFromAddress(uint32_t address){
	t_memoryDirection direction;

	direction.page = address / DUMMY_MEMORY_PAGE_SIZE;
	direction.offset = address - (direction.page * DUMMY_MEMORY_PAGE_SIZE);
	direction.size = sizeof(int);

	return direction;
}
int cpu_sharedVariableGet(char *identifier){
	printf("sharedVariableGet identifier: %s\n", identifier);
	ipc_client_sendGetSharedVariable(myCPU.connections[T_KERNEL].socketFileDescriptor,identifier);
	int value = ipc_client_waitSharedVariableValue(myCPU.connections[T_KERNEL].socketFileDescriptor);
	printf("global variable value: %d", value);
	return value;
}
int cpu_sharedVariableSet(char *identifier, int value){
	printf("sharedVariableSet");
}

char *program =
"#!/usr/bin/ansisop\n"
"\n"
"begin\n"
"!Global = !Global + 1\n"
"end\n";


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
//	char *testProgram = strdup(program);
//	//printf("writing program: %s", testProgram);
//	memcpy(myMemory,testProgram,strlen(testProgram));



   //wait for PCB
	while (1) {
		myCPU.status = WAITING;

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
		myCPU.quantum = quantum;
		myCPU.status = RUNNING;

		while(myCPU.status == RUNNING && myCPU.quantum > 0){
			   //cpu fetch
			   t_codeIndex currentInstructionIndex = myCPU.assignedPCB->codeIndex[myCPU.instructionPointer];
			   int page = currentInstructionIndex.start / DUMMY_MEMORY_PAGE_SIZE;
			   int offset = currentInstructionIndex.start - (page * DUMMY_MEMORY_PAGE_SIZE);

			   char *instruction = cpu_readMemoryDummy(myCPU.assignedPCB->pid,page,offset,currentInstructionIndex.size);
			   instruction[currentInstructionIndex.size]='\0';
			   fflush(stdout);
			   printf("fetched instruction is: %s\n",(char *) instruction);
			   fflush(stdout);

			   //cpu decode & execute
			   analizadorLinea(instruction,&functions,&kernel_functions);

			   myCPU.quantum--;
			   myCPU.instructionPointer++;
		   }
	}




   //generate PCB
//   t_PCB *dummyPCB = cpu_createPCBFromScript(testProgram);
//
//   void *serializedPCB = pcb_serializePCB(dummyPCB);
//
//   pcb_destroy(dummyPCB);
//
//   t_PCBVariableSize *variableInfo = malloc(sizeof(t_PCBVariableSize));
//   recv(myCPU.connections[T_KERNEL].socketFileDescriptor, variableInfo, sizeof(t_PCBVariableSize), 0);
//
//   uint32_t size = pcb_getBufferSizeFromVariableSize(variableInfo);
//
//   void *bafer = malloc(size);
//   recv(myCPU.connections[T_KERNEL].socketFileDescriptor, bafer, size, 0);
////   memcpy(variableInfo,serializedPCB,sizeof(t_PCBVariableSize));
////
////   // PCB received
//   t_PCB *newPCB = pcb_deSerializePCB(bafer, variableInfo);
//
//   pcb_dump(newPCB);
//
//
//   myCPU.assignedPCB = newPCB;
//   myCPU.status = RUNNING;
//   myCPU.quantum = 9999;

   //instruction cycle



	return EXIT_SUCCESS;
}

