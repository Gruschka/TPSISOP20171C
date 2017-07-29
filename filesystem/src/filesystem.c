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
#include<arpa/inet.h>
#include<netinet/in.h>
#include<sys/socket.h>
#include<sys/types.h>
#include <ipc/ipc.h>

t_config *config;
t_log *logger;
t_FS myFS;

int portno;
int kernelFileDescriptor;
int fs_loadConfig(t_FS *FS) { //Llena la estructura del FS segun el archivo config

	char *mount = config_get_string_value(config,"PUNTO_MONTAJE");
	portno =  config_get_string_value(config,"PUERTO");
	//char *mount = "/mnt/SADICA_FS/";

	FS->mountDirectoryPath = malloc(strlen(mount) + 1);

	FS->mountDirectoryPath[0] = '\0';
	strcat(FS->mountDirectoryPath, mount);

	FS->MetadataDirectoryPath = "/Metadata/";

	char *buffer = malloc(
			strlen(FS->mountDirectoryPath) + strlen(FS->MetadataDirectoryPath)
					+ 1);

	buffer[0] = '\0';
	strcat(buffer, FS->mountDirectoryPath);
	strcat(buffer, FS->MetadataDirectoryPath + 1);
	FS->MetadataDirectoryPath = buffer;

	FS->FSMetadataFileName = "Metadata.bin";
	buffer = malloc(
			strlen(FS->MetadataDirectoryPath) + strlen(FS->FSMetadataFileName)
					+ 1);
	buffer[0] = '\0';
	strcat(buffer, FS->MetadataDirectoryPath);
	strcat(buffer, FS->FSMetadataFileName);
	FS->FSMetadataFileName = buffer;

	FS->bitmapFileName = "Bitmap.bin";
	buffer = malloc(
			strlen(FS->MetadataDirectoryPath) + strlen(FS->bitmapFileName) + 1);
	buffer[0] = '\0';
	strcat(buffer, FS->MetadataDirectoryPath);
	strcat(buffer, FS->bitmapFileName);
	FS->bitmapFileName = buffer;

	FS->dataDirectoryPath = "/Bloques/";
	buffer = malloc(
			strlen(FS->mountDirectoryPath) + strlen(FS->dataDirectoryPath) + 1);
	buffer[0] = '\0';
	strcat(buffer, FS->mountDirectoryPath);
	strcat(buffer, FS->dataDirectoryPath + 1);
	FS->dataDirectoryPath = buffer;

	FS->filesDirectoryPath = "/Archivos/";
	buffer = malloc(
			strlen(FS->mountDirectoryPath) + strlen(FS->filesDirectoryPath)
					+ 1);
	buffer[0] = '\0';
	strcat(buffer, FS->mountDirectoryPath);
	strcat(buffer, FS->filesDirectoryPath + 1);
	FS->filesDirectoryPath = buffer;

	FS->metadata.blockAmount = 5192;
	FS->metadata.blockSize = 64;
	FS->metadata.magicNumber = "SADICA";
	return 0;
}
int fs_mount(t_FS *FS) {  //Crea el mount path y toda la estructura del FS

	//Directorio base
	DIR *mountDirectory = opendir(myFS.mountDirectoryPath);

	//check if mount path exists then open
	if (mountDirectory == NULL) {
		// doesnt exist then create
		int error = mkdir(myFS.mountDirectoryPath, 511);
		// 511 is decimal for mode 777 (octal)
		chmod(myFS.mountDirectoryPath, 511);
		if (!error) {
			log_debug(logger, "FS Mount path created on directory %s",
					myFS.mountDirectoryPath);
		} else {
			log_error(logger, "error creating mount path %s", strerror(errno));
			return EXIT_FAILURE;
		}
	}
	log_debug(logger, "found FS mount path");
	closedir(mountDirectory);

	//Crea (o abre si ya existe) la metadata del FileSystem
	fs_openOrCreateMetadata(&myFS);

	//Crea directorio de archivos
	DIR *filesDirectory;
	log_debug(logger, "looking for file directory %s", FS->filesDirectoryPath);
	filesDirectory = opendir(FS->filesDirectoryPath);
	if (filesDirectory == NULL) {
		// doesnt exist then create
		int error = mkdir(FS->filesDirectoryPath, 511);
		chmod(myFS.mountDirectoryPath, 511);
		if (!error) {
			log_debug(logger, "file directory created %s",
					FS->filesDirectoryPath);
		} else {
			log_error(logger, "Error creating file directory: %s",
					strerror(errno));
			return EXIT_FAILURE;
		}
	}
	log_debug(logger, "found file directory %s", FS->filesDirectoryPath);
	closedir(filesDirectory);

	//Crea directorio de datos (bloques)
	DIR *dataDirectory;
	log_debug(logger, "looking for blocks directory %s", FS->dataDirectoryPath);
	dataDirectory = opendir(FS->dataDirectoryPath);
	if (dataDirectory == NULL) {
		// doesnt exist then create
		int error = mkdir(FS->dataDirectoryPath, 511);
		chmod(myFS.mountDirectoryPath, 511);
		if (!error) {
			log_debug(logger, "data directory created %s",
					FS->dataDirectoryPath);
		} else {
			log_error(logger, "Error creating data directory: %s",
					strerror(errno));
			return EXIT_FAILURE;
		}
	}
	log_debug(logger, "found data directory %s", FS->dataDirectoryPath);
	closedir(dataDirectory);
	log_info(logger, "FS successfully mounted");
	return EXIT_SUCCESS;

}
int fs_openOrCreateMetadata(t_FS *FS) { //Abre o crea la carpeta Metadata
	// open/create metadata dir
	DIR *metadataDirectory;
	metadataDirectory = opendir(FS->MetadataDirectoryPath);
	if (metadataDirectory == NULL) {
		// doesnt exist then create
		int error = mkdir(FS->MetadataDirectoryPath, 511);
		chmod(FS->MetadataDirectoryPath, 511);
		if (!error) {
			log_debug(logger, "Metadata directory created on directory %s",
					FS->MetadataDirectoryPath);
		} else {
			log_error(logger, "Error creating metadata directory: %s",
					strerror(errno));
			return EXIT_FAILURE;
		}
	}
	//created or found
	log_debug(logger, "Found and opened Metadata directory %s",
			FS->MetadataDirectoryPath);
	closedir(metadataDirectory);
	fs_openOrCreateMetadataFiles(FS, FS->metadata.blockSize,
			FS->metadata.blockAmount, FS->metadata.magicNumber);
	return EXIT_SUCCESS;
}
int fs_openOrCreateMetadataFiles(t_FS *FS, int blockSize, int blockAmount,
		char *magicNumber) { //Abre o crea archivos Metadata.bin y Bitmap.bin
	FILE *metadataFileDescriptor;
	FILE *bitmapFileDescriptor;
	char buffer[50];

	t_FSMetadata checkMetadata;

	log_debug(logger, "Looking for metadata file");

	//Intenta abrir
	if (metadataFileDescriptor = fopen(FS->FSMetadataFileName, "r+")) { //Existe el archivo de metadata
		log_debug(logger, "found metadata file, checking FS parameters");
		checkMetadata = fs_getFSMetadataFromFile(metadataFileDescriptor); //Toma la metadata de dicho archivo;
		if (checkMetadata.blockAmount == blockAmount
				&& checkMetadata.blockSize == blockSize) { //Compara si los parametros de metadata matchean
			log_debug(logger, "metadata parameters match");
			fclose(metadataFileDescriptor);
		} else {
			log_error(logger,
					"metadata parameters don't match, aborting launch");
			fclose(metadataFileDescriptor);
			return EXIT_FAILURE;
		}
		free(checkMetadata.magicNumber); //Anda

	} else { //No puede abrirlo => Lo crea
		log_debug(logger, "metadata file not found creating with parameters");
		metadataFileDescriptor = fopen(FS->FSMetadataFileName, "w+");
		memset(buffer, 0, 50);
		sprintf(buffer, "TAMANIO_BLOQUES=%d\n", blockSize);
		fputs(buffer, metadataFileDescriptor);

		memset(buffer, 0, 50);

		sprintf(buffer, "CANTIDAD_BLOQUES=%d\n", blockAmount);
		fputs(buffer, metadataFileDescriptor);

		memset(buffer, 0, 50);
		sprintf(buffer, "MAGIC_NUMBER=%s\n", magicNumber);
		fputs(buffer, metadataFileDescriptor);
		fclose(metadataFileDescriptor);
	}
	log_debug(logger, "looking for bitmap file");
	if (bitmapFileDescriptor = fopen(FS->bitmapFileName, "rb+")) { //Abre bitmap

		fclose(bitmapFileDescriptor);

		FS->bitmapFileDescriptor = open(FS->bitmapFileName, O_RDWR);

		struct stat fileStats;
		fstat(FS->bitmapFileDescriptor, &fileStats);

		//Mappea el bitmap a memoria para que se actualice automaticamente al modificar la memoria
		char* mapPointer = mmap(0, fileStats.st_size, PROT_WRITE, MAP_SHARED,
				FS->bitmapFileDescriptor, 0);

		//Actualiza el bitarray pasando el puntero que devuelve el map
		FS->bitmap = bitarray_create_with_mode(mapPointer,
				FS->metadata.blockAmount / 8, LSB_FIRST);

		log_debug(logger, "opened bitmap from disk");

	} else { //Si no pudo abrir bit map (no lo encuentra) lo crea
		log_debug(logger, "bitmap file not found creating with parameters");
		bitmapFileDescriptor = fopen(FS->bitmapFileName, "wb+");
		chmod(FS->bitmapFileName, 511);

		int error = fs_writeNBytesOfXToFile(bitmapFileDescriptor,
				FS->metadata.blockAmount, 0);

		//int error = lseek(bitmapFileDescriptor, FS->metadata.blockAmount-1, SEEK_SET);
		//fputc('\0', bitmapFileDescriptor);

		if (error == -1) {
			fclose(bitmapFileDescriptor);
			log_error(logger, "Error calling lseek() to 'stretch' the file: %s",
					strerror(errno));
			return EXIT_FAILURE;
		}
		fclose(bitmapFileDescriptor);

		char* bloques = string_new();
		string_append(&bloques, "prueba");

		int mmapFileDescriptor = open(FS->bitmapFileName, O_RDWR);
		FS->bitmapFileDescriptor = mmapFileDescriptor;
		struct stat scriptMap;
		fstat(mmapFileDescriptor, &scriptMap);
		int aux = string_length(bloques);

		char* mapPointer = mmap(0, scriptMap.st_size, PROT_WRITE, MAP_SHARED,
				mmapFileDescriptor, 0);

		FS->bitmap = bitarray_create_with_mode(mapPointer,
				FS->metadata.blockAmount / 8, LSB_FIRST);

		log_debug(logger, "bitmap created with parameters");
	}

	return EXIT_SUCCESS;

}
t_FSMetadata fs_getFSMetadataFromFile(FILE *fileDescriptor) { //Recupera valores del archivo Metadata
	char buffer[50];
	char *blockSizeBuffer; //blockSizeBuffer = 0x1
	char *blockAmountBuffer;
	char *magicNumberBuffer;
	t_FSMetadata output;

	memset(buffer, 0, 50);
	memset(blockSizeBuffer, 0, sizeof(int) + 1); // set contenido de 0x1 a 0

	fgets(buffer, 50, fileDescriptor);

	strtok_r(buffer, "=", &blockSizeBuffer);
	output.blockSize = atoi(blockSizeBuffer);

	fgets(buffer, 50, fileDescriptor);
	strtok_r(buffer, "=", &blockAmountBuffer);
	output.blockAmount = atoi(blockAmountBuffer);

	memset(magicNumberBuffer, 0, 50);

	fgets(buffer, 50, fileDescriptor);
	strtok_r(buffer, "=", &magicNumberBuffer);
	output.magicNumber = malloc(strlen(magicNumberBuffer) + 1);
	memcpy(output.magicNumber, magicNumberBuffer,
			strlen(magicNumberBuffer) + 1);

	return output;

}
t_FileMetadata fs_getMetadataFromFile(FILE* filePointer) {

	char lineBuffer[255];
	char *metadataSizeBuffer;
	char *metadataBlocksBuffer;
	int * blocks;
	t_FileMetadata output;

	memset(lineBuffer, 0, 255);

	fgets(lineBuffer, 255, filePointer); // lineBuffer = TAMANIO=128
	strtok_r(lineBuffer, "=", &metadataSizeBuffer); // lineBuffer = TAMANIO\0128 => lineBuffer = TAMANIO y metadataSizeBuffer = 128\n
	output.size = atoi(metadataSizeBuffer);

	char *compiledRead;
	//strtok con igual => BLOQUES\0[4,1231,5,1,0,12,543]
	int blockCount = 1 + (output.size / myFS.metadata.blockSize);
	char *stringArray;
	//memset(stringArray, 0, 255);
	int stringArrayOffset = 0;
	int blockNumber;
	int iterator = 0;

	fgets(lineBuffer, strlen("BLOQUES=[") + 1, filePointer);
	memset(lineBuffer, 0, 255);
	char *temporalBuffer;

	int firstLine = 1;

	//Mientras haya para leer del archivo de metadata lo vuelco en el lineBuffer
	while (fgets(lineBuffer, 255, filePointer)) {

		if (!firstLine) { //Si  es la primera linea linea, no crea backup
			temporalBuffer = malloc(stringArrayOffset);
			memset(temporalBuffer, 0, stringArrayOffset);
			memcpy(temporalBuffer, stringArray, stringArrayOffset);
			free(stringArray);
		}

		//String array guarda los bloques sin corchetes, y se va alocando segun lo que tenga linebuffer + leido acumulado (realloc casero)
		stringArray = malloc(strlen(lineBuffer) + stringArrayOffset);
		memset(stringArray, 0, strlen(lineBuffer) + stringArrayOffset);
		if (!firstLine)
			memcpy(stringArray, temporalBuffer, stringArrayOffset);
		//Si es la primera linea => strinArrayOffset = 0 (arranca del principio)
		memcpy(stringArray + stringArrayOffset, lineBuffer, strlen(lineBuffer));
		if (!firstLine)
			free(temporalBuffer);
		stringArrayOffset += strlen(lineBuffer); //Acumulo lo que acabo de leer
		firstLine = 0;
		memset(lineBuffer, 0, 255);
	}

	strtok_r(stringArray, "]", &metadataBlocksBuffer); //Queda un string con los bloques sin los corchetes

	output.blocks = fs_getArrayFromString(&stringArray, blockCount); //Convierte string a array

	if (output.blocks == -1) {

		log_error(logger, "Critical error reading blocks");
		return output;
	}

	free(stringArray);

	return output;

}
int fs_validateFile(char *file) { //Se fija si un path existe
	FILE *fileDescriptor;

	if (fileDescriptor = fopen(file, "r+")) {
		close(fileDescriptor);
		return EXIT_SUCCESS;
	} else {
		close(fileDescriptor);
		return EXIT_FAILURE;
	}

}
int fs_createFile(char *path) { //Crea archivo nuevo
	FILE *newFileDescriptor;
	size_t firstFreeBlock;
	char bloques[50];
	memset(bloques, 0, 50);

	fs_createSubDirectoriesFromFilePath(path);

	if (fs_validateFile(path)) { //Si el archivo no existe
		newFileDescriptor = fopen(path, "w+"); //Crea archivo Metadata del archivo nuevo
		firstFreeBlock = fs_getFirstFreeBlock(&myFS);
		if (firstFreeBlock == -1) {
			log_error(logger, "Error creating file");
			return -1;
		}
		fputs("TAMANIO=0\n", newFileDescriptor); // Arrancan con tamanio 0
		sprintf(bloques, "BLOQUES=[%d]\n", firstFreeBlock); //Pasa info de bloques a bloques y la vuelca al archivo
		fputs(bloques, newFileDescriptor);
		fs_createBlockFile(firstFreeBlock); //Crea archivo de bloque
		fclose(newFileDescriptor);
		return EXIT_SUCCESS;
	}

	return -1;
}
int fs_createBlockFile(int blockNumber) {
	char *fileName;
	int fileNameLength = 0;

	fileNameLength += strlen(myFS.dataDirectoryPath)
			+ fs_getNumberOfDigits(blockNumber) + strlen(".bin") + 1;
	fileName = malloc(fileNameLength);
	memset(fileName, 0, fileNameLength);

	fflush(stdin);
	sprintf(fileName, "%s%d.bin", myFS.dataDirectoryPath, blockNumber);

	FILE *fileDescriptor = fopen(fileName, "w+");
	bitarray_set_bit(myFS.bitmap, blockNumber);

	free(fileName);
	fclose(fileDescriptor);
	return blockNumber;

}
int fs_deleteBlockFile(int blockNumber) {

	char *fileName;
	int fileNameLength = 0;

	fileNameLength += strlen(myFS.dataDirectoryPath)
			+ fs_getNumberOfDigits(blockNumber) + strlen(".bin") + 1;
	fileName = malloc(fileNameLength);

	sprintf(fileName, "%s%d.bin", myFS.dataDirectoryPath, blockNumber);

	remove(fileName);
	free(fileName);

	return EXIT_SUCCESS;

}
int fs_getFirstFreeBlock(t_FS *FS) { //Recorre bitmap hasta encontrar primer bloque no set
	int counter = 0;
	while (counter < FS->metadata.blockAmount) {
		if (!bitarray_test_bit(FS->bitmap, counter)) {
			return counter;
		}
		counter++;
	}
	return -1;
}
int fs_getNumberOfDigits(int number) { //Dado un numero devuelve su cantidad de digitos (usado para alocar espacio justo al crear block files)
	if (!number) {
		return 1;
	}
	int compare = number;
	int counter = 0;
	while (compare > 0) {
		compare = compare / 10;
		counter++;
	}
	return counter;
}
int fs_writeNBytesOfXToFile(FILE *fileDescriptor, int N, int C) { //El tamanio del archivo antes del mmap matchea con el tamanio del bitmap
	char *buffer = malloc(N);

	memset(buffer, C, N);
	fwrite(buffer, N, 1, fileDescriptor);
	free(buffer); //Anda
	return EXIT_SUCCESS;
}
int *fs_getArrayFromString(char **stringArray, int numberOfBlocks) {
	//[0]
	char *token;
	int blockBuffer;
	int *array = malloc(sizeof(int) * numberOfBlocks);

	char *stringPointer = *stringArray;

	//Takes first block from stringPointer
	token = strtok(stringPointer, ",");

	if (!token) {
		log_error(logger, "Fallo al convertir String a Array");
		return -1;
	}

	//Saves in buffer the value from token as an int
	blockBuffer = atoi(token);

	//Saves the new block on the array pointer
	array[0] = blockBuffer;
	int iterator = 1;

	//Iterates from the last position of the token pointer (NULL)
	while (token != NULL) {

		token = strtok(NULL, ",");
		if (token != NULL) {
			blockBuffer = atoi(token);
			array[iterator] = blockBuffer;
			iterator++;
		}

	}

	return array;
}
int fs_removeFile(char* filePath) {

	if (fs_validateFile(filePath)) { //If the path is invalid
		log_error(logger, "Path invalido - No se puede eliminar archivo");
		return EXIT_FAILURE;
	}

	FILE *filePointer = fopen(filePath, "r+");
	t_FileMetadata fileMetadata = fs_getMetadataFromFile(filePointer);

	//Borrar bloques
	int iterator = 0;
	int numberOfBlocks = (fileMetadata.size + (myFS.metadata.blockSize - 1))
			/ myFS.metadata.blockSize;
	while (iterator < numberOfBlocks) {
		fs_deleteBlockFile(fileMetadata.blocks[iterator]);
		bitarray_clean_bit(myFS.bitmap, fileMetadata.blocks[iterator]);
		iterator++;
	}

	fclose(filePointer);

	//Borrar metadata
	free(fileMetadata.blocks);
	remove(filePath);
	return EXIT_SUCCESS;
}
void fs_dump() {
	printf("FS Metadata File Name: %s\n", myFS.FSMetadataFileName);
	printf("FS Metadata Directory Path: %s\n", myFS.MetadataDirectoryPath);
	printf("FS bitmap file descriptor: %d\n", myFS.bitmapFileDescriptor);
	printf("FS Bitmap File Name: %s\n", myFS.bitmapFileName);
	printf("FS data directory path: %s\n", myFS.dataDirectoryPath);
	printf("FS Files directory path: %s\n", myFS.filesDirectoryPath);
	printf("FS block amount: %d\n", myFS.metadata.blockAmount);
	printf("FS block size: %d\n", myFS.metadata.blockSize);
	printf("FS magic number: %s\n", myFS.metadata.magicNumber);
	printf("FS Mount path: %s\n", myFS.mountDirectoryPath);
	printf("BITMAP\n");

	int i = 0;
	int unBit = 0;

	while (i < myFS.metadata.blockAmount) {
		unBit = bitarray_test_bit(myFS.bitmap, i);
		printf("%d", unBit);
		i++;
	}
	printf("\n");

	fflush(stdout);

}
int fs_writeFile(char * filePath, uint32_t offset, uint32_t size, void * buffer) {

	if (fs_validateFile(filePath)) { //If the path is invalid
		log_error(logger, "Invalid path - Can not write file");
		return -1;
	}

	FILE *metadataFilePointer = fopen(filePath, "r+");
	t_FileMetadata fileMetadata = fs_getMetadataFromFile(metadataFilePointer);

	int sparseBlocks = offset / myFS.metadata.blockSize;  //Block Index
	int amountOfBlocksOfContent = size / myFS.metadata.blockSize;

	int carriedFloatingPoint = (offset % myFS.metadata.blockSize)
			+ (size % myFS.metadata.blockSize);
	int amountOfBlocksToWrite = 1 + sparseBlocks + amountOfBlocksOfContent;
	if (carriedFloatingPoint > myFS.metadata.blockSize)
		amountOfBlocksToWrite++; //Si se paso apenas de un bloque ya lo esta ocupando

	int fileBlockCount = (fileMetadata.size / myFS.metadata.blockSize) + 1;

	int newBlocksNeeded = amountOfBlocksToWrite - fileBlockCount;

	if (newBlocksNeeded > fs_getAmountOfFreeBlocks())
		return -1;

	int blockToWrite;
	t_FileMetadata newFileMetadata;
	newFileMetadata.blocks = malloc(sizeof(int) * amountOfBlocksToWrite);
	memcpy(newFileMetadata.blocks, fileMetadata.blocks,
			sizeof(int) * fileBlockCount);

	int iterator = 0;
	// check for the first block, if im being asked to write further than the blocks I have
	if (sparseBlocks > fileBlockCount - 1) {
		while (iterator < sparseBlocks) {
			newFileMetadata.blocks[fileBlockCount] = fs_createBlockFile(
					fs_getFirstFreeBlock(&myFS));
			blockToWrite = newFileMetadata.blocks[fileBlockCount];
			fileBlockCount++;
			iterator++;
		}
	} else {
		blockToWrite = fileMetadata.blocks[sparseBlocks];
	}
	int positionInBlockToWrite = offset % myFS.metadata.blockSize;

	int remainingSpaceInCurrentBlock = myFS.metadata.blockSize
			- positionInBlockToWrite;
	int sizeRemainingToWrite = size;

	int temporalBufferSize;
	FILE * blockFilePointer;

	int amountWritten = 0;
	int blocksWritten = 0;
	char *temporalBuffer = NULL;
	while (sizeRemainingToWrite > 0) { //While theres stuff to write

		blockFilePointer = fs_openBlockFile(blockToWrite);

		fseek(blockFilePointer, positionInBlockToWrite, SEEK_SET);

		// DRY DONT REPEAT YOURSELF
		if (sizeRemainingToWrite > remainingSpaceInCurrentBlock) {
			temporalBufferSize = remainingSpaceInCurrentBlock;
		} else {
			temporalBufferSize = sizeRemainingToWrite;
		}

		temporalBuffer = malloc(temporalBufferSize + 1);
		memset(temporalBuffer, 0, temporalBufferSize + 1);
		memcpy(temporalBuffer, buffer + amountWritten, temporalBufferSize);
		amountWritten += temporalBufferSize;

		fputs(temporalBuffer, blockFilePointer);
		sizeRemainingToWrite -= amountWritten;

		free(temporalBuffer);
		fclose(blockFilePointer);

		blocksWritten++;
		if (fileBlockCount < amountOfBlocksToWrite
				&& (blockToWrite == newFileMetadata.blocks[fileBlockCount - 1])) {
			//need to create a new block
			newFileMetadata.blocks[fileBlockCount] = fs_createBlockFile(
					fs_getFirstFreeBlock(&myFS));
			blockToWrite = newFileMetadata.blocks[fileBlockCount];
			remainingSpaceInCurrentBlock = myFS.metadata.blockSize;
			positionInBlockToWrite = 0;
			fileBlockCount++;
		} else {
			blockToWrite = newFileMetadata.blocks[blocksWritten];
			remainingSpaceInCurrentBlock = myFS.metadata.blockSize;
			positionInBlockToWrite = 0;
		}

	}

	FILE *lastBlockFilePointer = fs_openBlockFile(
			newFileMetadata.blocks[fileBlockCount - 1]);
	struct stat lastBlockFileStats;
	fstat(lastBlockFilePointer->_fileno, &lastBlockFileStats);

	int updatedFileSize = lastBlockFileStats.st_size
			+ ((fileBlockCount - 1) * myFS.metadata.blockSize);

	newFileMetadata.size = updatedFileSize;

	fs_updateFileMetadata(metadataFilePointer, newFileMetadata);


	free(fileMetadata.blocks); //anda
	free(newFileMetadata.blocks);

	return EXIT_SUCCESS;
}
FILE* fs_openBlockFile(int blockNumber) {
	char *fileName = fs_getBlockFilePath(blockNumber);
	FILE *filePointer = fopen(fileName, "r+");
	if (filePointer != NULL)
		free(fileName);
	return filePointer;
}
int fs_updateFileMetadata(FILE *filePointer, t_FileMetadata newMetadata) {

	char lineBuffer[255];
	char *metadataSizeBuffer;
	char *metadataBlocksBuffer;
	int * blocks;
	t_FileMetadata output;

	fseek(filePointer, 0, SEEK_SET);

	//Updates Size information on metadata
	memset(lineBuffer, 0, 255);
	sprintf(lineBuffer, "TAMANIO=%d\n", newMetadata.size);
	fputs(lineBuffer, filePointer);

	memset(lineBuffer, 0, 255);
	int blockCount = (newMetadata.size / myFS.metadata.blockSize) + 1;
	int iterator = 0;
	sprintf(lineBuffer, "BLOQUES=[");

	fputs(lineBuffer, filePointer);
	memset(lineBuffer, 0, 255);

	while (iterator < blockCount) {
		sprintf(lineBuffer, "%d", newMetadata.blocks[iterator]);
		fputs(lineBuffer, filePointer);
		memset(lineBuffer, 0, 255);
		iterator++;
		if (iterator < blockCount) {
			sprintf(lineBuffer, ",");
			fputs(lineBuffer, filePointer);
			memset(lineBuffer, 0, 255);
		}
	}
	memset(lineBuffer, 0, 255);
	sprintf(lineBuffer, "]\n");

	fputs(lineBuffer, filePointer);
	fclose(filePointer);

	return 0;
}
int fs_getAmountOfFreeBlocks() {

	int i = 0;
	int unBit = 0;
	int freeBlocks = 0;

	while (i < myFS.metadata.blockAmount) {

		unBit = bitarray_test_bit(myFS.bitmap, i);

		if (!unBit)
			freeBlocks++;

		i++;
	}

	return freeBlocks;

}
char *fs_readBlockFile(int blockNumberToRead, uint32_t offset, uint32_t size) {

	char *fileName = fs_getBlockFilePath(blockNumberToRead);

	FILE * blockFilePointer = fs_openBlockFile(blockNumberToRead);
	int sizeToRead = size;
	int error = 0;
	char * readValues = malloc(size);

	memset(readValues, 0, sizeToRead);

	if (fs_validateFile(fileName)) { //If the path is invalid
		log_error(logger, "Invalid path - Can not write file");
		return 'NULL';
	}

	free(fileName);
	error = fseek(blockFilePointer, offset, SEEK_SET);

	if (strlen(blockFilePointer) == 0) {
		log_error(logger,
				"Error while seeking offset on READ FILE operation (No bytes read - possible sparse file)");
		return NULL;
	}

	fflush(stdin);
	//error = fread(readValues, sizeof(char),size, blockFilePointer);
	

	error = fgets(readValues, size, blockFilePointer);

	if (error == NULL) {
		log_error(logger,
				"Error while reading on READ FILE operation (sparse)");
		return NULL;
	}

	printf("Reading %d bytes with %d offset from block %d : %s \n",
			size, offset, blockNumberToRead, readValues);

	fclose(blockFilePointer);

	return readValues;

}
char *fs_getBlockFilePath(int blockNumber) {

	int fileNameLength = 0;
	char * filePath;

	fileNameLength += strlen(myFS.dataDirectoryPath)
			+ fs_getNumberOfDigits(blockNumber) + strlen(".bin") + 1;
	filePath = malloc(fileNameLength);

	sprintf(filePath, "%s%d.bin", myFS.dataDirectoryPath, blockNumber);
	return filePath;

}
char *fs_readFile(char * filePath, uint32_t offset, uint32_t size) {

	if (fs_validateFile(filePath)) { //If the path is invalid
		log_error(logger, "Invalid path - Can not write file");
		return -1;
	}

	char *readValues = malloc(size);
	memset(readValues, 0, size);

	FILE *filePointer = fopen(filePath, "r+");
	t_FileMetadata fileMetadata = fs_getMetadataFromFile(filePointer);

	int amountOfBlocksInMetadata = fileMetadata.size / myFS.metadata.blockSize;
	int floatingPoint = fileMetadata.size % myFS.metadata.blockSize;
	if (floatingPoint > 0)
		amountOfBlocksInMetadata++;
	int currentBlockIndex = offset / myFS.metadata.blockSize;
	int finalBlockToRead = 1 + (offset + size) / myFS.metadata.blockSize;

	int totalBytesUntilEndOfFile = fileMetadata.size - offset;
	if (currentBlockIndex > amountOfBlocksInMetadata
			|| size > totalBytesUntilEndOfFile) {
		log_error(logger, "Invalid block - could not read program %s",
				filePath);
		return -1;
	}

	int amountOfBlocksToRead = size / myFS.metadata.blockSize;
	int readOffset = offset % myFS.metadata.blockSize;
	int bytesUntilEndOfBlock = myFS.metadata.blockSize - readOffset;
	int bytesRemainingToRead = size;
	int currentBlock = fileMetadata.blocks[currentBlockIndex];

	char *buffer;
	int offsetBuffer = 0;

	while (bytesRemainingToRead <= size) { //Mientras todavia quede por leer

		if (bytesRemainingToRead > bytesUntilEndOfBlock) { //Si tiene que leer mas de un bloque
			buffer = fs_readBlockFile(currentBlock, readOffset,
					bytesUntilEndOfBlock); //Lee hasta fin de bloque
			if (buffer == NULL)
				return NULL;

			memcpy(readValues + offsetBuffer, buffer, bytesUntilEndOfBlock);
			bytesRemainingToRead -= bytesUntilEndOfBlock; //Resta los bytes que leyo
			// jump to next block
			currentBlockIndex++;
			currentBlock = fileMetadata.blocks[currentBlockIndex];
			offsetBuffer += size - bytesRemainingToRead;
			bytesUntilEndOfBlock = myFS.metadata.blockSize;
			readOffset = 0;

		} else { //Si tiene que terminar de leer los bytes restantes en el currentblock
			buffer = fs_readBlockFile(fileMetadata.blocks[currentBlockIndex],
					readOffset, bytesRemainingToRead);
			if (buffer == NULL)
				return NULL;

			memcpy(readValues + offsetBuffer, buffer, bytesRemainingToRead);
			char *lastChar = readValues + offsetBuffer + bytesRemainingToRead;
			lastChar[0] = '\0';
			bytesRemainingToRead = 0;
			offsetBuffer += size - bytesRemainingToRead;
			break;

		}
		memset(buffer, 0, sizeof(buffer));
		free(buffer);

	}

	printf("READ %d BYTES WITH %d OFFSET FROM FILE [%s]: %s\n",
			strlen(readValues), offset, filePath, readValues);

	free(buffer);
	free(fileMetadata.blocks);
	return readValues;

}
int fs_fileContainsBlockNumber(char *filePath, int blockNumber) { //No se usa por ahora - dice si un bloque esta contenido por un archivo

	FILE *filePointer = fopen(filePath, "r+");
	t_FileMetadata fileMetadata = fs_getMetadataFromFile(filePointer);

	int amountOfBlocksInMetadata = 1
			+ fileMetadata.size / myFS.metadata.blockSize;

	int iterator = 0;

	while (iterator < amountOfBlocksInMetadata) {

		if (fileMetadata.blocks[iterator] == blockNumber) {
			return EXIT_SUCCESS;
		}

		iterator++;
	}

	return EXIT_FAILURE;

}
void kernelServerSocket_handleNewConnection(int fd) {
	log_info(logger, "New connection. fd: %d", fd);
	kernelFileDescriptor = fd;
}
void kernelServerSocket_handleDisconnection(int fd) {
	log_info(logger, "New disconnection. fd: %d", fd);

}
void kernelServerSocket_handleDeserializedStruct(int fd,
		ipc_operationIdentifier operationId, void *buffer) {
	switch (operationId) {
	case HANDSHAKE: {
		ipc_struct_handshake *handshake = buffer;
		log_info(logger, "Handshake received. Process identifier: %s",
				processName(handshake->processIdentifier));
		ipc_server_sendHandshakeResponse(fd, 1, 0);
		break;
	}
	case FILESYSTEM_VALIDATE_FILE: {
		puts("FILESYSTEM_VALIDATE_FILE");
		ipc_struct_fileSystem_validate_file *request = buffer;
		ipc_struct_fileSystem_validate_file_response response;
		response.header.operationIdentifier = FILESYSTEM_VALIDATE_FILE_RESPONSE;

		response.status = fs_validateFile(fs_getFullPathFromFileName(request->path));

		send(kernelFileDescriptor,&response,sizeof(ipc_struct_fileSystem_validate_file_response),0);

		break;
	}
	case FILESYSTEM_CREATE_FILE: {
		puts("FILESYSTEM_CREATE_FILE");
		ipc_struct_fileSystem_create_file *request = buffer;
		ipc_struct_fileSystem_create_file_response response;
		response.header.operationIdentifier = FILESYSTEM_CREATE_FILE_RESPONSE;


		fs_createFile(fs_getFullPathFromFileName(request->path));

		response.status = EXIT_SUCCESS;

		send(kernelFileDescriptor,&response,sizeof(ipc_struct_fileSystem_create_file_response),0);

		break;
	}
	case FILESYSTEM_DELETE_FILE: {
		puts("FILESYSTEM_DELETE_FILE");
		break;
	}
	case FILESYSTEM_READ_FILE: {
		puts("FILESYSTEM_READ_FILE");
		ipc_struct_fileSystem_read_file *request = buffer;
		ipc_struct_fileSystem_read_file_response response;
		response.header.operationIdentifier = FILESYSTEM_READ_FILE_RESPONSE;

		response.buffer = fs_readFile(fs_getFullPathFromFileName(request->path),request->offset,request->size);
		response.bufferSize = request->size;

		int bufferSize = sizeof(ipc_header) + sizeof(int) + request->size;
		int bufferOffset = 0;
		char *buffer = malloc(bufferSize);
		memset(buffer,0,bufferSize);

		memcpy(buffer+bufferOffset,&response.header,sizeof(ipc_header));
		bufferOffset += sizeof(ipc_header);

		memcpy(buffer+bufferOffset,&response.bufferSize,sizeof(int));
		bufferOffset += sizeof(int);

		memcpy(buffer+bufferOffset,response.buffer,request->size);
		bufferOffset += request->size;

		send(kernelFileDescriptor,buffer,bufferSize,0);

		break;
	}
	case FILESYSTEM_WRITE_FILE: {
		puts("FILESYSTEM_WRITE_FILE");
		ipc_struct_fileSystem_write_file *request = buffer;
		ipc_struct_fileSystem_write_file_response response;
		response.header.operationIdentifier = FILESYSTEM_WRITE_FILE_RESPONSE;

		response.bytesWritten = fs_writeFile(fs_getFullPathFromFileName(request->path),request->offset,request->size,request->buffer);
		send(kernelFileDescriptor,&response,sizeof(ipc_struct_fileSystem_write_file_response),0);

		break;
	}
	default:
		break;
	}
}
char *fs_getFullPathFromFileName(char *file){


	int fileDirectoryPathLength = strlen(myFS.filesDirectoryPath);
	int fileNameLength = strlen(file);

	int fullPathLength = fileDirectoryPathLength + fileNameLength;

	char *fullPath = malloc(fullPathLength);

	memset(fullPath, 0, fullPathLength);

	int offset = 0;

	memcpy(fullPath+offset, myFS.filesDirectoryPath, fileDirectoryPathLength);

	offset += fileDirectoryPathLength;

	memcpy(fullPath+offset-1, file, fileNameLength);

	return fullPath;

}
int fs_createSubDirectoriesFromFilePath(char *filePath){

	int iterator = strlen(filePath);

	char *buffer;

	while(iterator > 0){

		if(filePath[iterator] == '/'){
			buffer = malloc(sizeof(char)*iterator+1);
			memset(buffer,0, sizeof(char)*iterator+1);
			memcpy(buffer,filePath,sizeof(char)*iterator);
			fs_createPreviousFolders(buffer);
			free(buffer); //Anda
			return EXIT_SUCCESS;
		}

		iterator--;

	}

	return EXIT_FAILURE;

	///mnt/SADICA_FS/Archivos/prueba1.bin


}
void fs_createPreviousFolders(const char *dir) {
        char tmp[256];
        char *p = NULL;
        size_t len;

        snprintf(tmp, sizeof(tmp),"%s",dir);
        len = strlen(tmp);
        if(tmp[len - 1] == '/')
                tmp[len - 1] = 0;
        for(p = tmp + 1; *p; p++)
                if(*p == '/') {
                        *p = 0;
                        mkdir(tmp, S_IRWXU);
                        *p = '/';
                }
        mkdir(tmp, S_IRWXU);
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
//	char *read = fs_readFile("/mnt/SADICA_FS/Archivos/largeFile.bin", 0, 65);
//		puts(read);
	ipc_createServer("5004",kernelServerSocket_handleNewConnection,kernelServerSocket_handleDisconnection,kernelServerSocket_handleDeserializedStruct);


//	fs_createSubDirectoriesFromFilePath("/mnt/SADICA_FS/Archivos/test/prueba1.bin");
//	fs_createFile("/mnt/SADICA_FS/Archivos/test/prueba1.bin");
//	fs_createFile("/mnt/SADICA_FS/Archivos/test/prueba2.bin");
//
//	fs_validateFile("/prueba1.bin");
//
//
//char *bafer = string_new();
//string_append(&bafer,"Lorem ipsum dolor sit amet, consectetur adipiscing elit. Nunc mi mauris, suscipit euismod leo vitae, tempor sagittis elit nullam.");
//	fs_writeFile("/mnt/SADICA_FS/Archivos/test/prueba1.bin",0,strlen(bafer),bafer);
//	char *read = fs_readFile("/mnt/SADICA_FS/Archivos/test/prueba1.bin", 0, 128);
//	puts(read);
//	free(read);
//	read = fs_readFile("/mnt/SADICA_FS/Archivos/test/prueba1.bin", 60, 68);
//	free(read);
//    fs_removeFile("/mnt/SADICA_FS/Archivos/test/prueba1.bin");


	return EXIT_SUCCESS;

}
