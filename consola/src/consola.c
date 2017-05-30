/*
 ============================================================================
 Name        : consola.c
 Author      : Hernan Canzonetta
 Version     :
 Copyright   : Copyright
 Description : Hello World in C, Ansi-style
 ============================================================================
 */

//Ordenados por prioridad descendente.
//TO DO: Recibir PID del Kernel y no usar el hardcodeado
//TO DO: Implementar funcionamiento de los programas con archivos (actualmente no hacen nada) y recibir respuestas del kernel (receive)
//TO DO: Devolver termination status al kernel (Segun si se finishea el proceso o se mata la consola)
//TO DO: Desconectar consola
//TO DO: Ver si podemos repetir menos logica con las funciones que devuelven TID e Indice de una lista.
#include "consola.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <stdio.h>
#include <stdlib.h>
#include <netdb.h>
#include <netinet/in.h>
#include <string.h>
#include <unistd.h>
#include <commons/config.h>
#include <commons/log.h>
#include <commons/collections/list.h>
#include <ctype.h>
#include <ipc/serialization.h>
#include <ipc/ipc.h>
#include <signal.h>
#define CONSOLE_DISC 15


t_config *consoleConfig;
t_log *logger;
int portno;
char *serverIp = 0;
t_list * processList;

int globalPid = 0; //Pid global que se incrementa con cada hilo que se crea para poder testear finalizar hilos por pid

typedef struct t_process {
	pthread_t threadID;
	uint32_t processId;
	int kernelSocket;
} t_process;


int main(int argc, char **argv) {


	char *logFile = tmpnam(NULL);
	logger = log_create(logFile, "CONSOLE", 1, LOG_LEVEL_DEBUG);

	if (argc < 2) {
			log_error(logger, "Falta pasar la ruta del archivo de configuracion");
			return EXIT_FAILURE;
		} else {
			consoleConfig = config_create(argv[1]);
			serverIp = config_get_string_value(consoleConfig, "IP_KERNEL");
			portno = config_get_int_value(consoleConfig, "PUERTO_KERNEL");
	}

	processList = list_create();

	showMenu();

	printf("Stopping console execution\n");

	//pthread_join(threadId, NULL);  //NULL means that it isn't going to catch the return value from pthread_exit
	joinThreadList(processList);
	printf("Closing console\n");
	return 0;

}


void showMenu(){

	int menuopt;
	int unPid;
	printf("\nConsole Menu:\n");

	char program[50];

	do{
		printf("\n1-Start Program\n2-End Program\n3-Disconnect Program\n"
				"4-Clear Console\n5-Show current threads\n6-Exit console\n");
		scanf("%d",&menuopt);
		switch(menuopt){
		case 1:						 //Start Program
			requestFilePath(program);//Save in program the file path of the file
			startProgram(program);
			break;

		case 2:						 //End Program
			unPid = requestPid();
			endProgram(unPid);
			break;

		case 3:						 //Disconnect Program
			disconnectConsole();
			break;

		case 4:						 //Clear Console
			clearConsole();
			break;

		case 5:
			showAllPrograms(processList);
			break;

		case 6:
			return;
			break;


		default:
			printf("Invalid input\n");
			break;
		}
	}while (menuopt != 6);
}
void startProgram(char * programPath) {

	printf("\nInitiating:%s\n", programPath);

	//Thread ID
	pthread_t threadId;


	//Create thread attributes

	pthread_attr_t attr;
	pthread_attr_init(&attr);


	pthread_create(&threadId, &attr, executeProgram, programPath);
	printf("El tid dentro de SP es: %u", threadId);





}

int requestPid(){
	int pid;
	printf("\nEnter pid:\n");
	scanf("%d", &pid);
	return pid;

}


pthread_t getTidfromPid(int aPid){
	//TO DO: Ver si se puede hacer con un list_find para hacerlo menos grasuli


	t_process * aux = NULL;
	t_list * threadListHead = processList;
	int errorCode = 2;
	int threadListSize = list_size(threadListHead);
	int i = 0;




		for(i = 0; i < threadListSize; i++){

			aux = list_get(threadListHead, i);

			if(aux->processId == aPid){
				return aux->threadID;
			}

		}
	return errorCode;
}

int getIndexFromTid(pthread_t tid){

	t_process * aux = NULL;
	t_list * threadListHead = processList;
	int errorCode = -1;
	int threadListSize = list_size(threadListHead);
	int i = 0;


		for(i = 0; i < threadListSize; i++){

			aux = list_get(threadListHead, i);

			if(aux->threadID == tid){
				return i;
			}

		}
	return errorCode;

}
void endProgram(int pid){
	printf("\nFinishing program with PID: %d\n",pid);
	pthread_t tidToKill = getTidfromPid(pid);
	int indexOfRemovedThread= getIndexFromTid(tidToKill);

	int result = pthread_kill(tidToKill, SIGKILL);

	if(result == 0){
	    	printf("Program finished successfully\n");
	    }else{
	    	printf("Error - program not finished successfully\n");
	    }


	/*int result = pthread_kill(tidToKill, 2);

    if(result == 0){
    	printf("Program finished successfully");
    }*/

	list_remove(processList, indexOfRemovedThread);

	//TO DO: Mandar mensaje al kernel con termination status?

}


void disconnectConsole(){

	int listSize = list_size(processList);
	int i;
	t_process * aux = NULL;
	int errorCode;
	int consoleDisc = CONSOLE_DISC;
	printf("\nDisconnecting Console\n");
	for(i=0; i < listSize; i++){

		aux = list_get(processList, i);
		printf("Aborting Thread: %u PID: %u", aux->threadID, aux->processId);
		send(aux->kernelSocket, &consoleDisc, sizeof(int), 0);
		errorCode = pthread_kill(aux->threadID, SIGKILL);
		if (errorCode == 0){
			printf("Thread Successfully Aborted");
		}else{
			printf("Thread Not Aborted Correctly");
		}
		list_remove(processList, i);

	}



}


void clearConsole(){
	system("clear");
	return;
}


void requestFilePath(char *filePath){

	printf("\nPlease provide file path\n");
	getchar();
	gets(filePath);
	//puts(filePath);

}


void *executeProgram(void *arg){


	printf("Execute ");

	char * program = (char *)arg;

	connectToKernel(program);

	return NULL;


}

int parser_getAnSISOPFromFile(char *name, void **buffer);

void connectToKernel(char * program){

		int pid; //TO DO: Este valor debe tomarlo por parte del KERNEL
		int sockfd, n;
		struct sockaddr_in serv_addr;
		struct hostent *server;
		int programLength;
		t_process aux; // Para la lista de PIDs y Thread Ids
		pthread_t self = pthread_self();
		aux.threadID = self;

		printf("EL programa es: %s", program);
		printf("\nServer Ip: %s Port No: %d", serverIp, portno);
		printf("\nConnecting to Kernel\n");

		/* Create a socket point */
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

	   /* Now connect to the server */
	   if (connect(sockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
	      perror("ERROR connecting");
	      exit(1);
	   }

	   // Send handshake and wait for response
	   ipc_client_sendHandshake(CONSOLE, sockfd);
	   ipc_struct_handshake_response *response = ipc_client_waitHandshakeResponse(sockfd);

	   log_debug(logger, "Se recibió respuesta de handshake. Success: %d", response->success);
	   // Now sends the program and is read by server

	   void *buffer = 0;
	   //Comennto parte de archivo para poder trabajar sin archivos y probar la lista de t_process
	   //programLength = parser_getAnSISOPFromFile(program, &buffer);

	   //log_debug(logger, "Read file. %s. Size: %d", program, programLength);
	   //dump_buffer(buffer, programLength);
	  // ipc_client_sendStartProgram(sockfd, programLength, buffer);

	   //Aca deberiamos recibir el PID del hilo por parte del Kernel

	   aux.kernelSocket = sockfd;
	   recv(sockfd,&aux.processId, sizeof(uint32_t),MSG_WAITALL);
	   list_add(processList , &aux);


	   printf("Program Started.\nThread Id: %u\nPID:%u\n",aux.threadID,aux.processId);

	   int iterations = 0;

	   while(1){

		   printf("Hi! I'm thread: %u\n", self);
		   sleep(20);
		   iterations++; //Grasada para que imprima 3 veces y termine

		   if(iterations == 30){
			   printf("Finishing thread %u\n", self);
			   break;
		   }



	   }

	   return;
}



//File Manager
int parser_getAnSISOPFromFile(char *name, void **buffer){
	FILE *file;
	unsigned long fileLen;

	//Open file
	file = fopen(name, "rb");
	if (!file)
	{
		log_error(logger, "No se puede abrir el archivo %s", name);
	}

	//Get file length
	fseek(file, 0, SEEK_END);
	fileLen=ftell(file);
	fseek(file, 0, SEEK_SET);

	//Allocate memory
	*buffer=(char *)malloc(fileLen+1);
	if (!*buffer)
	{
		log_error(logger, "Error de memoria");
		fclose(file);
	}

	//Read file contents into buffer
	fread(*buffer, fileLen, 1, file);
	fclose(file);

	//Do whatever with buffer
	return fileLen;
}

// Debug
void dump_buffer(void *buffer, int size)
{
	char ch;
	int i;
	for (i = 0; i < size; i++) {
		ch = *((char *)buffer + i);
		if (isprint(ch)) {
			printf("%c", ch);
		} else {
			switch(ch)
			{
			case '\n':
				printf("\\n");
				break;

			default:
				printf("\\x%02x", ch);
				break;
			}
		}
	}
}


void joinThreadList(t_list * threadListHead){


	int threadListSize = list_size(threadListHead);
	t_process * aux = NULL;
	int i = 0;

	if(threadListSize == 0){
		printf("There are no threads to join\n");
		return;
	}


	for(i = 0; i < threadListSize; i++){



		aux = list_get(threadListHead, i);

		printf("Waiting for Thread[%d]: %u from process %u\n",i, aux->threadID, aux->processId);

		pthread_join(aux->threadID, NULL);  //NULL means that it isn't going to catch the return value from pthread_exit

		printf("Thread:[%d] - %u successfully joined\n",i, aux->threadID);


	}


	printf("All the threads have joined!\n");


}


void showAllPrograms(t_list * threadListHead){

	int threadListSize = list_size(threadListHead);
	t_process * aux = NULL;
	int i = 0;

	printf("Listing active program threads:\n");

	if(threadListSize == 0){
		printf("There are no active threads\n");
		return;
	}


	for(i = 0; i < threadListSize; i++){

		aux = list_get(threadListHead, i);

		printf("[%d] - TID: %u  PID: %u\n",i, aux->threadID, aux->processId);


	}






}
