/*
 ============================================================================
 Name        : consola.c
 Author      : Hernan Canzonetta
 Version     :
 Copyright   : Copyright
 Description : Hello World in C, Ansi-style
 ============================================================================
 */

#include "consola.h"
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
#include <commons/config.h>
#include <commons/log.h>


t_config *consoleConfig;
t_log *logger;
int portno;
char *serverIp = 0;


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
	showMenu();



}


void showMenu(){

	int menuopt;
	printf("\nConsole Menu:\n");

	char program[50];

	do{
		printf("1-Start Program\n2-End Program\n3-Disconnect Program\n"
				"4-Clear Console\n");
		scanf("%d",&menuopt);
		switch(menuopt){
		case 1:						 //Start Program
			requestFilePath(program);//Save in program the file path of the file
			startProgram(program);
			break;

		case 2:						 //End Program
			endProgram();
			break;

		case 3:						 //Disconnect Program
			disconnectConsole();
			break;

		case 4:						 //Clear Console
			clearConsole();
			break;

		default:
			printf("Invalid input\n");
			break;


		}

	}while (menuopt != 3);
}
void startProgram(char * programPath) {

	printf("\nInitiating:%s\n", programPath);

	//Thread ID
	pthread_t threadId;

	//Create thread attributes
	pthread_attr_t attr;
	pthread_attr_init(&attr);

	pthread_create(&threadId, &attr, executeProgram, programPath);

	pthread_join(threadId, NULL);  //NULL means that it isn't going to catch the return value from pthread_exit




}



void endProgram(){
	printf("\nEnd Program\n");

}



void disconnectConsole(){
	printf("\nDisconnecting Console\n");

}


void clearConsole(){
	printf("\Cleanning console\n");
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

int parser_getAnSISOPFromFile(char *name, char **buffer);

void connectToKernel(char * program){

		int sockfd, n;
		struct sockaddr_in serv_addr;
		struct hostent *server;
		char *buffer = 0;
		int programLength;

		printf("EL programa es: %s", program);
		printf("\nServer Ip: %s Port No: %d", serverIp, portno);
		printf("\nConnecting to Kernel\n");

		//Connect to Kernel and Handshake


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

	   // Now sends the program and is read by server


	   programLength = parser_getAnSISOPFromFile(program, &buffer);

	   /* Send message to the server */
	   n = write(sockfd, buffer, programLength);

	   if (n < 0) {
	      perror("ERROR sending message to socket");
	      exit(1);
	   }

	   /* Now read server response */
	   bzero(buffer,256);
	   n = read(sockfd, buffer, 255);

	   if (n < 0) {
	      perror("ERROR reading from socket");
	      exit(1);
	   }

	   printf("%s\n",buffer);
	   return;





}


//File Manager
int parser_getAnSISOPFromFile(char *name, char **buffer){
	FILE *file;
	unsigned long fileLen;

	//Open file
	file = fopen(name, "rb");
	if (!file)
	{
	fprintf(stderr, "Unable to open file %s", name);
	}

	//Get file length
	fseek(file, 0, SEEK_END);
	fileLen=ftell(file);
	fseek(file, 0, SEEK_SET);

	//Allocate memory
	*buffer=(char *)malloc(fileLen+1);
	if (!*buffer)
	{
		fprintf(stderr, "Memory error!");
		fclose(file);
	}

	//Read file contents into buffer
	fread(*buffer, fileLen, 1, file);
	fclose(file);

	//Do what ever with buffer
	return fileLen;
}


