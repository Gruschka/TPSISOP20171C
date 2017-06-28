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
	PCB->sp = 0;
	PCB->stackIndex = NULL;

	return PCB;
}

char* PROGRAMA =
		"begin\n"
		"variables a, b\n"
		"a = 3\n"
		"b = 5\n"
		"a = b + 12\n"
		"end\n"
		"\n";

 char *otroPrograma =
"#!/usr/bin/ansisop\n"
"begin\n"
"variables f, i, t\n"
"\n"
"#`i`: Iterador\n"
"i=0\n"
"\n"
"#`f`: Hasta donde contar\n"
"f=20\n"
":inicio\n"
"\n"
"#Incrementar el iterador\n"
"i=i+1\n"
"\n"
"#Imprimir el contador\n"
"prints n i\n"
"\n"
"#`t`: Comparador entre `i` y `f`\n"
"t=f-i\n"
"#De no ser iguales, salta a inicio\n"
"jnz t inicio\n"
"\n"
"end\n";

 char *otroOtroPrograma =
"#!/usr/bin/ansisop\n"
"\n"
"begin\n"
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
"	a <- f\n"
"	return a\n"
"end\n";



 AnSISOP_funciones functions = {
 		.AnSISOP_definirVariable		= dummy_definirVariable,
 		.AnSISOP_obtenerPosicionVariable= dummy_obtenerPosicionVariable,
 		.AnSISOP_finalizar 				= dummy_finalizar,
 		.AnSISOP_dereferenciar			= dummy_dereferenciar,
 		.AnSISOP_asignar				= dummy_asignar,

 };

 AnSISOP_kernel kernel_functions = { };

int main() {

	// start CPU
	cpu_start(&myCPU);

	// connect to memory -- DUMMY
	cpu_connect(&myCPU,T_D_MEMORY);
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
   t_PCB *dummyPCB = cpu_createPCBFromScript(otroOtroPrograma);

   void *serializedPCB = pcb_serializePCB(dummyPCB);

   pcb_destroy(dummyPCB);

   t_PCBVariableSize *variableInfo = malloc(sizeof(t_PCBVariableSize));
   memcpy(variableInfo,serializedPCB,sizeof(t_PCBVariableSize));

   t_PCB *newPCB = pcb_deSerializePCB(serializedPCB,variableInfo);

   pcb_dump(newPCB);

	return EXIT_SUCCESS;
}

