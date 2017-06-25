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
//TO DO recibir respuestas del kernel (receive)
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
#include <pthread.h>


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

void programThread_sig_handler(int signo)
{
    write(1, "si2g", 4);

    write(1, "sig", 4);
	pthread_exit(0);



}

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
				"4-Clear Console\n5-Show current threads\n");
		scanf("%d",&menuopt);
		switch(menuopt){
		case 1:	{					 //Start Program
			requestFilePath(program);//Save in program the file path of the file
			startProgram(program);
			break;
		}
		case 2: {						 //End Program
			unPid = requestPid();
			endProgram(unPid);
			break;
		}
		case 3:{						 //Disconnect Program
			disconnectConsole();
			break;
		}
		case 4:{						 //Clear Console
			clearConsole();
			break;
		}
		case 5:{
			showCurrentThreads();
			break;
		}
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


t_process *getThreadfromPid(int aPid){

	t_process * aux = NULL;
	int threadListSize = list_size(processList);
	int i = 0;

		for(i = 0; i < threadListSize; i++){

			aux = list_get(processList, i);

			if(aux->processId == aPid){
				return aux;
			}

		}

	return NULL;
}

int getIndexFromTid(pthread_t tid){

	t_process * aux = NULL;
	int errorCode = -1;
	int threadListSize = list_size(processList);
	int i = 0;


		for(i = 0; i < threadListSize; i++){

			aux = list_get(processList, i);

			if(aux->threadID == tid){
				return i;
			}

		}
	return errorCode;

}
void endProgram(int pid){

	t_process *threadToKill = getThreadfromPid(pid);
	int indexOfRemovedThread= getIndexFromTid(threadToKill->threadID);
	ipc_client_sendFinishProgram(threadToKill->kernelSocket, threadToKill->processId);
	close(threadToKill->kernelSocket);
	int result = pthread_kill(threadToKill, SIGUSR1);
	printf("ABORTING PROCESS - PID:[%d] SOCKET:[%d] TID:[%u]\n", threadToKill->processId, threadToKill->kernelSocket, threadToKill->threadID);


	if (signal(SIGUSR1, programThread_sig_handler) == SIG_ERR)
	 	        printf("\ncan't catch SIGUSR1\n");

	result = pthread_kill(threadToKill->threadID, SIGUSR1);

	 if (result != 0){
		    perror("Error: Thread not finished successfully");
 	}


	printf("\n\nLLEGO CHETITO ACA\n\n");

	list_remove(processList, indexOfRemovedThread);

}


void disconnectConsole(){
	printf("\n\n *** DISCONNECTING CONSOLE ***\n\n");
	sleep(5);
	abort();
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
		t_process *aux = malloc(sizeof(t_process)); // Para la lista de PIDs y Thread Ids
		pthread_t self = pthread_self();
		aux->threadID = self;

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
	   programLength = parser_getAnSISOPFromFile(program, &buffer);

	   log_debug(logger, "Read file. %s. Size: %d", program, programLength);
	   dump_buffer(buffer, programLength);
	   ipc_client_sendStartProgram(sockfd, programLength, buffer);

	   	ipc_struct_program_start_response *startResponse = ipc_client_receiveStartProgramResponse(sockfd);
	   	aux->kernelSocket = sockfd;
	   	aux->processId = startResponse->pid;
	   	log_debug(logger, "program_start_response-> pid: %d", aux->processId);
	   	list_add(processList , aux);
	   	int iterations = 0;
	   	int newThreadIndex = getIndexFromTid(aux->threadID);
	   	while(1){

	   		printf("Hi! I'm thread %u\n",aux->threadID);

	   		sleep(20);
	   		if(iterations == 10){
	   			printf("Finishing %u\n",aux->threadID);

	   			list_remove(processList, newThreadIndex);
	   			break;
	   		}

	   		iterations++;

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

void showCurrentThreads(){
	int listSize = list_size(processList);
	int i;
	t_process * aux;
	if(listSize == 0) printf("No threads currently in execution\n");
	for (i=0 ; i<listSize; i++){
		aux = list_get(processList, i);
		printf("[%d] - TID: %u  PID: %u\n",i, aux->threadID, aux->processId);
	}
}
