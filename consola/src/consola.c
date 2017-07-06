/*
 ============================================================================
 Name        : consola.c
 Author      : Deus Vult
 Version     :
 Copyright   : Copyright
 Description :
 ============================================================================
 */


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
#include <time.h>

t_config *consoleConfig;
t_log *logger;
int portno;
char *serverIp = 0;
t_list * processList;


typedef struct t_process {
	pthread_t threadID;
	uint32_t processId;
	int kernelSocket,consoleImpressions;
	time_t startTime,endTime;
	struct t_process * processMemoryAddress;
} t_process;

// Senial para matar threads
void programThread_sig_handler(int signo) {

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

void showMenu() {

	int menuopt;
	int unPid;
	printf("\nConsole Menu:\n");

	char program[50];

	do {
		printf("\n1-Start Program\n2-End Program\n3-Disconnect Program\n"
				"4-Clear Console\n5-Show current threads\n");
		scanf("%d", &menuopt);
		switch (menuopt) {
		case 1: {					 //Start Program
			requestFilePath(program);//Save in program the file path of the file
			startProgram(program);
			break;
		}
		case 2: {						 //End Program
			unPid = requestPid();
			endProgram(unPid);
			break;
		}
		case 3: {						 //Disconnect Program
			disconnectConsole();
			break;
		}
		case 4: {						 //Clear Console
			clearConsole();
			break;
		}
		case 5: {
			showCurrentThreads();
			break;
		}
		default:
			printf("Invalid input\n");
			break;
		}
	} while (menuopt != 6);
}
void startProgram(char * programPath) {


	printf("\nInitiating:%s\n", programPath);

	//Thread ID
	pthread_t threadId;

	//Create thread attributes
	pthread_attr_t attr;
	pthread_attr_init(&attr);

	//Set to detached
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

	//Create thread
	pthread_create(&threadId, &attr, executeProgram, programPath);



}


int requestPid() {
	int pid;
	printf("\nEnter pid:\n");
	scanf("%d", &pid);
	return pid;

}

//Funcion utilizada para obtener un proceso de la lista
t_process *getThreadfromPid(int aPid) {

	t_process * aux = NULL;
	int threadListSize = list_size(processList);
	int i = 0;

	for (i = 0; i < threadListSize; i++) {

		aux = list_get(processList, i);

		if (aux->processId == aPid) {
			return aux;
		}

	}

	return NULL;
}

//Funcion Utilizada para obtener el indice de un Thread
int getIndexFromTid(pthread_t tid) {

	t_process * aux = NULL;
	int errorCode = -1;
	int threadListSize = list_size(processList);
	int i = 0;

	for (i = 0; i < threadListSize; i++) {

		aux = list_get(processList, i);

		if (aux->threadID == tid) {
			return i;
		}

	}
	return errorCode;

}


void endProgram(int pid) {
	int result;

	if (noThreadsInExecution() == 1) {
		printf("No threads currently in execution - End Program not available\n");
		return; //Si no hay hilos en ejecucion corta
	}

	t_process *threadToKill = getThreadfromPid(pid);


	if (getThreadfromPid(pid) == NULL) {
		printf("No thread with such pid could be found - End Program aborted\n");
		return; //Si el pid ingresado no existe dentro de la lista de procesos
	}


	//Saves the time on which the process was forced to stop executio (considering the instant previous to send the finish program notification).
	threadToKill->endTime = time(NULL);

	int indexOfRemovedThread = getIndexFromTid(threadToKill->threadID);
	ipc_client_sendFinishProgram(threadToKill->kernelSocket,
			threadToKill->processId);

	//Cierra el socket
	close(threadToKill->kernelSocket);



	printf("ABORTING PROCESS - PID:[%d] SOCKET:[%d] TID:[%u]\n",
			threadToKill->processId, threadToKill->kernelSocket,
			threadToKill->threadID);

	showFinishedThreadInfo(threadToKill);

	if (signal(SIGUSR1, programThread_sig_handler) == SIG_ERR)
		printf("\ncan't catch SIGUSR1\n");

	//Mata el Hilo
	result = pthread_kill(threadToKill->threadID, SIGUSR1);

	if (result != 0) {
		perror("Error: Thread not finished successfully");
	}

	list_remove(processList, indexOfRemovedThread);
	free(threadToKill->processMemoryAddress);

}
void disconnectConsole() {
	printf("\n\n *** DISCONNECTING CONSOLE ***\n\n");
	sleep(5);
	abort();
}
void clearConsole() {
	system("clear");
	return;
}
void requestFilePath(char *filePath) {

	printf("\nPlease provide file path\n");
	getchar();
	gets(filePath);
	//puts(filePath);

}

//Recibe el path del programa
void *executeProgram(void *arg) {

	char * program = (char *) arg;

	connectToKernel(program);

	return NULL;

}
int parser_getAnSISOPFromFile(char *name, void **buffer);
void connectToKernel(char * program) {

	//Declare variables used in function
	int sockfd, n;
	struct sockaddr_in serv_addr;
	struct hostent *server;
	int programLength;

	//Create a pointer to t_process in order to add it to the list of threads in execution
	t_process *aux = malloc(sizeof(t_process));

	//Saves a reference to the thread
	pthread_t self = pthread_self();

	//Populate the information of the aux pointer
	aux->threadID = self;
	aux->processMemoryAddress = aux; //Saves a reference to the memory direction of aux so it can be freed from endProgram

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
	bcopy((char *) server->h_addr, (char *)&serv_addr.sin_addr.s_addr, server->h_length);
	serv_addr.sin_port = htons(portno);

	/* Now connect to the server */
	if (connect(sockfd, (struct sockaddr*) &serv_addr, sizeof(serv_addr)) < 0) {
		perror("ERROR connecting");
		exit(1);
	}

	//Saves the time on which process starts executing (considering the instant after it is connected to the Kernel)
	aux->startTime = time(NULL);

	// Send handshake and wait for response
	ipc_client_sendHandshake(CONSOLE, sockfd);
	ipc_struct_handshake_response *response = ipc_client_waitHandshakeResponse(
			sockfd);
	log_debug(logger, "Se recibió respuesta de handshake. Success: %d",
			response->success);

	free(response);
	void *buffer = 0;
	//Guarda el largo del programa y lo mete en buffer
	programLength = parser_getAnSISOPFromFile(program, &buffer);

	log_debug(logger, "Read file. %s. Size: %d", program, programLength);
	dump_buffer(buffer, programLength);
	//Envia el codigo del programa al Kernel
	ipc_client_sendStartProgram(sockfd, programLength, buffer);
	//Se le asigna pid
	ipc_struct_program_start_response *startResponse =
			ipc_client_receiveStartProgramResponse(sockfd);
	aux->kernelSocket = sockfd;
	aux->processId = startResponse->pid;
	log_debug(logger, "program_start_response-> pid: %d", aux->processId);

	free(startResponse);

	list_add(processList, aux);
	int iterations = 0;
	int newThreadIndex = getIndexFromTid(aux->threadID);

	//TODO: Receive actual information from kernel and print it in console
	while (1) {
		/*
		ipc_header header;

		recv(sockfd, &header, sizeof(ipc_header), 0);

		if (header.operationIdentifier == PRINT_CONSOLE_MESSAGE) {

			uint32_t length;


			recv(sockfd, &length, sizeof(uint32_t), 0);

			char *messageBuffer = malloc(length);

			recv(sockfd, messageBuffer, length, 0);
			printf("\nProcess %u printing message \n", aux->processId);
			log_debug(logger, "%s", messageBuffer);

			free(messageBuffer);
			aux->consoleImpressions ++;

		}

		//If the Kernel console requests to finish a program
		if (header.operationIdentifier == PROGRAM_FINISH) {

			//endProgram automatically saves the endTime of the process so theres no need to save it here.
			endProgram(aux->processId);


		}*/

		printf("Hi! I'm thread %u\n", aux->threadID);

		   sleep(5);
		   if (iterations == 3) {
		    printf("Finishing %u\n", aux->threadID);
		    //endProgram(aux->processId);

		    break;
		    }

		   iterations++;

	}

	//Saves process end time in case it finishes it execution normally
	aux->endTime = time(NULL);

	//Shows time info
	showFinishedThreadInfo(aux);

	//Frees reserved memory
	free(aux);
	return;
}

//File Manager
int parser_getAnSISOPFromFile(char *name, void **buffer) {
	FILE *file;
	unsigned long fileLen;

	//Open file
	file = fopen(name, "rb");
	if (!file) {
		log_error(logger, "No se puede abrir el archivo %s", name);
	}

	//Get file length
	fseek(file, 0, SEEK_END);
	fileLen = ftell(file);
	fseek(file, 0, SEEK_SET);

	//Allocate memory
	*buffer = (char *) malloc(fileLen + 1);
	if (!*buffer) {
		log_error(logger, "Error de memoria");
		fclose(file);
	}

	//Read file contents into buffer
	fread(*buffer, fileLen, 1, file);
	fclose(file);
	free(*buffer);
	//Do whatever with buffer
	return fileLen;
}

// Debug
void dump_buffer(void *buffer, int size) {
	char ch;
	int i;
	for (i = 0; i < size; i++) {
		ch = *((char *) buffer + i);
		if (isprint(ch)) {
			printf("%c", ch);
		} else {
			switch (ch) {
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

void showCurrentThreads() {
	int listSize = list_size(processList);
	int i;
	t_process * aux;
	if (listSize == 0)
		printf("No threads currently in execution\n");
	for (i = 0; i < listSize; i++) {
		aux = list_get(processList, i);
		printf("[%d] - TID: %u  PID: %u\n", i, aux->threadID, aux->processId);
	}
}

//Devuelve 0 si no hay elementos
int noThreadsInExecution() {

	int listSize = list_size(processList);

	if (listSize == 0)
		return 1;

	return 0;
}

void showFinishedThreadInfo(t_process * aProcess){

	printf("\n\n***************************************************************************************\n");
	printf("Program with TID: [%u]  PID: [%u] has finished its execution\n", aProcess->threadID, aProcess->processId);
	printf("Program started execution at: %s",ctime(&aProcess->startTime));
	printf("Program finished execution at: %s",ctime(&aProcess->endTime));
	printf("The thread was running for %f seconds\n", difftime(aProcess->endTime, aProcess->startTime));
	printf("This thread printed [%d] messages on screen\n", aProcess->consoleImpressions);
	printf("***************************************************************************************\n");

	return;

}
