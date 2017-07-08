/*
 * shared_variables.c
 *
 *  Created on: Jun 25, 2017
 *      Author: hcanzonetta
 */
#include "shared_variables.h"

#include <stdlib.h>
#include <string.h>

#include <commons/collections/list.h>

extern t_list *sharedVariables;

shared_variable *createSharedVariable(char *identifier) {
	shared_variable *variable = malloc(sizeof(shared_variable));
	char *identifierBuf = malloc(strlen(identifier));

	memcpy(identifierBuf, identifier + 1, strlen(identifier));
	variable->identifier = identifierBuf;
	variable->value = 0;
	pthread_mutex_init(&variable->mutex, NULL);

	return variable;
}

shared_variable *getSharedVariable(char *identifier) {
	int i = 0;
	shared_variable *variable;
	for (i = 0; i < list_size(sharedVariables); i++) {
		variable = list_get(sharedVariables, i);

		if (strcmp(variable->identifier, identifier) == 0) {
			return variable;
		}
	}

	return NULL;
}

int setSharedVariableValue(char *identifier, uint32_t value) {
	shared_variable *foundVariable = getSharedVariable(identifier);

	if (foundVariable != NULL) {
		pthread_mutex_lock(&foundVariable->mutex);
		foundVariable->value = value;
		pthread_mutex_unlock(&foundVariable->mutex);

		return 0;
	} else {
		return -1;
	}
}

int getSharedVariableValue(char *identifier) {
	shared_variable *foundVariable = getSharedVariable(identifier);

	if (foundVariable != NULL) {
		pthread_mutex_lock(&foundVariable->mutex);
		int value = foundVariable->value;
		pthread_mutex_unlock(&foundVariable->mutex);
		return value;
	}

	//TODO: VER COMO MANEJAR ERROR
	return -1;
}
