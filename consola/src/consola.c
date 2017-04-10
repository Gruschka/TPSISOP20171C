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




int main(void) {

	showMenu();

}


void showMenu(){

	int menuopt;
	printf("\nConsole Menu:\n");
	printf("1-Start Program\n2-End Program\n3-Disconnect Program\n"
			"4-Clear Console\n");
	scanf("%d",&menuopt);
	char program[50];

	do{
		switch(menuopt){
		case 1:
			requestFilePath(program);//Save in program the file path of the file
			startProgram(program);
			showMenu(); //Una vez que se inicia un programa, vuelve a mostrar menu
			break;

		case 2:
			endProgram();
			break;

		case 3:
			disconnectConsole();
			break;

		case 4:
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
	printf("\Clear console\n");

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




}


int connectToKernel(char * program){
		FILE* configFile;
		int sockfd, portno, n;
		struct sockaddr_in serv_addr;
		struct hostent *server;
		char serverIp[30];
		char buffer[256];


		printf("EL programa es: %s", program);
		configFile = fopen("/home/utnso/git/tp-2017-1c-Deus-Vult/consola/src/consoleConfig.txt", "r");
		fscanf(configFile, "%s", serverIp);
		fscanf(configFile, "%d", &portno);
		fclose(configFile);
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

	   /* Now ask for a message from the user, this message
	      * will be read by server
	   */

	   printf("Please enter the message: ");
	   bzero(buffer,256);
	   fgets(buffer,255,stdin);

	   /* Send message to the server */
	   n = write(sockfd, buffer, strlen(buffer));

	   if (n < 0) {
	      perror("ERROR writinsadaklsdjaskldjakldjaslkdjasklg to socket");
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
	   return 0;





}


