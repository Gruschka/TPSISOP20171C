/*
 * consola.h
 *
 *  Created on: 9/4/2017
 *      Author: utnso
 */

#ifndef CONSOLA_H_
#define CONSOLA_H_


void showMenu();
void startProgram(char *program);
void endProgram();
void disconnectProgram();
void clearConsole();
void *executeProgram(void *arg);
void connectToKernel(char *);

void dump_buffer(void *buffer, int size);

//void console_print_programs(t_list * head);
#endif /* CONSOLA_H_ */
