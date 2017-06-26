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
	CPU->assignedPCB = 0;
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
	CPU->assignedPCB = 0;
	CPU->currentInstruction = 0;
	CPU->instructionPointer = 0;
	CPU->variableCounter = 0;
	CPU->status = RUNNING;

	return 0;
}
uint32_t cpu_connect(t_CPU *CPU, t_connectionType connectionType){
	   int n;

	   if(connectionType == T_MEMORY){
		   myMemory = malloc(512*4);
		   return 0;
	   }

	   char buffer[256];



	   /* Create a socket point */

	   CPU->connections[connectionType].socketFileDescriptor = socket(AF_INET, SOCK_STREAM, 0);

	   if (CPU->connections[connectionType].socketFileDescriptor < 0) {
		  perror("ERROR opening socket");
		  exit(1);
	   }

	   CPU->connections[connectionType].server = gethostbyname(CPU->connections[connectionType].host);
	   bzero((char *) &CPU->connections[connectionType].serv_addr, sizeof(CPU->connections[connectionType].serv_addr));
	   CPU->connections[connectionType].serv_addr.sin_family = AF_INET;
	   bcopy((char *)CPU->connections[connectionType].server, (char *)&CPU->connections[connectionType].serv_addr.sin_addr.s_addr, CPU->connections[connectionType].server->h_length);
	   CPU->connections[connectionType].serv_addr.sin_port = htons(CPU->connections[connectionType].portNumber);

	   /* Now connect to the server */
	   if (connect(CPU->connections[connectionType].socketFileDescriptor, (struct sockaddr*)&CPU->connections[connectionType].serv_addr, sizeof(CPU->connections[connectionType].serv_addr)) < 0) {
		  perror("ERROR connecting");
		  exit(1);
	   }

	   CPU->connections[connectionType].status = CONNECTED;

	   /* Send message to the server */
	   printf("Please enter the message: ");
	   bzero(buffer,256);
	   fgets(buffer,255,stdin);

	   n = write(CPU->connections[connectionType].socketFileDescriptor, buffer, strlen(buffer));

	   if (n < 0) {
		  perror("ERROR writinsadaklsdjaskldjakldjaslkdjasklg to socket");
		  exit(1);
	   }

	   /* Now read server response */
	   bzero(buffer,256);
	   n = read(CPU->connections[connectionType].socketFileDescriptor, buffer, 255);

	   if (n < 0) {
		  perror("ERROR reading from socket");
		  exit(1);
	   }

	   printf("%s\n",buffer);

	   return 0;
}

 char* PROGRAMA =
		"begin\n"
		"variables a, b\n"
		"a = 3\n"
		"b = 5\n"
		"a = b + 12\n"
		"end\n"
		"\n";


int main() {

	cpu_start(&myCPU);
	char *script = strdup(PROGRAMA);
	int programLength = string_length(script);
	t_metadata_program *program = metadata_desde_literal(PROGRAMA);

	cpu_connect(&myCPU,T_MEMORY);

	cpu_writeMemoryDummy(1,0,0,programLength,script);

	void *lectura = cpu_readMemoryDummy(1,0,0,programLength);

	char *logfile = tmpnam(NULL);

	logger = log_create(logfile,"CPU",1,LOG_LEVEL_DEBUG);

	char *serverIp = "10.0.1.90";
	int portno = 5001;
	int sockfd, n;
	struct sockaddr_in serv_addr;
	struct hostent *server;

	printf("\nServer Ip: %s Port No: %d", serverIp, portno);
	printf("\nConnecting to Kernel\n");

	 //Create a socket point
	sockfd = socket(AF_INET, SOCK_STREAM, 0);

   if (sockfd < 0) {
	  perror("ERROR opening socket");
	  exit(1);
   }

   server = gethostbyname(serverIp);
   bzero((char *) &serv_addr, sizeof(serv_addr));
   serv_addr.sin_family = AF_INET;
   bcopy((char *)server->h_addr, (char *)&serv_addr.sin_addr.s_addr, server->h_length);
   serv_addr.sin_port = htons(portno);
/*

   // Now connect to the server
   if (connect(sockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
	  perror("ERROR connecting");
	  exit(1);
   }

   // Send handshake and wait for response
   ipc_client_sendHandshake(CPU, sockfd);
   ipc_struct_handshake_response *response = ipc_client_waitHandshakeResponse(sockfd);

*/
   t_PCB *dummyPCB = pcb_createDummy(1,2,3,4);

   void *serializedPCB;
   serializedPCB = pcb_serializePCB(dummyPCB);

   t_PCBVariableSize *variableInfo = malloc(sizeof(t_PCBVariableSize));
   memcpy(variableInfo,serializedPCB,sizeof(t_PCBVariableSize));

   dummyPCB = pcb_deSerializePCB(serializedPCB,variableInfo->stackIndexRecordCount,variableInfo->stackVariableCount,variableInfo->stackArgumentCount);

   pcb_dump(dummyPCB);


	return EXIT_SUCCESS;
}

