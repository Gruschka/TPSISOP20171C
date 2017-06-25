/*
 * memory.h
 *
 *  Created on: 27/04/2014
 *      Author: utnso
 */

#ifndef MEMORY_H_
#define MEMORY_H_
#include <sys/types.h>
#include <commons/collections/list.h>

typedef struct pointerTranslation{
	u_int32_t translation;
	void *reference;
	u_int32_t size;
}pointerTranslation;

typedef struct Memory{
	void *base;
	int size;
	int lastGivenOffset;
	t_list *pointerTranslations;
}Memory;

extern Memory myMemory;

Memory memory_create(int);
int memory_write(u_int32_t,int,int,void *);
void *memory_read(u_int32_t,int,int);
u_int32_t memory_requestSegment(int);
void *memory_translateDirection(u_int32_t);
pointerTranslation *memory_getTranslation(int);
int memory_getTranslationSize(int);

#endif /* MEMORY_H_ */
