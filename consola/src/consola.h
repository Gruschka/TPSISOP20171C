/*
 * consola.h
 *
 *  Created on: 9/4/2017
 *      Author: utnso
 */

#ifndef CONSOLA_H_
#define CONSOLA_H_


void showMenu();
//void joinThreadList(t_list * threadList);
void startProgram(char *program);
void endProgram(int  pid);
void disconnectProgram();
void clearConsole();
void *executeProgram(void *arg);
void disconnectConsole();
void requestFilePath(char *filePath);
void connectToKernel(char *);
int requestPid();
void dump_buffer(void *buffer, int size);

//void console_print_programs(t_list * head);
#endif /* CONSOLA_H_ */
