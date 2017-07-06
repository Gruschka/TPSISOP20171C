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

t_config *config;
t_log *logger;
t_FS myFS;

int fs_loadConfig(t_FS *FS){

	char *mount = config_get_string_value(config,"PUNTO_MONTAJE");

	FS->mountDirectoryPath = malloc(strlen(mount) + 1);

	FS->mountDirectoryPath[0] = '\0';
	strcat(FS->mountDirectoryPath,mount);

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
int fs_mount(t_FS *FS, char *path){
	//check if mount path exists then open
	FS->mountDirectory = opendir(path);
	if(myFS.mountDirectory == NULL){
		// doesnt exist then create
		int error = mkdir(path,0777);
		if(!error){
			log_debug(logger,"FS Mount path created on directory %s", path);
		}else{
			log_error(logger,"Error creating mount directory: %s",strerror(errno));
			return EXIT_FAILURE;
		}
	}
	log_debug(logger,"found FS mount path");

	fs_openOrCreateMetadata();

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
int fs_openOrCreateMetadata(){
	// open/create metadata dir

	myFS.metadataDirectory = opendir(myFS.MetadataDirectoryPath);
	if(myFS.metadataDirectory == NULL){
		// doesnt exist then create
		int error = mkdir(myFS.MetadataDirectoryPath,0777);
		if(!error){
			log_debug(logger,"Metadata directory created on directory %s", myFS.MetadataDirectoryPath);
		}else{
			log_error(logger,"Error creating metadata directory: %s",strerror(errno));
			return EXIT_FAILURE;
		}
	}
	//created or found
	log_debug(logger,"Found and opened Metadata directory %s", myFS.MetadataDirectoryPath);
	fs_openOrCreateMetadataFiles(myFS.metadata.blockSize,myFS.metadata.blockAmount,myFS.metadata.magicNumber);
	return EXIT_SUCCESS;
}
int fs_openOrCreateMetadataFiles(int blockSize, int blockAmount, char *magicNumber){
	FILE *metadataFileDescriptor;
	FILE *bitmapFileDescriptor;
	char buffer[50];
	char *numberToString;
	int numberLength;
	t_FSMetadata checkMetadata;

	log_debug(logger,"Looking for metadata file");
	if(metadataFileDescriptor = fopen(myFS.FSMetadataFileName,"r+")){
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
		metadataFileDescriptor = fopen(myFS.FSMetadataFileName,"w+");
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
	if(bitmapFileDescriptor = fopen(myFS.bitmapFileName,"rb+")){
		log_debug(logger,"found bitmap file");
	}else{
		log_debug(logger,"bitmap file not found creating with parameters");
		bitmapFileDescriptor = fopen(myFS.bitmapFileName,"wb+");
		char *bitarray = malloc(myFS.metadata.blockAmount);
		memset(bitarray,0,myFS.metadata.blockAmount);
		myFS.bitmap = bitarray_create_with_mode(bitarray,myFS.metadata.blockAmount,LSB_FIRST);

		fwrite(myFS.bitmap->bitarray,myFS.metadata.blockAmount,1,bitmapFileDescriptor);

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



	fs_mount(&myFS,myFS.mountDirectoryPath);

	return EXIT_SUCCESS;
}
