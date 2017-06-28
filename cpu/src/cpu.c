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
uint32_t cpu_start(t_CPU *CPU){
	CPU->assignedPCB = NULL;
	CPU->connections[T_KERNEL].host = "127.0.0.1";
	CPU->connections[T_KERNEL].portNumber = 5000;
	CPU->connections[T_KERNEL].server = 0;
	CPU->connections[T_KERNEL].socketFileDescriptor = 0;
	CPU->connections[T_KERNEL].status = DISCONNECTED;
	CPU->connections[T_MEMORY].host = "127.0.0.1";
	CPU->connections[T_MEMORY].portNumber = 5001;
	CPU->connections[T_MEMORY].server = 0;
	CPU->connections[T_MEMORY].socketFileDescriptor = 0;
	CPU->connections[T_MEMORY].status = DISCONNECTED;
	CPU->currentInstruction = 0;
	CPU->instructionPointer = 0;
	CPU->variableCounter = 0;
	CPU->status = WAITING;

	return 0;
}
uint32_t cpu_connect(t_CPU *CPU, t_connectionType connectionType){
	t_CPUConnection *kernelConnection;
	switch (connectionType) {
		case T_D_MEMORY:
			printf("\nDummy");
			printf("\nConnecting to DummyMemory\n");
			myMemory = malloc(512*4);
			memset(myMemory,0,512*4);
			CPU->connections[1].portNumber = 9999;
			CPU->connections[1].status = CONNECTED;
			printf("\nConnected to memory\n");
			return 0;
			break;
		case T_KERNEL:
			kernelConnection = &CPU->connections[0];
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
			   kernelConnection->status = CONNECTED;
			   printf("CONNECTED TO KERNEL");

			break;
		case T_D_KERNEL:
			printf("\nConnecting to Dummy Kernel\n");
			CPU->connections[0].status = CONNECTED;
			printf("\nConnected to Dummy Kernel\n");
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

 char *program =
"#!/usr/bin/ansisop\n"
"\n"
"begin\n"
"variables a\n"
"a <- f\n"
"prints n a\n"
"end\n"
"\n"
"function f\n"
"variables a\n"
"a=1\n"
"prints n a\n"
"a <- g\n"
"return a\n"
"end\n"
"\n"
"function g\n"
"variables a\n"
"a=0\n"
"prints n a\n"
"return a\n"
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
	// start CPU
	cpu_start(&myCPU);

	// connect to memory -- DUMMY
	cpu_connect(&myCPU,T_D_MEMORY);

	char *testProgram = strdup(program);
	//printf("writing program: %s", testProgram);
	memcpy(myMemory,testProgram,strlen(testProgram));

	cpu_connect(&myCPU,T_D_KERNEL);

	char *logfile = tmpnam(NULL);

	logger = log_create(logfile,"CPU",1,LOG_LEVEL_DEBUG);

/*   // Send handshake and wait for response
   ipc_client_sendHandshake(CPU, myCPU.connections[0].socketFileDescriptor);
   ipc_struct_handshake_response *response = ipc_client_waitHandshakeResponse(myCPU.connections[0].socketFileDescriptor);
   free(response);

 */  //wait for PCB
   myCPU.status = WAITING;

   //generate PCB
   t_PCB *dummyPCB = cpu_createPCBFromScript(testProgram);

   void *serializedPCB = pcb_serializePCB(dummyPCB);

   pcb_destroy(dummyPCB);

   t_PCBVariableSize *variableInfo = malloc(sizeof(t_PCBVariableSize));
   memcpy(variableInfo,serializedPCB,sizeof(t_PCBVariableSize));

   // PCB received
   t_PCB *newPCB = pcb_deSerializePCB(serializedPCB,variableInfo);

   //pcb_dump(newPCB);


   myCPU.assignedPCB = newPCB;
   myCPU.status = RUNNING;
   myCPU.quantum = 9999;

   //instruction cycle
   while(myCPU.status == RUNNING && myCPU.quantum > 0){
	   //cpu fetch
	   t_codeIndex currentInstructionIndex = myCPU.assignedPCB->codeIndex[myCPU.instructionPointer];
	   int page = currentInstructionIndex.start / DUMMY_MEMORY_PAGE_SIZE;
	   int offset = currentInstructionIndex.start - (page * DUMMY_MEMORY_PAGE_SIZE);

	   void *instruction = cpu_readMemoryDummy(myCPU.assignedPCB->pid,page,offset,currentInstructionIndex.size);
	   fflush(stdout);
	   printf("fetched instruction is: %s\n",(char *) instruction);
	   fflush(stdout);

	   //cpu decode & execute
	   analizadorLinea(instruction,&functions,&kernel_functions);

	   myCPU.quantum--;
	   myCPU.instructionPointer++;
   }


	return EXIT_SUCCESS;
}

