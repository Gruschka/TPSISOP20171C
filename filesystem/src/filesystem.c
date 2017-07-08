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
#include <fcntl.h>
#include <dirent.h>
#include <commons/config.h>
#include <commons/log.h>
#include <commons/bitarray.h>
#include <commons/string.h>
#include <errno.h>
#include <math.h>
#include "filesystem.h"
#include <sys/mman.h>

t_config *config;
t_log *logger;
t_FS myFS;

int fs_loadConfig(t_FS *FS){ //Llena la estructura del FS segun el archivo config

	//char *mount = config_get_string_value(config,"PUNTO_MONTAJE");

	char *mount = "/mnt/SADICA_FS/";

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

	FS->metadata.blockAmount=5192;
	FS->metadata.blockSize=64;
	FS->metadata.magicNumber = "SADICA";
	return 0;
}
int fs_mount(t_FS *FS){  //Crea el mount path
	DIR *mountDirectory = opendir(myFS.mountDirectoryPath);

	//check if mount path exists then open
	if(mountDirectory == NULL){
		// doesnt exist then create
		int error = mkdir(myFS.mountDirectoryPath,511);
		// 511 is decimal for mode 777 (octal)
		chmod(myFS.mountDirectoryPath,511);
		if(!error){
			log_debug(logger,"FS Mount path created on directory %s", myFS.mountDirectoryPath);
		}else{
			log_error(logger,"error creating mount path %s",strerror(errno));
			return EXIT_FAILURE;
		}
	}
	log_debug(logger,"found FS mount path");
	closedir(mountDirectory);

	fs_openOrCreateMetadata(&myFS);

	DIR *filesDirectory;
	log_debug(logger,"looking for file directory %s", FS->filesDirectoryPath);
	filesDirectory = opendir(FS->filesDirectoryPath);
	if(filesDirectory == NULL){
		// doesnt exist then create
		int error = mkdir(FS->filesDirectoryPath,511);
		chmod(myFS.mountDirectoryPath,511);
		if(!error){
			log_debug(logger,"file directory created %s", FS->filesDirectoryPath);
		}else{
			log_error(logger,"Error creating file directory: %s",strerror(errno));
			return EXIT_FAILURE;
		}
	}
	log_debug(logger,"found file directory %s", FS->filesDirectoryPath);
	closedir(filesDirectory);

	DIR *dataDirectory;
	log_debug(logger,"looking for blocks directory %s", FS->dataDirectoryPath);
	dataDirectory = opendir(FS->dataDirectoryPath);
	if(dataDirectory == NULL){
		// doesnt exist then create
		int error = mkdir(FS->dataDirectoryPath,511);
		chmod(myFS.mountDirectoryPath,511);
		if(!error){
			log_debug(logger,"data directory created %s", FS->dataDirectoryPath);
		}else{
			log_error(logger,"Error creating data directory: %s",strerror(errno));
			return EXIT_FAILURE;
		}
	}
	log_debug(logger,"found data directory %s", FS->dataDirectoryPath);
	closedir(dataDirectory);
	log_info(logger,"FS successfully mounted");
	return EXIT_SUCCESS;

}
int fs_openOrCreateMetadata(t_FS *FS){ //Abre o crea la carpeta Metadata
	// open/create metadata dir
	DIR *metadataDirectory;
	metadataDirectory = opendir(FS->MetadataDirectoryPath);
	if(metadataDirectory == NULL){
		// doesnt exist then create
		int error = mkdir(FS->MetadataDirectoryPath,511);
		chmod(FS->MetadataDirectoryPath,511);
		if(!error){
			log_debug(logger,"Metadata directory created on directory %s", FS->MetadataDirectoryPath);
		}else{
			log_error(logger,"Error creating metadata directory: %s",strerror(errno));
			return EXIT_FAILURE;
		}
	}
	//created or found
	log_debug(logger,"Found and opened Metadata directory %s", FS->MetadataDirectoryPath);
	closedir(metadataDirectory);
	fs_openOrCreateMetadataFiles(FS,FS->metadata.blockSize,FS->metadata.blockAmount,FS->metadata.magicNumber);
	return EXIT_SUCCESS;
}
int fs_openOrCreateMetadataFiles(t_FS *FS, int blockSize, int blockAmount, char *magicNumber){ //Abre o crea archivos Metadata.bin y Bitmap.bin
	FILE *metadataFileDescriptor;
	FILE *bitmapFileDescriptor;
	char buffer[50];

	t_FSMetadata checkMetadata;

	log_debug(logger,"Looking for metadata file");
	if(metadataFileDescriptor = fopen(FS->FSMetadataFileName,"r+")){
		log_debug(logger,"found metadata file, checking FS parameters");
		checkMetadata = fs_getFSMetadataFromFile(metadataFileDescriptor);
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

		fclose(bitmapFileDescriptor);


		FS->bitmapFileDescriptor = open(FS->bitmapFileName,O_RDWR);

		struct stat fileStats;
		fstat(FS->bitmapFileDescriptor, &fileStats);

		char* mapPointer = mmap(0,fileStats.st_size, PROT_WRITE, MAP_SHARED, FS->bitmapFileDescriptor, 0);

		FS->bitmap = bitarray_create_with_mode(mapPointer, FS->metadata.blockAmount/8, LSB_FIRST);

		log_debug(logger,"opened bitmap from disk");


	}else{
		log_debug(logger,"bitmap file not found creating with parameters");
		bitmapFileDescriptor = fopen(FS->bitmapFileName,"wb+");
		chmod(FS->bitmapFileName,511);

		int error = fs_writeNBytesOfXToFile(bitmapFileDescriptor,FS->metadata.blockAmount,0);

		//int error = lseek(bitmapFileDescriptor, FS->metadata.blockAmount-1, SEEK_SET);
	    //fputc('\0', bitmapFileDescriptor);

	    if (error == -1) {
		fclose(bitmapFileDescriptor);
		log_error(logger,"Error calling lseek() to 'stretch' the file: %s",strerror(errno));
	        return EXIT_FAILURE;
	    }
	    fclose(bitmapFileDescriptor);

		char* bloques = string_new();
		string_append(&bloques , "prueba");

		int mmapFileDescriptor = open(FS->bitmapFileName, O_RDWR);
		FS->bitmapFileDescriptor = mmapFileDescriptor;
		struct stat scriptMap;
		fstat(mmapFileDescriptor, &scriptMap);
		int aux =string_length(bloques);

		char* mapPointer = mmap(0,scriptMap.st_size, PROT_WRITE, MAP_SHARED, mmapFileDescriptor, 0);

		FS->bitmap = bitarray_create_with_mode(mapPointer, FS->metadata.blockAmount/8, LSB_FIRST);


		log_debug(logger,"bitmap created with parameters");
	}

	return EXIT_SUCCESS;

}
t_FSMetadata fs_getFSMetadataFromFile(FILE *fileDescriptor){ //Recupera valores del archivo Metadata
	char buffer[50];
	char *blockSizeBuffer;//blockSizeBuffer = 0x1
	char *blockAmountBuffer;
	char *magicNumberBuffer;
	t_FSMetadata output;

	memset(buffer,0,50);
	memset(blockSizeBuffer,0,sizeof(int)+1); // set contenido de 0x1 a 0

	fgets(buffer,50,fileDescriptor);

	strtok_r(buffer,"=",&blockSizeBuffer);
	output.blockSize = atoi(blockSizeBuffer);

	fgets(buffer,50,fileDescriptor);
	strtok_r(buffer,"=",&blockAmountBuffer);
	output.blockAmount = atoi(blockAmountBuffer);


	memset(magicNumberBuffer,0,50);

	fgets(buffer,50,fileDescriptor);
	strtok_r(buffer,"=",&magicNumberBuffer);
	output.magicNumber = malloc(strlen(magicNumberBuffer)+1);
	memcpy(output.magicNumber,magicNumberBuffer,strlen(magicNumberBuffer)+1);

	return output;

}

t_FileMetadata fs_getMetadataFromFile(FILE* filePointer){

		char lineBuffer[255];
		char *metadataSizeBuffer;
		char *metadataBlocksBuffer;
		int * blocks;
		t_FileMetadata output;

		memset(lineBuffer,0,255);

		fgets(lineBuffer,255,filePointer); // lineBuffer = TAMANIO=128
		strtok_r(lineBuffer,"=",&metadataSizeBuffer); // lineBuffer = TAMANIO\0128 => lineBuffer = TAMANIO y metadataSizeBuffer = 128\n
		output.size = atoi(metadataSizeBuffer);

		memset(lineBuffer,0,255);

		//strtok con igual => BLOQUES\0[4,1231,5,1,0,12,543]
		fgets(lineBuffer,255,filePointer);
		strtok_r(lineBuffer,"=",&metadataBlocksBuffer);

		char *stringArray = malloc(strlen(metadataBlocksBuffer));
		memset(stringArray,0,strlen(metadataBlocksBuffer)+1);
		memcpy(stringArray,metadataBlocksBuffer,strlen(metadataBlocksBuffer)-1);

		output.blocks = fs_getArrayFromString(&stringArray,(output.size +(myFS.metadata.blockSize -1))/myFS.metadata.blockSize);

		if(output.blocks == -1){

			log_error(logger, "Critical error reading blocks");
			return output;
		}

		free(stringArray);

		return output;



}
//recibe [4,1231,5,1,0,12,543] devuelve int*
int fs_validateFile(char *path){ //Se fija si un path existe
	FILE *fileDescriptor;
	if(fileDescriptor = fopen(path,"r+")){
		close(fileDescriptor);
		return EXIT_SUCCESS;
	}else{
		close(fileDescriptor);
		return EXIT_FAILURE;
	}
}
int fs_createFile(char *path){ //Crea archivo nuevo
	FILE *newFileDescriptor;
	size_t firstFreeBlock;
	char bloques[50];
	memset(bloques,0,50);

	if(fs_validateFile(path)){
		newFileDescriptor = fopen(path,"w+"); //Crea archivo Metadata del archivo nuevo
		firstFreeBlock = fs_getFirstFreeBlock(&myFS);
		if(firstFreeBlock == -1) return -1;
		fputs("TAMANIO=0\n",newFileDescriptor);
		sprintf(bloques,"BLOQUES=[%d]\n",firstFreeBlock);
		fputs(bloques,newFileDescriptor);
		fs_createBlockFile(firstFreeBlock);
		bitarray_set_bit(myFS.bitmap,firstFreeBlock);
		fclose(newFileDescriptor);
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
	fclose(fileDescriptor);
	return EXIT_SUCCESS;

}
int fs_deleteBlockFile(int blockNumber){

	char *fileName;
	int fileNameLength = 0;

	fileNameLength += strlen(myFS.dataDirectoryPath) + fs_getNumberOfDigits(blockNumber) + strlen(".bin") + 1;
	fileName = malloc(fileNameLength);

	sprintf(fileName,"%s%d.bin\n",myFS.dataDirectoryPath,blockNumber);

	remove(fileName);
	free(fileName);

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
int fs_writeNBytesOfXToFile(FILE *fileDescriptor, int N, int C){ //El tamanio del archivo antes del mmap matchea con el tamanio del bitmap
	char *buffer = malloc(N);

	memset(buffer,C,N);
	fwrite(buffer,N,1,fileDescriptor);
	return EXIT_SUCCESS;
}
int *fs_getArrayFromString(char **stringArray, int numberOfBlocks){
	//[0]
	char *token;
	int blockBuffer;
	int *array = malloc(sizeof(int)*numberOfBlocks);



	//Removes brackets from stringArray and saves blocks on stringPointer
	char*stringPointer = string_substring_from(*stringArray ,1); //string_substring returns a pointer to a different direction
	stringPointer  = string_substring_until(stringPointer ,strlen(stringPointer)-1);


	//Takes first block from stringPointer
	token = strtok(stringPointer,",");

	if(!token){
		return -1;
	}

	//Saves in buffer the value from token as an int
	blockBuffer = atoi(token);

	//Saves the new block on the array pointer
	array[0] = blockBuffer;
	int iterator =1;

	//Iterates from the last position of the token pointer (NULL)
	while(token != NULL){

		token = strtok(NULL,",");
		if(token!=NULL){
			blockBuffer = atoi(token);
			array[iterator] = blockBuffer;
			iterator++;
		}

	}

	return array;
}
int fs_removeFile(char* filePath){

	if(fs_validateFile(filePath)){ //If the path is invalid
			log_error(logger, "Path invalido - No se puede eliminar archivo");
			return EXIT_FAILURE;
	}

	FILE *filePointer = fopen(filePath, "r+");
	t_FileMetadata fileMetadata = fs_getMetadataFromFile(filePointer);



	//Borrar bloques
	int iterator = 0;
	int numberOfBlocks = (fileMetadata.size + (myFS.metadata.blockSize -1)) / myFS.metadata.blockSize;
	while(iterator < numberOfBlocks){
		fs_deleteBlockFile(fileMetadata.blocks[iterator]);
		iterator++;
	}

	//Liberar bitmap
	iterator = 0;
	while(iterator < numberOfBlocks){
		bitarray_clean_bit(myFS.bitmap, fileMetadata.blocks[iterator]);
		iterator++;
	}


	fclose(filePointer);

	//Borrar metadata
	remove(filePath);

}
void fs_dump(){
	printf("FS Metadata File Name: %s\n",myFS.FSMetadataFileName);
	printf("FS Metadata Directory Path: %s\n",myFS.MetadataDirectoryPath);
	printf("FS bitmap file descriptor: %d\n",myFS.bitmapFileDescriptor);
	printf("FS Bitmap File Name: %s\n",myFS.bitmapFileName);
	printf("FS data directory path: %s\n",myFS.dataDirectoryPath);
	printf("FS Files directory path: %s\n",myFS.filesDirectoryPath);
	printf("FS block amount: %d\n",myFS.metadata.blockAmount);
	printf("FS block size: %d\n",myFS.metadata.blockSize);
	printf("FS magic number: %s\n",myFS.metadata.magicNumber);
	printf("FS Mount path: %s\n",myFS.mountDirectoryPath);
	printf("BITMAP\n");

	int i = 0;
	int unBit = 0;

	while(i < myFS.metadata.blockAmount){
		unBit = bitarray_test_bit(myFS.bitmap,i);
		printf("%d",unBit);
		i++;
	}

	fflush(stdout);





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


	fs_dump();

	fs_createFile("/mnt/SADICA_FS/Archivos/prueba1.bin");
	fs_createFile("/mnt/SADICA_FS/Archivos/prueba2.bin");
	fs_createFile("/mnt/SADICA_FS/Archivos/prueba3.bin");
	fs_createFile("/mnt/SADICA_FS/Archivos/prueba4.bin");
	fs_removeFile("/mnt/SADICA_FS/Archivos/prueba1.bin");

	fs_dump();

	return EXIT_SUCCESS;
}
