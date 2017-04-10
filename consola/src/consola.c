/*
 ============================================================================
 Name        : consola.c
 Author      : Hernan Canzonetta
 Version     :
 Copyright   : Copyright
 Description : Hello World in C, Ansi-style
 ============================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <commons/config.h>
#include <pthread.h>
#include <string.h>





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
		disconnectProgram();
		break;

	case 4:
		clearConsole();
		break;

	case 5:

		break;

	default:
		printf("Invalid input\n");
		break;


	}

}
void startProgram(char * programPath){

	printf("\nInitiating:%s\n", programPath);

	//Thread ID
	pthread_t threadId;

	//Create thread attributes
	pthread_attr_t attr;
	pthread_attr_init(&attr);

	pthread_create(&threadId, &attr,  , NULL);

	pthread_join(threadId, NULL);  //NULL means that it isn't going to catch the return value from pthread_exit




}
void endProgram(){
	printf("\nEnd Program\n");

}
void disconnectProgram(){
	printf("\nDisconnect Program\n");

}
void clearConsole(){
	printf("\Clear console\n");

}
void requestFilePath(char * filePath){

	printf("\nPlease provide file path\n");
	getchar();
	gets(filePath);
	//puts(filePath);

}
