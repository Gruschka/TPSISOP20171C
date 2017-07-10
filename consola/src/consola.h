/*
 * consola.h
 *
 *  Created on: 9/4/2017
 *      Author: utnso
 */

#ifndef CONSOLA_H_
#define CONSOLA_H_

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>
#include <netdb.h>



typedef struct t_process {
	pthread_t threadID;
	uint32_t processId;
	int kernelSocket,consoleImpressions;
	time_t startTime,endTime;
	struct t_process * processMemoryAddress;
} t_process;



void showMenu();
void startProgram(char * programPath);
int requestPid();
t_process *getThreadfromPid(int aPid);
int getIndexFromTid(pthread_t tid);
void endProgram(int pid);
void disconnectConsole();
void clearConsole();
void requestFilePath(char *filePath);
void *executeProgram(void *arg);
void connectToKernel(char * program);
int parser_getAnSISOPFromFile(char *name, void **buffer);
void dump_buffer(void *buffer, int size);
void showCurrentThreads();
int noThreadsInExecution();
void showFinishedThreadInfo(t_process * aProcess);
void programThread_sig_handler(int signo);



#endif /* CONSOLA_H_ */
