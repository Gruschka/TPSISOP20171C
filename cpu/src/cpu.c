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

uint32_t cpu_connect(t_CPU *CPU, t_connectionType connectionType){
	   int sockfd, portno, n;
	   struct sockaddr_in serv_addr;
	   struct hostent *server;

	   char buffer[256];


	   portno = 5000;

	   /* Create a socket point */
	   CPU->connections[connectionType]->socketFileDescriptor = socket(AF_INET, SOCK_STREAM, 0);

	   if (CPU->connections[connectionType]->socketFileDescriptor < 0) {
		  perror("ERROR opening socket");
		  exit(1);
	   }

	   CPU->connections[connectionType]->server = gethostbyname("10.0.1.90");
	   bzero((char *) &CPU->connections[connectionType]->serv_addr, sizeof(CPU->connections[connectionType]->serv_addr));
	   CPU->connections[connectionType]->serv_addr.sin_family = AF_INET;
	   bcopy((char *)CPU->connections[connectionType]->server, (char *)&CPU->connections[connectionType]->serv_addr.sin_addr.s_addr, CPU->connections[connectionType]->server->h_length);
	   CPU->connections[connectionType]->serv_addr.sin_port = htons(CPU->connections[connectionType]->portNumber);

	   /* Now connect to the server */
	   if (connect(sockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
		  perror("ERROR connecting");
		  exit(1);
	   }

	   /* Now ask for a message from the user, this message
		  * will be read by server
	   */



	   /* Send message to the server */
	   printf("Please enter the message: ");
	   bzero(buffer,256);
	   fgets(buffer,255,stdin);

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
//test
int main(int argc, char *argv[]) {

}
