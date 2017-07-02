/*
 * shared_variables.h
 *
 *  Created on: Jun 25, 2017
 *      Author: utnso
 */

#ifndef SHARED_VARIABLES_H_
#define SHARED_VARIABLES_H_

#include <pthread.h>
#include <stdint.h>

typedef struct shared_variable {
	char *identifier;
	int value;
	pthread_mutex_t mutex;
} __attribute__((packed)) shared_variable;

shared_variable *createSharedVariable(char *identifier);
int getSharedVariableValue(char *identifier);
int setSharedVariableValue(char *identifier, uint32_t value);

#endif /* SHARED_VARIABLES_H_ */
