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
#include <stdio.h>
#include <stdlib.h>
#include <netdb.h>
#include <netinet/in.h>
#include <string.h>
#include <unistd.h>
#include "cpu.h"

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


	   char buffer[256];



	   /* Create a socket point */

	   CPU->connections[connectionType].socketFileDescriptor = socket(AF_INET, SOCK_STREAM, 0);

	   if (CPU->connections[connectionType].socketFileDescriptor < 0) {
		  perror("ERROR opening socket");
		  exit(1);
	   }

	   CPU->connections[connectionType].server = gethostbyname(CPU->connections[connectionType].host);
	   bzero((char *) (&CPU->connections[connectionType].serv_addr), sizeof(CPU->connections[connectionType].serv_addr));
	   CPU->connections[connectionType].serv_addr.sin_family = AF_INET;
	   bcopy((char *) (CPU->connections[connectionType].server), (char *)(&CPU->connections[connectionType].serv_addr.sin_addr.s_addr), CPU->connections[connectionType].server->h_length);
	   CPU->connections[connectionType].serv_addr.sin_port = htons(CPU->connections[connectionType].portNumber);

	   /* Now connect to the server */
	   if (connect(CPU->connections[connectionType].socketFileDescriptor, (struct sockaddr*)(&CPU->connections[connectionType].serv_addr), sizeof(CPU->connections[connectionType].serv_addr)) < 0) {
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
//test

