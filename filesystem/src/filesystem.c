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


		char *compiledRead;
		//strtok con igual => BLOQUES\0[4,1231,5,1,0,12,543]
		int blockCount = 1 + (output.size / myFS.metadata.blockSize);
		char *stringArray = malloc(255);
		memset(stringArray,0,255);
		int stringArrayOffset = 0;
		int blockNumber;
		int iterator = 0;

		fgets(lineBuffer,strlen("BLOQUES=[")+1,filePointer);
		memset(lineBuffer,0,255);
		char *temporalBuffer;

		int firstLine = 1;

		//Mientras haya para leer del archivo de metadata lo vuelco en el lineBuffer
		while(fgets(lineBuffer,255,filePointer)){

			if(!firstLine){ //Si  es la primera linea linea, no crea backup
				temporalBuffer = malloc(stringArrayOffset);
				memset(temporalBuffer,0,stringArrayOffset);
				memcpy(temporalBuffer,stringArray,stringArrayOffset);
				free(stringArray);
			}

			//String array guarda los bloques sin corchetes, y se va alocando segun lo que tenga linebuffer + leido acumulado
			stringArray = malloc(strlen(lineBuffer) + stringArrayOffset);
			memset(stringArray,0,strlen(lineBuffer) + stringArrayOffset);
			if(!firstLine) memcpy(stringArray,temporalBuffer,stringArrayOffset);
			//Si es la primera linea => strinArrayOffset = 0 (arranca del principio)
			memcpy(stringArray+stringArrayOffset,lineBuffer,strlen(lineBuffer));
			if(!firstLine)free(temporalBuffer);
			stringArrayOffset+=strlen(lineBuffer); //Acumulo lo que acabo de leer
			firstLine = 0;
			memset(lineBuffer,0,255);
		}


		strtok_r(stringArray,"]",&metadataBlocksBuffer);

		output.blocks =fs_getArrayFromString(&stringArray,blockCount);

		if(output.blocks == -1){

			log_error(logger, "Critical error reading blocks");
			return output;
		}

		free(stringArray);

		return output;



}
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
		fclose(newFileDescriptor);
		return EXIT_SUCCESS;
	}
}
int fs_createBlockFile(int blockNumber){
	char *fileName;
	int fileNameLength = 0;

	fileNameLength += strlen(myFS.dataDirectoryPath) + fs_getNumberOfDigits(blockNumber) +strlen(".bin") + 1;
	fileName= malloc(fileNameLength);
	memset(fileName,0,fileNameLength);

	fflush(stdin);
	sprintf(fileName,"%s%d.bin",myFS.dataDirectoryPath,blockNumber);

	FILE *fileDescriptor = fopen(fileName,"w+");
	bitarray_set_bit(myFS.bitmap,blockNumber);

	free(fileName);
	fclose(fileDescriptor);
	return blockNumber;

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

	char *stringPointer = *stringArray;

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
	printf("\n");

	fflush(stdout);





}
int fs_writeFile(char * filePath, uint32_t offset, uint32_t size, void * buffer){

	if(fs_validateFile(filePath)){ //If the path is invalid
			log_error(logger, "Invalid path - Can not write file");
			return -1;
	}


	FILE *metadataFilePointer = fopen(filePath, "r+");
	t_FileMetadata fileMetadata = fs_getMetadataFromFile(metadataFilePointer);

	int sparseBlocks = offset / myFS.metadata.blockSize;
	int amountOfBlocksOfContent = size / myFS.metadata.blockSize;

	int carriedFloatingPoint = (offset % myFS.metadata.blockSize)+(size % myFS.metadata.blockSize);

	int amountOfBlocksToWrite = 1 + sparseBlocks + amountOfBlocksOfContent;

	if(carriedFloatingPoint > myFS.metadata.blockSize) amountOfBlocksToWrite++;

	int fileBlockCount = (fileMetadata.size / myFS.metadata.blockSize) + 1;

	int newBlocksNeeded = amountOfBlocksToWrite - fileBlockCount;

	if(newBlocksNeeded > fs_getAmountOfFreeBlocks()) return -1;

    int blockToWrite;
    t_FileMetadata newFileMetadata;
	newFileMetadata.blocks = malloc(sizeof(int) * amountOfBlocksToWrite);
	memcpy(newFileMetadata.blocks,fileMetadata.blocks,sizeof(int) * fileBlockCount);

	int iterator=0;
    // check for the first block, if im being asked to write further than the blocks I have
    if(sparseBlocks > fileBlockCount-1){
    	while(iterator < sparseBlocks){
			newFileMetadata.blocks[fileBlockCount] = fs_createBlockFile(fs_getFirstFreeBlock(&myFS));
			blockToWrite = newFileMetadata.blocks[fileBlockCount];
			fileBlockCount++;
    		iterator++;
    	}
    }else{
    	blockToWrite = fileMetadata.blocks[sparseBlocks];
    }
	int positionInBlockToWrite = offset % myFS.metadata.blockSize;

	int remainingSpaceInCurrentBlock = myFS.metadata.blockSize - positionInBlockToWrite;
	int sizeRemainingToWrite = size;

	int temporalBufferSize;
	FILE * blockFilePointer;


	int amountWritten = 0;
	int blocksWritten = 0;
	char *temporalBuffer = NULL;
	while(sizeRemainingToWrite > 0){ //While theres stuff to write

		blockFilePointer = fs_openBlockFile(blockToWrite);

		fseek(blockFilePointer, positionInBlockToWrite, SEEK_SET);

		// DRY DONT REPEAT YOURSELF
		if(sizeRemainingToWrite > remainingSpaceInCurrentBlock){
			temporalBufferSize = remainingSpaceInCurrentBlock;
		}else{
			temporalBufferSize = sizeRemainingToWrite;
		}

		temporalBuffer = malloc(temporalBufferSize+1);
		memset(temporalBuffer,0,temporalBufferSize+1);
		memcpy(temporalBuffer,buffer+amountWritten, temporalBufferSize);
		amountWritten += strlen(temporalBuffer);

		fputs(temporalBuffer,blockFilePointer);
		sizeRemainingToWrite -= strlen(temporalBuffer);

		free(temporalBuffer);
		fclose(blockFilePointer);

		blocksWritten++;
		if(fileBlockCount < amountOfBlocksToWrite && (blockToWrite == newFileMetadata.blocks[fileBlockCount-1])){
			//need to create a new block
			newFileMetadata.blocks[fileBlockCount] = fs_createBlockFile(fs_getFirstFreeBlock(&myFS));
			blockToWrite = newFileMetadata.blocks[fileBlockCount];
			remainingSpaceInCurrentBlock = myFS.metadata.blockSize;
			positionInBlockToWrite = 0;
			fileBlockCount++;
		}else{
			blockToWrite = newFileMetadata.blocks[blocksWritten];
			remainingSpaceInCurrentBlock = myFS.metadata.blockSize;
			positionInBlockToWrite = 0;
		}


	}

	FILE *lastBlockFilePointer = fs_openBlockFile(newFileMetadata.blocks[fileBlockCount-1]);
	struct stat lastBlockFileStats;
	fstat(lastBlockFilePointer->_fileno,&lastBlockFileStats);

	int updatedFileSize = lastBlockFileStats.st_size + ((fileBlockCount-1) * myFS.metadata.blockSize);

	newFileMetadata.size = updatedFileSize;

	fs_updateFileMetadata(metadataFilePointer,newFileMetadata);

}
FILE* fs_openBlockFile(int blockNumber){
		char *fileName = fs_getBlockFilePath(blockNumber);
		FILE *filePointer = fopen(fileName,"r+");
		if(filePointer != NULL) free(fileName);
		return filePointer;
}
int fs_updateFileMetadata(FILE *filePointer, t_FileMetadata newMetadata){

	char lineBuffer[255];
	char *metadataSizeBuffer;
	char *metadataBlocksBuffer;
	int * blocks;
	t_FileMetadata output;

	fseek(filePointer,0,SEEK_SET);

	//Updates Size information on metadata
	memset(lineBuffer,0,255);
	sprintf(lineBuffer,"TAMANIO=%d\n",newMetadata.size);
	fputs(lineBuffer,filePointer);

	memset(lineBuffer,0,255);
	int blockCount = (newMetadata.size / myFS.metadata.blockSize) + 1;
	int iterator = 0;
	sprintf(lineBuffer,"BLOQUES=[");

	fputs(lineBuffer, filePointer);
	memset(lineBuffer,0,255);

	while(iterator < blockCount){
		sprintf(lineBuffer,"%d",newMetadata.blocks[iterator]);
		fputs(lineBuffer,filePointer);
		memset(lineBuffer,0,255);
		iterator++;
		if(iterator < blockCount){
			sprintf(lineBuffer,",");
			fputs(lineBuffer,filePointer);
			memset(lineBuffer,0,255);
		}
	}
	memset(lineBuffer,0,255);
	sprintf(lineBuffer,"]\n");

	fputs(lineBuffer,filePointer);
	fclose(filePointer);

	return 0;
}
int fs_getAmountOfFreeBlocks(){

		int i = 0;
		int unBit = 0;
		int freeBlocks = 0;

		while(i < myFS.metadata.blockAmount){

			unBit = bitarray_test_bit(myFS.bitmap,i);

			if(!unBit) freeBlocks++;

			i++;
		}

		return freeBlocks;



}
char *fs_readBlockFile(int blockNumberToRead, uint32_t offset, uint32_t size){

	char *fileName = fs_getBlockFilePath(blockNumberToRead);

	FILE * blockFilePointer = fs_openBlockFile(blockNumberToRead);
	int sizeToRead = sizeof(char) * size;
	int error = 0;
	char * readValues = malloc(sizeToRead);

	memset(readValues,0, sizeToRead);

	if(fs_validateFile(fileName)){ //If the path is invalid
			log_error(logger, "Invalid path - Can not write file");
			return -1;
	}

	error = fseek(blockFilePointer, offset, SEEK_SET);

	if(error == -1){
		log_error(logger, "Error while seeking offset on READ FILE operation");
		return -1;
	}

	fflush(stdin);
	error = fgets(readValues, size, blockFilePointer);

	if(error == -1){
		log_error(logger, "Error while reading on READ FILE operation");
		return -1;
	}

	printf("Reading %d bytes with %d offset from block %d : %s \n", size,offset,blockNumberToRead, readValues);

	fclose(blockFilePointer);

	return readValues;



}
char *fs_getBlockFilePath(int blockNumber){

		int fileNameLength = 0;
		char * filePath;

		fileNameLength += strlen(myFS.dataDirectoryPath) + fs_getNumberOfDigits(blockNumber) +strlen(".bin") + 1;
		filePath = malloc(fileNameLength);

		sprintf(filePath,"%s%d.bin",myFS.dataDirectoryPath,blockNumber);
		return filePath;

}
int fs_readFile(char * filePath, uint32_t offset, uint32_t size){

	if(fs_validateFile(filePath)){ //If the path is invalid
			log_error(logger, "Invalid path - Can not write file");
			return -1;
	}

	FILE *filePointer = fopen(filePath, "r+");
	t_FileMetadata fileMetadata = fs_getMetadataFromFile(filePointer);

	int amountOfBlocksInMetadata = fileMetadata.size / myFS.metadata.blockSize;
	int currentBlockNumber = offset / myFS.metadata.blockSize; //todo: validar que el primer bloque es mio
	int amountOfBlocksToRead = size / myFS.metadata.blockSize; //todo: validar que caigo en un bloque mio
	int readOffset = offset % myFS.metadata.blockSize;
	int bytesUntilEndOfBlock =  myFS.metadata.blockSize - readOffset;
	int bytesRemainingToRead = size;
	int currentBlock = fileMetadata.blocks[currentBlockNumber];
	char *readValues = malloc(size);
	memset(readValues,0,size);
	char *buffer;
	int offsetBuffer = 0;

	while(bytesRemainingToRead <= size){ //Mientras todavia quede por leer

		if(bytesRemainingToRead > bytesUntilEndOfBlock){ //Si tiene que leer mas de un bloque
			buffer = fs_readBlockFile(currentBlock, readOffset, bytesUntilEndOfBlock); //Lee hasta fin de bloque
			memcpy(readValues+offsetBuffer, buffer, bytesUntilEndOfBlock);
			bytesRemainingToRead -= bytesUntilEndOfBlock; //Resta los bytes que leyo
			// jump to next block
			currentBlockNumber++;
			currentBlock = fileMetadata.blocks[currentBlockNumber];
			offsetBuffer += bytesUntilEndOfBlock;
			bytesUntilEndOfBlock = myFS.metadata.blockSize;
			readOffset = 0;

		}else{ //Si tiene que terminar de leer los bytes restantes en el currentblock
			buffer = fs_readBlockFile(fileMetadata.blocks[currentBlockNumber], readOffset, bytesRemainingToRead);
			memcpy(readValues+offsetBuffer, buffer, bytesRemainingToRead);
			bytesRemainingToRead = 0;
			offsetBuffer += bytesRemainingToRead;
			break;

		}
		free(buffer);

	}

	printf("READ: %s\n",readValues);

	return EXIT_SUCCESS;




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


	//fs_createFile("/mnt/SADICA_FS/Archivos/prueba2.bin");
	//fs_createFile("/mnt/SADICA_FS/Archivos/prueba3.bin");
	//fs_createFile("/mnt/SADICA_FS/Archivos/prueba4.bin");


	//char *bafer = string_new();

	//string_append(&bafer, "h");
	//fs_writeFile("/mnt/SADICA_FS/Archivos/prueba1.bin",192,strlen(bafer),bafer);
	//free(bafer);
	//fs_dump();

	char *bafer = string_new();
	string_append(&bafer,"Lorem ipsum dolor sit amet, consectetur adipiscing elit. Nunc mi mauris, suscipit euismod leo vitae, tempor sagittis elit nullam.");


	fs_writeFile("/mnt/SADICA_FS/Archivos/prueba1.bin",0,strlen(bafer),bafer);


	fs_dump();

	fs_readFile("/mnt/SADICA_FS/Archivos/prueba1.bin",0,129);
	/*fs_removeFile("/mnt/SADICA_FS/Archivos/prueba1.bin");
	fs_dump();
	 */
	return EXIT_SUCCESS;
}
