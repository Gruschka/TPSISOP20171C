/*
 ============================================================================
 Name        : memoria.c
 Author      : Federico Trimboli
 Version     :
 Copyright   : Copyright
 Description : Insanely great memory.
 ============================================================================
 */

// TODO: dar un máximo de cache para cada proceso
// TODO: integración con IPC
// TODO: consola de la memoria

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <limits.h>
#include <time.h>
#include <commons/config.h>
#include <commons/log.h>

#include <sys/socket.h>
#include <arpa/inet.h>
#include <pthread.h>

typedef unsigned char mem_bool;
static u_int32_t k_connectionPort;
static u_int32_t k_numberOfPages;

void millisleep(u_int32_t milliseconds) {
	struct timespec ts;
	ts.tv_sec = milliseconds / 1000;
	ts.tv_nsec = (milliseconds % 1000) * 1000000;
	nanosleep(&ts, NULL);
}

//////// Fin de código de memoria física

typedef struct mem_page_entry {
    int32_t processID;
    int32_t processPageNumber;
} mem_page_entry;

static u_int32_t k_numberOfFrames;
static u_int32_t k_frameSize;
static void *physicalMemory;
static u_int32_t k_physicalMemoryAccessDelay;

int64_t calculatePair(int32_t k1, int32_t k2) {
	return ((int64_t)k1 << 32) + (int64_t)k2;
}

int32_t calculateHash(int32_t processID, int32_t processPageNumber) {
	return calculatePair(processID, processPageNumber) % k_numberOfFrames;
}

mem_page_entry *getPageEntryPointerForIndex(int index) {
	return physicalMemory + index * sizeof(mem_page_entry);
}

void *getFramePointerForPageIndex(int index) {
	return physicalMemory + ((k_numberOfFrames - k_numberOfPages + index) * k_frameSize);
}

int findAvailablePageIndex(int32_t processID, int32_t processPageNumber) {
	int hash = calculateHash(processID, processPageNumber);

	mem_page_entry *availableEntry = NULL;
	int i;
	for (i = hash; i < k_numberOfPages; i++) {
		mem_page_entry *possiblyAvailableEntry = getPageEntryPointerForIndex(i);
		if (possiblyAvailableEntry->processID == -1) {
			availableEntry = possiblyAvailableEntry;
			break;
		}
	}

	if (availableEntry != NULL) {
		return i;
	}

	for (i = hash - 1; i >= 0; i--) {
		mem_page_entry *possiblyAvailableEntry = getPageEntryPointerForIndex(i);
			if (possiblyAvailableEntry->processID == -1) {
				availableEntry = possiblyAvailableEntry;
				break;
			}
		}

	if (availableEntry != NULL) {
		return i;
	}

	return -1;
}

int findPageIndex(int32_t processID, int32_t processPageNumber) {
	int32_t hash = calculateHash(processID, processPageNumber);
	mem_page_entry *entry = NULL;

	int32_t i;
	for (i = hash; i < k_numberOfPages; i++) {
		mem_page_entry *possibleEntry = getPageEntryPointerForIndex(i);
		if (possibleEntry->processID == processID && possibleEntry->processPageNumber == processPageNumber) {
			entry = possibleEntry;
			break;
		}
	}

	if (entry != NULL) {
		return i;
	}

	for (i = hash - 1; i >= 0; i--) {
		mem_page_entry *possibleEntry = getPageEntryPointerForIndex(i);
		if (possibleEntry->processID == processID && possibleEntry->processPageNumber == processPageNumber) {
			entry = possibleEntry;
			break;
		}
	}

	if (entry != NULL) {
		return i;
	}

	return -1;
}

mem_bool areOffsetAndSizeValid(int32_t offset, int32_t size) {
	if (size == 0) { return 0; }
	return (offset + size) <= k_frameSize;
}

int isProcessAlreadyInitialized(int32_t processID) {
	int i;
	for (i = 0; i < k_numberOfPages; i++) {
		mem_page_entry *entry = getPageEntryPointerForIndex(i);
		if (entry->processID == processID) {
			return 1;
		}
	}
	return 0;
}

int hasMemoryAvailablePages(int32_t numberOfPages) {
	int availablePages = 0;

	int i;
	for (i = 0; i < k_numberOfPages; i++) {
		mem_page_entry *entry = getPageEntryPointerForIndex(i);
		if (entry->processID == -1) {
			availablePages += 1;
			if (availablePages == numberOfPages) {
				return 1;
			}
		}
	}
	return 0;
}

int assignPageToProcess(int32_t processID, int32_t processPageNumber) {
	int pageIndex = findPageIndex(processID, processPageNumber);
	if (pageIndex != -1) { return 0; }

	pageIndex = findAvailablePageIndex(processID, processPageNumber);
	if (pageIndex == -1) { return 0; }
	mem_page_entry *entry = getPageEntryPointerForIndex(pageIndex);
	entry->processID = processID;
	entry->processPageNumber = processPageNumber;
	return 1;
}

int numberOfPagesOwnedByProcess(int32_t processID) {
	int numberOfPagesOwned = 0;

	int i = 0;
	for (i = 0; i < k_numberOfPages; i++) {
			mem_page_entry *entry = getPageEntryPointerForIndex(i);
			if (entry->processID == processID) {
				numberOfPagesOwned++;
			}
		}

	return numberOfPagesOwned;
}

//////// Fin de código de memoria física

//////// Comienzo de código de cache

typedef struct mem_cached_page_entry {
	int32_t processID;
	int32_t processPageNumber;
	int32_t lruCounter;
	void *pageContentPointer;
} mem_cached_page_entry;

static u_int32_t k_numberOfEntriesInCache;
static u_int32_t k_maxPagesForEachProcessInCache;
static void *cache;
static u_int32_t cache_currentCacheLRUCounter = 0;

mem_cached_page_entry *cache_getEntryPointerForIndex(int index) {
	return cache + index * (sizeof(mem_cached_page_entry) + k_frameSize);
}

mem_cached_page_entry *cache_getEntryPointer(int32_t processID, int32_t processPageNumber) {
	int i;
	for (i = 0; i < k_numberOfEntriesInCache; i++) {
		mem_cached_page_entry *entry = cache_getEntryPointerForIndex(i);
		if (entry->processID == processID && entry->processPageNumber == processPageNumber) {
			return entry;
		}
	}

	return NULL;
}

mem_cached_page_entry *cache_getEntryPointerOrLRUEntryPointer(int32_t processID, int32_t processPageNumber) {
	mem_cached_page_entry *existingEntry = cache_getEntryPointer(processID, processPageNumber);
	if (existingEntry != NULL) { return existingEntry; }

	mem_cached_page_entry *lruEntry = cache_getEntryPointerForIndex(0);
	int i;
	for (i = 1; i < k_numberOfEntriesInCache; i++) {
		mem_cached_page_entry *entry = cache_getEntryPointerForIndex(i);
		if (entry->lruCounter < lruEntry->lruCounter) {
			lruEntry = entry;
		}
	}
	return lruEntry;
}

void cache_write(int32_t processID, int32_t processPageNumber, void *pageContentPointer) {
	mem_cached_page_entry *entry = cache_getEntryPointerOrLRUEntryPointer(processID, processPageNumber);
	entry->processID = processID;
	entry->processPageNumber = processPageNumber;
	entry->lruCounter = cache_currentCacheLRUCounter + 1;
	cache_currentCacheLRUCounter = entry->lruCounter;
	memcpy(entry->pageContentPointer, pageContentPointer, k_frameSize);
}

void *cache_read(int32_t processID, int32_t processPageNumber) {
	mem_cached_page_entry *entry = cache_getEntryPointer(processID, processPageNumber);
	if (entry == NULL) { return NULL; }
	entry->lruCounter = cache_currentCacheLRUCounter + 1;
	cache_currentCacheLRUCounter = entry->lruCounter;
	void *buffer = malloc(k_frameSize);
	memcpy(buffer, entry->pageContentPointer, k_frameSize);
	return buffer;
}

//////// Fin de código de cache

//////// Interfaz pública

int mem_initProcess(int32_t processID, int32_t numberOfPages) {
	if (isProcessAlreadyInitialized(processID)) {
		return 0;
	}

	if (!hasMemoryAvailablePages(numberOfPages)) {
		return 0;
	}

	int i;
	for (i = 0; i < numberOfPages; i++) {
		assignPageToProcess(processID, i);
	}

	return 1;
}

void *mem_read(int32_t processID, int32_t processPageNumber, int32_t offset, int32_t size) {
	if (!isProcessAlreadyInitialized(processID)) {
		return NULL;
	}

	if (!areOffsetAndSizeValid(offset, size)) {
		return NULL;
	}

	// Primero revisamos si podemos encontrar
	// los datos en la cache
	void *cache = cache_read(processID, processPageNumber);
	if (cache != NULL) {
		void *buffer = malloc(size);
		memcpy(buffer, cache, size);
		return buffer;
	}

	// Si no estaban en la cache, los vamos a buscar a
	// la memoria física
	int pageIndex = findPageIndex(processID, processPageNumber);
	if (pageIndex == -1) { return NULL; }

	void *frame = getFramePointerForPageIndex(pageIndex);

	// Guardamos el frame en la cache
	cache_write(processID, processPageNumber, frame);

	// Copiamos los bytes pedidos en un buffer y lo devolvemos
	void *pointer = frame + offset;
	void *buffer = malloc(size);
	memcpy(buffer, pointer, size);
	millisleep(k_physicalMemoryAccessDelay);
	return buffer;
}

int mem_write(int32_t processID, int32_t processPageNumber, int32_t offset, int32_t size, void *buffer) {
	if (!isProcessAlreadyInitialized(processID)) {
		return 0;
	}

	if (!areOffsetAndSizeValid(offset, size)) {
		return 0;
	}

	// Si la página no existe, devolvemos error.
	int pageIndex = findPageIndex(processID, processPageNumber);
	if (pageIndex == -1) { return 0; }
	void *frame = getFramePointerForPageIndex(pageIndex);

	// Si existe, escribimos los datos
	void *pointer = frame + offset;
	memcpy(pointer, buffer, size);
	millisleep(k_physicalMemoryAccessDelay);

	// Luego guardamos el frame en la cache
	cache_write(processID, processPageNumber, frame);

	return 1;
}

int mem_addPagesToProcess(int32_t processID, int32_t numberOfPages) {
	if (!isProcessAlreadyInitialized(processID)) {
		return 0;
	}

	if (!hasMemoryAvailablePages(numberOfPages)) {
		return 0;
	}

	int pagesOriginallyOwned = numberOfPagesOwnedByProcess(processID);

	int i;
	for (i = 0; i < numberOfPages; i++) {
		assignPageToProcess(processID, pagesOriginallyOwned + i);
	}

	return 1;
}

void mem_deinitProcess(int32_t processID) {
	// Primero destruimos las entradas de las páginas
	// de la memoria física
	int i = 0;
	for (i = 0; i < k_numberOfPages; i++) {
		mem_page_entry *entry = getPageEntryPointerForIndex(i);
		if (entry->processID == processID) {
			entry->processID = -1;
			entry->processPageNumber = -1;
		}
	}

	// Luego destruimos las entradas de las páginas
	// de la cache
	for (i = 0; i < k_numberOfEntriesInCache; i++) {
		mem_cached_page_entry *entry = cache_getEntryPointerForIndex(i);
		if (entry->processID == processID) {
			entry->processID = -1;
			entry->processPageNumber = -1;
			entry->lruCounter = 0;
		}
	}
}

//////// Fin de interfaz pública

//////// Dumps

void dump_cache() {
	char *logPath = "./src/cache_dump.txt";
	t_log *log = log_create(logPath, "memoria", 1, LOG_LEVEL_INFO);
	// TODO: dumpear cache
}

//////// Fin de dumps

//////// Comienzo de consola

void menu_configurePhysicalMemoryDelay() {
	int newDelay = 0;
	printf("Introduzca el nuevo retardo en milisegundos (0 para cancelar): \n> ");
	scanf("%d", &newDelay);
	if (newDelay == 0) {
		printf("No se ha modificado el retardo de la memoria.\n\n");
		return;
	}

	k_physicalMemoryAccessDelay = newDelay;
	printf("El nuevo retardo de la memoria es %dms\n\n", newDelay);
}

void menu_dump() {
	int optionIndex = 0;
	do {
		printf("¿Qué desea dumpear? (0 para cancelar): \n1. Memoria cache\n2. Tabla de páginas y listado de procesos activos\n3. Contenido de la memoria\n> ");
		scanf("%d", &optionIndex);
		switch (optionIndex) {
		case 0: printf("No se dumpeó nada.\n\n"); break;
		case 1: dump_cache(); break;
		case 2: printf("TODO: dumpear estructuras administrativas.\n\n"); break;
		case 3: printf("TODO: dumpear contenido de la memoria.\n\n"); break;
		default: printf("Opción inválida, vuelva a intentar.\n\n"); break;
		}
	} while (optionIndex > 3);
}

void menu_flush() {
	printf("TODO: flushear cache.\n\n");
}

void menu_size() {
	int optionIndex = 0;
		do {
			printf("¿El tamaño de qué desea obtener? (0 para cancelar): \n1. Memoria física completa\n2. Un proceso en particular\n> ");
			scanf("%d", &optionIndex);
			switch (optionIndex) {
			case 0: printf("Se canceló la consulta de tamaño.\n\n"); break;
			case 1: printf("TODO: imprimir tamaño de la memoria (frames libres y frames ocupados).\n\n"); break;
			case 2: printf("TODO: pedir processID y dar el tamaño que ocupa.\n\n"); break;
			default: printf("Opción inválida, vuelva a intentar.\n\n"); break;
			}
		} while (optionIndex > 2);
}

//////// Fin de consola

int main(int argc, char **argv) {
	// Configuración
	char *configPath = (argc > 1) ? argv[1] : "./src/config.txt";
	printf("Levantando configuración del archivo '%s'.\n", configPath);
	t_config *config = config_create(configPath);
	k_connectionPort = config_get_int_value(config, "PUERTO");
	k_numberOfFrames = config_get_int_value(config, "MARCOS");
	k_frameSize = config_get_int_value(config, "MARCO_SIZE");
	k_numberOfEntriesInCache = config_get_int_value(config, "ENTRADAS_CACHE");
	k_maxPagesForEachProcessInCache = config_get_int_value(config, "CACHE_X_PROC");
	k_physicalMemoryAccessDelay = config_get_int_value(config, "RETARDO_MEMORIA");

	// Physical memory initialization
	printf("Inicializando memoria física.\n");
	int totalNumberOfBytes = k_numberOfFrames * k_frameSize;
	physicalMemory = malloc(totalNumberOfBytes);
	k_numberOfPages = floor(totalNumberOfBytes / (k_frameSize + sizeof(mem_page_entry)));

	int i;
	for (i = 0; i < k_numberOfPages; i++) {
		mem_page_entry *entry = getPageEntryPointerForIndex(i);
		entry->processID = -1;
		entry->processPageNumber = -1;
	}

	// Cache memory initialization
	printf("Inicializando memoria cache.\n");
	cache = malloc(k_numberOfEntriesInCache * (sizeof(mem_cached_page_entry) + k_frameSize));
	for (i = 0; i < k_numberOfEntriesInCache; i++) {
		mem_cached_page_entry *entry = cache_getEntryPointerForIndex(i);
		entry->processID = -1;
		entry->processPageNumber = -1;
		entry->lruCounter = 0;
		entry->pageContentPointer = (void *)entry + sizeof(mem_cached_page_entry);
	}

//	int a = mem_initProcess(0, 10);
//	int b = mem_initProcess(1, 10);
//	int c = mem_initProcess(2, 10);
//	int d = mem_initProcess(0, 1);
//
//	char *texto = "esto es una prueba capo";
//	int a1 = mem_write(0, 0, 0, 24, texto);
//	int a2 = mem_write(0, 9, 0, 24, texto);
//	int a3 = mem_write(1, 2, 0, 24, texto);
//	int a4 = mem_write(0, 10, 0, 24, texto);
//
//	char *meTraje1 = mem_read(0, 0, 0, 24);
//	char *meTraje2 = mem_read(0, 9, 0, 24);
//	char *meTraje3 = mem_read(1, 2, 0, 24);
//	char *meTraje4 = mem_read(0, 10, 0, 24);
//
//	int b1 = mem_addPagesToProcess(0, 1);
//	int b2 = mem_write(0, 10, 0, 24, texto);
//	char *b3 = mem_read(0, 10, 0, 24);
//	char *b4 = mem_read(0, 11, 0, 24);
//
//	mem_deinitProcess(0);
//
//	char *c1 = mem_read(0, 0, 0, 24);
//	char *c2 = mem_read(1, 2, 0, 24);

	printf("Todo configurado y funcionando.\n\n");

	void *server_main(void *args);
	pthread_t serverThread;
	if(pthread_create(&serverThread, NULL, server_main, NULL) < 0) {
		perror("could not create thread");
		return 1;
	}

	int optionIndex = 0;
	do {
		printf("Seleccione una opción (0 para salir): \n1. Configurar retardo\n2. dump\n3. flush\n4. size\n> ");
		scanf("%d", &optionIndex);
		switch (optionIndex) {
		case 0: printf("Saliendo.\n\n"); break;
		case 1: menu_configurePhysicalMemoryDelay(); break;
		case 2: menu_dump(); break;
		case 3: menu_flush(); break;
		case 4: menu_size(); break;
		default: printf("Opción inválida, vuelva a intentar.\n\n"); break;
		}
	} while (optionIndex != 0);

	return EXIT_SUCCESS;
}

void *connection_handler(void *);

void *server_main(void *args) {
	int socket_desc, client_sock, c;
	struct sockaddr_in server, client;
	// Create socket
	socket_desc = socket(AF_INET, SOCK_STREAM, 0);
	if(socket_desc == -1) {
		printf("Could not create socket");
	}
	puts("Socket created");
	// Prepare the sockaddr_in structure
	server.sin_family = AF_INET;
	server.sin_addr.s_addr = INADDR_ANY;
	server.sin_port = htons(8888);
	// Bind
	if(bind(socket_desc,(struct sockaddr *)&server, sizeof(server)) < 0) {
		// print the error message
		perror("bind failed. Error");
		return 1;
	}
	puts("bind done");
	// Listen
	listen(socket_desc, 3);
	// Accept and incoming connection
	puts("Waiting for incoming connections...");
	c = sizeof(struct sockaddr_in);
	// Accept and incoming connection
	puts("Waiting for incoming connections...");
	c = sizeof(struct sockaddr_in);
	pthread_t thread_id;
	while((client_sock = accept(socket_desc, (struct sockaddr *)&client, (socklen_t*)&c))) {
		puts("Connection accepted");
		if(pthread_create( &thread_id, NULL,  connection_handler, (void*) &client_sock) < 0) {
			perror("could not create thread");
			return 1;
		}
		// Now join the thread, so that we dont terminate before the thread
		// pthread_join( thread_id, NULL);
		puts("Handler assigned");
	}
	if (client_sock < 0) {
		perror("accept failed");
		return 1;
	}
	return 0;
}

//
// This will handle connection for each client
//
void *connection_handler(void *socket_desc) {
	// Get the socket descriptor
	int sock = *(int*)socket_desc;
	int read_size;
	char *message, client_message[2000];
	// Send some messages to the client
	message = "Greetings! I am your connection handler\n";
	write(sock, message, strlen(message));
	message = "Now type something and i shall repeat what you type \n";
	write(sock, message, strlen(message));
	// Receive a message from client
	while( (read_size = recv(sock, client_message, 2000, 0)) > 0 )  {
		// end of string marker
		client_message[read_size] = '\0';
		// Send the message back to client
		write(sock, client_message, strlen(client_message));
		// clear the message buffer
		memset(client_message, 0, 2000);
	}
	if(read_size == 0) {
		puts("Client disconnected");
		fflush(stdout);
	}
	else if(read_size == -1) {
		perror("recv failed");
	}
	return 0;
}
