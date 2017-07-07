/*
 ============================================================================
 Name        : filesystem.c
 Author      : Hernan Canzonetta
 Version     :
 Copyright   : Copyright
 Description : Hello World in C, Ansi-style
 ============================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <commons/config.h>
#include <commons/log.h>
#include <commons/bitarray.h>
#include <errno.h>
#include <math.h>
#include "filesystem.h"
#include <sys/mman.h>

t_config *config;
t_log *logger;
t_FS myFS;

int fs_loadConfig(t_FS *FS){

	char *mount = config_get_string_value(config,"PUNTO_MONTAJE");

	FS->mountDirectoryPath = malloc(strlen(mount) + 1);

	FS->mountDirectoryPath[0] = '\0';
	strcat(FS->mountDirectoryPath,mount);

	free(mount);

	FS->MetadataDirectoryPath = "/Metadata/";

	char *buffer = malloc(strlen(FS->mountDirectoryPath) + strlen(FS->MetadataDirectoryPath) + 1);

	buffer[0] = '\0';
	strcat(buffer,FS->mountDirectoryPath);
	strcat(buffer,FS->MetadataDirectoryPath+1);
	FS->MetadataDirectoryPath = buffer;

	FS->FSMetadataFileName = "Metadata.bin";
	buffer = malloc(strlen(FS->MetadataDirectoryPath) + strlen(FS->FSMetadataFileName) + 1);
	buffer[0] = '\0';
	strcat(buffer,FS->MetadataDirectoryPath);
	strcat(buffer,FS->FSMetadataFileName);
	FS->FSMetadataFileName = buffer;

	FS->bitmapFileName = "Bitmap.bin";
	buffer = malloc(strlen(FS->MetadataDirectoryPath) + strlen(FS->bitmapFileName) + 1);
	buffer[0] = '\0';
	strcat(buffer,FS->MetadataDirectoryPath);
	strcat(buffer,FS->bitmapFileName);
	FS->bitmapFileName = buffer;

	FS->dataDirectoryPath = "/Bloques/";
	buffer = malloc(strlen(FS->mountDirectoryPath) + strlen(FS->dataDirectoryPath) + 1);
	buffer[0] = '\0';
	strcat(buffer,FS->mountDirectoryPath);
	strcat(buffer,FS->dataDirectoryPath+1);
	FS->dataDirectoryPath = buffer;

	FS->filesDirectoryPath = "/Archivos/";
	buffer = malloc(strlen(FS->mountDirectoryPath) + strlen(FS->filesDirectoryPath) + 1);
	buffer[0] = '\0';
	strcat(buffer,FS->mountDirectoryPath);
	strcat(buffer,FS->filesDirectoryPath+1);
	FS->filesDirectoryPath = buffer;

	FS->mountDirectory = NULL;
	FS->dataDirectory = NULL;
	FS->filesDirectory = NULL;
	FS->metadataDirectory = NULL;

	FS->metadata.blockAmount=5192;
	FS->metadata.blockSize=64;
	FS->metadata.magicNumber = "SADICA";
	return 0;
}
int fs_mount(t_FS *FS){
	myFS.mountDirectory = opendir(myFS.mountDirectoryPath);

	//check if mount path exists then open
	if(myFS.mountDirectory == NULL){
		// doesnt exist then create
		int error = mkdir(myFS.mountDirectoryPath,0777);
		if(!error){
			log_debug(logger,"FS Mount path created on directory %s", myFS.mountDirectoryPath);
		}else{
			log_error(logger,"error creating mount path %s",strerror(errno));
			return EXIT_FAILURE;
		}
	}
	log_debug(logger,"found FS mount path");

	fs_openOrCreateMetadata(&myFS);

	log_debug(logger,"looking for file directory %s", FS->filesDirectoryPath);
	FS->filesDirectory = opendir(FS->filesDirectoryPath);
	if(myFS.filesDirectory == NULL){
		// doesnt exist then create
		int error = mkdir(FS->filesDirectoryPath,0777);
		if(!error){
			log_debug(logger,"file directory created %s", FS->filesDirectoryPath);
		}else{
			log_error(logger,"Error creating file directory: %s",strerror(errno));
			return EXIT_FAILURE;
		}
	}
	log_debug(logger,"found file directory %s", FS->filesDirectoryPath);

	log_debug(logger,"looking for blocks directory %s", FS->dataDirectoryPath);
	FS->dataDirectory = opendir(FS->dataDirectoryPath);
	if(myFS.dataDirectory == NULL){
		// doesnt exist then create
		int error = mkdir(FS->dataDirectoryPath,0777);
		if(!error){
			log_debug(logger,"data directory created %s", FS->dataDirectoryPath);
		}else{
			log_error(logger,"Error creating data directory: %s",strerror(errno));
			return EXIT_FAILURE;
		}
	}
	log_debug(logger,"found data directory %s", FS->dataDirectoryPath);

	log_info(logger,"FS successfully mounted");
	return EXIT_SUCCESS;

}
int fs_openOrCreateMetadata(t_FS *FS){
	// open/create metadata dir

	FS->metadataDirectory = opendir(FS->MetadataDirectoryPath);
	if(FS->metadataDirectory == NULL){
		// doesnt exist then create
		int error = mkdir(FS->MetadataDirectoryPath,0777);
		if(!error){
			log_debug(logger,"Metadata directory created on directory %s", FS->MetadataDirectoryPath);
		}else{
			log_error(logger,"Error creating metadata directory: %s",strerror(errno));
			return EXIT_FAILURE;
		}
	}
	//created or found
	log_debug(logger,"Found and opened Metadata directory %s", FS->MetadataDirectoryPath);
	fs_openOrCreateMetadataFiles(FS,FS->metadata.blockSize,FS->metadata.blockAmount,FS->metadata.magicNumber);
	return EXIT_SUCCESS;
}
int fs_openOrCreateMetadataFiles(t_FS *FS, int blockSize, int blockAmount, char *magicNumber){
	FILE *metadataFileDescriptor;
	FILE *bitmapFileDescriptor;
	char buffer[50];
	char *numberToString;
	int numberLength;
	t_FSMetadata checkMetadata;

	log_debug(logger,"Looking for metadata file");
	if(metadataFileDescriptor = fopen(FS->FSMetadataFileName,"r+")){
		log_debug(logger,"found metadata file, checking FS parameters");
		checkMetadata = fs_getMetadataFromFile(metadataFileDescriptor);
		if(checkMetadata.blockAmount == blockAmount && checkMetadata.blockSize == blockSize){
			log_debug(logger,"metadata parameters match");
			fclose(metadataFileDescriptor);
		}else{
			log_error(logger,"metadata parameters don't match, aborting launch");
			fclose(metadataFileDescriptor);
			return EXIT_FAILURE;
		}

	}else{
		log_debug(logger,"metadata file not found creating with parameters");
		metadataFileDescriptor = fopen(FS->FSMetadataFileName,"w+");
		memset(buffer,0,50);
		sprintf(buffer,"TAMANIO_BLOQUES=%d\n",blockSize);
		fputs(buffer,metadataFileDescriptor);

		memset(buffer,0,50);

		sprintf(buffer,"CANTIDAD_BLOQUES=%d\n",blockAmount);
		fputs(buffer,metadataFileDescriptor);

		memset(buffer,0,50);
		sprintf(buffer,"MAGIC_NUMBER=%s\n",magicNumber);
		fputs(buffer,metadataFileDescriptor);
		fclose(metadataFileDescriptor);
	}
	log_debug(logger,"looking for bitmap file");
	if(bitmapFileDescriptor = fopen(FS->bitmapFileName,"rb+")){
		log_debug(logger,"found bitmap file");
		struct stat st;
		fstat(bitmapFileDescriptor,&st);
		int size = st.st_size;

		char *bitarray = malloc(FS->metadata.blockAmount);
		FS->bitmap->bitarray = mmap(0,FS->metadata.blockAmount,PROT_WRITE,MAP_PRIVATE,bitmapFileDescriptor,0);

		FS->bitmap = bitarray_create(bitarray,FS->metadata.blockAmount);

	}else{
		log_debug(logger,"bitmap file not found creating with parameters");
		bitmapFileDescriptor = fopen(FS->bitmapFileName,"wb+");
	    int error = lseek(bitmapFileDescriptor, FS->metadata.blockAmount-1, SEEK_SET);
	    if (error == -1) {
		close(bitmapFileDescriptor);
		log_error(logger,"Error calling lseek() to 'stretch' the file: %s",strerror(errno));
	        return EXIT_FAILURE;
	    }

		char *bitarray;

		FS->bitmap = bitarray_create_with_mode(bitarray,FS->metadata.blockAmount,LSB_FIRST);
		FS->bitmap->bitarray = mmap(0,FS->metadata.blockAmount,PROT_WRITE,MAP_PRIVATE,bitmapFileDescriptor,0);


		fclose(bitmapFileDescriptor);
		log_debug(logger,"bitmap created with parameters");
	}

	return EXIT_SUCCESS;

}
t_FSMetadata fs_getMetadataFromFile(FILE *fileDescriptor){
	char buffer[50];
	char *value;
	t_FSMetadata output;

	memset(buffer,0,50);
	memset(value,0,50);

	fgets(buffer,50,fileDescriptor);
	strtok_r(buffer,"=",&value);
	output.blockSize = atoi(value);

	fgets(buffer,50,fileDescriptor);
	strtok_r(buffer,"=",&value);
	output.blockAmount = atoi(value);

	fgets(buffer,50,fileDescriptor);
	strtok_r(buffer,"=",&value);
	output.magicNumber = malloc(strlen(value)+1);
	memcpy(output.magicNumber,value,strlen(value)+1);

	return output;

}
int fs_validateFile(char *path){
	FILE *fileDescriptor;
	if(fileDescriptor = fopen(path,"r+")){
		close(fileDescriptor);
		return EXIT_SUCCESS;
	}else{
		close(fileDescriptor);
		return EXIT_FAILURE;
	}
}
int fs_createFile(char *path){
	FILE *newFileDescriptor;
	size_t firstFreeBlock;
	char bloques[50];
	memset(bloques,0,50);

	if(fs_validateFile(path)){
		newFileDescriptor = fopen(path,"w+");
		firstFreeBlock = fs_getFirstFreeBlock(&myFS);
		fputs("TAMANIO=0\n",newFileDescriptor);
		sprintf(bloques,"BLOQUES=[%d]\n",firstFreeBlock);
		fputs(bloques,newFileDescriptor);
		fs_createBlockFile(firstFreeBlock);
		bitarray_set_bit(myFS.bitmap,firstFreeBlock);
		return EXIT_SUCCESS;
	}
}
int fs_createBlockFile(int blockNumber){
	char *fileName;
	int fileNameLength = 0;

	fileNameLength += strlen(myFS.dataDirectoryPath) + fs_getNumberOfDigits(blockNumber) +strlen(".bin") + 1;
	fileName= malloc(fileNameLength);

	sprintf(fileName,"%s%d.bin\n",myFS.dataDirectoryPath,blockNumber);

	FILE *fileDescriptor = fopen(fileName,"w+");
	free(fileName);
	close(fileDescriptor);
	return EXIT_SUCCESS;

}
int fs_getFirstFreeBlock(t_FS *FS){
	int counter = 0;
	while(counter < FS->metadata.blockAmount){
		if(!bitarray_test_bit(FS->bitmap,counter)){
			return counter;
		}
		counter++;
	}
	return -1;
}
int fs_getNumberOfDigits(int number){
	if(!number){
		return 1;
	}
	int compare = number;
	int counter = 0;
	while(compare > 0){
		compare = compare / 10;
		counter++;
	}
	return counter;
}

int main(int argc, char **argv) {
	char *logFile = tmpnam(NULL);

	logger = log_create(logFile, "FS", 1, LOG_LEVEL_DEBUG);

	if (argc < 2) {
		log_error(logger, "Falta pasar la ruta del archivo de configuracion");
		return EXIT_FAILURE;
	} else {
		config = config_create(argv[1]);
	}
	puts("Holis"); /* prints Holis */

	fs_loadConfig(&myFS);



	fs_mount(&myFS);

	fs_createFile("/mnt/SADICA_FS/Archivos/prueba1.bin");
	fs_createFile("/mnt/SADICA_FS/Archivos/prueba2.bin");
	fs_createFile("/mnt/SADICA_FS/Archivos/prueba3.bin");
	fs_createFile("/mnt/SADICA_FS/Archivos/prueba4.bin");

	return EXIT_SUCCESS;
}
