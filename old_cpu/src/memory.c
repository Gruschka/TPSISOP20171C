/*
 * memory.c
 *
 *  Created on: 27/04/2014
 *      Author: utnso
 */

#include "memory.h"
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

Memory memory_create(int size){
	Memory newMemory;
	newMemory.base = malloc(size);
	newMemory.size = size;
	newMemory.lastGivenOffset = 0;
	newMemory.pointerTranslations = list_create();
	return newMemory;
}

int memory_write(u_int32_t base, int offset,int size,void *value){
	void *pointer = memory_translateDirection(base);
	pointer += offset;
	memcpy(pointer,value,size);
	return 0;
}

void *memory_read(u_int32_t base, int offset, int size){
	void *pointer = memory_translateDirection(base);
	pointer += offset;
	void *buffer = malloc(size);
	memcpy(buffer,pointer,size);
	return buffer;
}

u_int32_t memory_requestSegment(int size){
	if(size != 0){
		void *segment = malloc(size);
		pointerTranslation *newTranslation = malloc(sizeof(pointerTranslation));
		newTranslation->translation = 0 + myMemory.lastGivenOffset;
		newTranslation->reference = segment;
		newTranslation->size = size;
		list_add(myMemory.pointerTranslations,newTranslation);
		myMemory.lastGivenOffset += size;
		return newTranslation->translation;
	}else{
		return 0;
	}

}//a

int memory_getTranslationSize(int index){
	return 0;
}
void *memory_translateDirection(u_int32_t translation){
	int iterator = 0;
	void *newReference;
	u_int32_t segment = memory_getTranslation(iterator)->translation;
	u_int32_t segmentSize = memory_getTranslation(iterator)->size;
	int offset = 0;
	while(iterator <= myMemory.pointerTranslations->elements_count){
		if(segment == translation){
			return memory_getTranslation(iterator)->reference;
		}else{
			if((translation > segment) && (translation < segment+segmentSize)){
				offset = translation - segment;
				newReference = memory_getTranslation(iterator)->reference + offset;
				return newReference;
			}
			iterator++;
			segment = memory_getTranslation(iterator)->translation;
			segmentSize = memory_getTranslation(iterator)->size;
		}
	}
	return 0;
}

pointerTranslation *memory_getTranslation(int index){
	return list_get(myMemory.pointerTranslations,index);
}
