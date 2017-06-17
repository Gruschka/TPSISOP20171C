/*
 * parser.c
 *
 *  Created on: 06/10/2014
 *      Author: utnso
 */

#include "parser.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int parser_getBesoFromFile(char *name, char **buffer){
	FILE *file;
	unsigned long fileLen;

	//Open file
	file = fopen(name, "rb");
	if (!file)
	{
	fprintf(stderr, "Unable to open file %s", name);
	}

	//Get file length
	fseek(file, 0, SEEK_END);
	fileLen=ftell(file);
	fseek(file, 0, SEEK_SET);

	//Allocate memory
	*buffer=(char *)malloc(fileLen+1);
	if (!*buffer)
	{
		fprintf(stderr, "Memory error!");
		fclose(file);
	}

	//Read file contents into buffer
	fread(*buffer, fileLen, 1, file);
	fclose(file);

	//Do what ever with buffer
	return fileLen;
}
