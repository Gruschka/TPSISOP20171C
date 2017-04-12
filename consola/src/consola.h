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
int connectToKernel(char *);

#endif /* CONSOLA_H_ */
