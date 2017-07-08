/*
 ============================================================================
 Name        : memoria.c
 Author      : Federico Trimboli
 Version     :
 Copyright   : Copyright
 Description : Insanely great memory.
 ============================================================================
 */

// TODO: consola: falta dar size de un proceso en particular (¿a qué se refiere?)

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
#include <unistd.h>
#include <pthread.h>
#include <ipc/ipc.h>

t_log *consoleLog;

pthread_rwlock_t physicalMemoryRwlock;
pthread_rwlock_t cacheMemoryRwlock;

typedef unsigned char mem_bool;
static u_int32_t k_connectionPort;
static u_int32_t k_numberOfPages;

void millisleep(u_int32_t milliseconds) {
	struct timespec ts;
	ts.tv_sec = milliseconds / 1000;
	ts.tv_nsec = (milliseconds % 1000) * 1000000;
	nanosleep(&ts, NULL);
}

//////// Comienzo de código de memoria física

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

	int i;
	for (i = 0; i < k_numberOfPages; i++) {
		mem_page_entry *entry = getPageEntryPointerForIndex(i);
		if (entry->processID == processID) {
			numberOfPagesOwned++;
		}
	}

	return numberOfPagesOwned;
}

int *activeProcessesIDs() {
	int *processesIDsPointers = malloc(sizeof(int));
	*processesIDsPointers = -1;

	int i;
	for (i = 0; i < k_numberOfPages; i++) {
		mem_page_entry *entry = getPageEntryPointerForIndex(i);
		if (entry->processID != -1) {
			int j;
			for (j = 0; *(processesIDsPointers + j) != -1; j++) {
				int *processIDPointer = processesIDsPointers + j;
				if (*processIDPointer == entry->processID) break;
			}

			int *processIDPointer = processesIDsPointers + j;
			if (*processIDPointer == -1) {
				processesIDsPointers = realloc(processesIDsPointers, (j + 1 + 1) * sizeof(int));
				*(processesIDsPointers + j) = entry->processID;
				*(processesIDsPointers + j + 1) = -1;
			}
		}
	}

	return processesIDsPointers;
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

	// Averiguamos cuántas entradas ya tiene en cache el proceso
	mem_cached_page_entry *randomEntryForProcess = NULL;
	int numberOfCachedPagesForProcess = 0;
	{
		int i;
		for (i = 0; i < k_numberOfEntriesInCache; i++) {
			mem_cached_page_entry *entry = cache_getEntryPointerForIndex(i);
			if (entry->processID == processID) {
				numberOfCachedPagesForProcess++;
				randomEntryForProcess = entry;
			}
		}
	}

	if (numberOfCachedPagesForProcess >= k_maxPagesForEachProcessInCache) {
		// El proceso ya llegó al máximo, por lo tanto agarramos el LRU
		// dentro del subset de páginas que ya tiene el proceso
		mem_cached_page_entry *lruEntry = randomEntryForProcess;
		int i;
		for (i = 1; i < k_numberOfEntriesInCache; i++) {
			mem_cached_page_entry *entry = cache_getEntryPointerForIndex(i);
			if (entry->processID == processID && entry->lruCounter < lruEntry->lruCounter) {
				lruEntry = entry;
			}
		}
		return lruEntry;
	}

	// Si llegamos a este punto es porque el proceso no excedió su límite
	// de entradas en cache, y se le otorgará la página según LRU global
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
	pthread_rwlock_wrlock(&cacheMemoryRwlock);
	mem_cached_page_entry *entry = cache_getEntryPointerOrLRUEntryPointer(processID, processPageNumber);
	entry->processID = processID;
	entry->processPageNumber = processPageNumber;
	entry->lruCounter = cache_currentCacheLRUCounter + 1;
	cache_currentCacheLRUCounter = entry->lruCounter;
	memcpy(entry->pageContentPointer, pageContentPointer, k_frameSize);
	pthread_rwlock_unlock(&cacheMemoryRwlock);
}

void *cache_read(int32_t processID, int32_t processPageNumber) {
	pthread_rwlock_wrlock(&cacheMemoryRwlock);
	mem_cached_page_entry *entry = cache_getEntryPointer(processID, processPageNumber);
	if (entry == NULL) { pthread_rwlock_unlock(&cacheMemoryRwlock); return NULL; }

	entry->lruCounter = cache_currentCacheLRUCounter + 1;
	cache_currentCacheLRUCounter = entry->lruCounter;
	void *buffer = malloc(k_frameSize);
	memcpy(buffer, entry->pageContentPointer, k_frameSize);
	pthread_rwlock_unlock(&cacheMemoryRwlock);
	return buffer;
}

//////// Fin de código de cache

//////// Interfaz pública

int mem_initProcess(int32_t processID, int32_t numberOfPages) {
	pthread_rwlock_wrlock(&physicalMemoryRwlock);

	if (numberOfPages == 0) {
		pthread_rwlock_unlock(&physicalMemoryRwlock);
		return 0;
	}

	if (isProcessAlreadyInitialized(processID)) {
		pthread_rwlock_unlock(&physicalMemoryRwlock);
		return 0;
	}

	if (!hasMemoryAvailablePages(numberOfPages)) {
		pthread_rwlock_unlock(&physicalMemoryRwlock);
		return 0;
	}

	int i;
	for (i = 0; i < numberOfPages; i++) {
		assignPageToProcess(processID, i);
	}

	pthread_rwlock_unlock(&physicalMemoryRwlock);

	return 1;
}

void *mem_read(int32_t processID, int32_t processPageNumber, int32_t offset, int32_t size) {
	pthread_rwlock_rdlock(&physicalMemoryRwlock);

	if (!isProcessAlreadyInitialized(processID)) {
		pthread_rwlock_unlock(&physicalMemoryRwlock);
		return NULL;
	}

	if (!areOffsetAndSizeValid(offset, size)) {
		pthread_rwlock_unlock(&physicalMemoryRwlock);
		return NULL;
	}

	// Primero revisamos si podemos encontrar
	// los datos en la cache
	void *cache = cache_read(processID, processPageNumber);
	if (cache != NULL) {
		void *pointer = cache + offset;
		void *buffer = malloc(size);
		memcpy(buffer, pointer, size);
		pthread_rwlock_unlock(&physicalMemoryRwlock);
		return buffer;
	}

	// Si no estaban en la cache, los vamos a buscar a
	// la memoria física
	int pageIndex = findPageIndex(processID, processPageNumber);
	if (pageIndex == -1) { pthread_rwlock_unlock(&physicalMemoryRwlock); return NULL; }

	void *frame = getFramePointerForPageIndex(pageIndex);

	// Guardamos el frame en la cache
	cache_write(processID, processPageNumber, frame);

	// Copiamos los bytes pedidos en un buffer y lo devolvemos
	void *pointer = frame + offset;
	void *buffer = malloc(size);
	memcpy(buffer, pointer, size);
	millisleep(k_physicalMemoryAccessDelay);

	pthread_rwlock_unlock(&physicalMemoryRwlock);
	return buffer;
}

int mem_write(int32_t processID, int32_t processPageNumber, int32_t offset, int32_t size, void *buffer) {
	pthread_rwlock_wrlock(&physicalMemoryRwlock);

	if (!isProcessAlreadyInitialized(processID)) {
		pthread_rwlock_unlock(&physicalMemoryRwlock);
		return 0;
	}

	if (!areOffsetAndSizeValid(offset, size)) {
		pthread_rwlock_unlock(&physicalMemoryRwlock);
		return 0;
	}

	// Si la página no existe, devolvemos error.
	int pageIndex = findPageIndex(processID, processPageNumber);
	if (pageIndex == -1) { pthread_rwlock_unlock(&physicalMemoryRwlock); return 0; }
	void *frame = getFramePointerForPageIndex(pageIndex);

	// Si existe, escribimos los datos
	void *pointer = frame + offset;
	memcpy(pointer, buffer, size);
	millisleep(k_physicalMemoryAccessDelay);

	// Luego guardamos el frame en la cache
	cache_write(processID, processPageNumber, frame);

	pthread_rwlock_unlock(&physicalMemoryRwlock);
	return 1;
}

int mem_addPagesToProcess(int32_t processID, int32_t numberOfPages) {
	pthread_rwlock_wrlock(&physicalMemoryRwlock);

	if (numberOfPages == 0) {
		pthread_rwlock_unlock(&physicalMemoryRwlock);
		return 0;
	}

	if (!isProcessAlreadyInitialized(processID)) {
		pthread_rwlock_unlock(&physicalMemoryRwlock);
		return 0;
	}

	if (!hasMemoryAvailablePages(numberOfPages)) {
		pthread_rwlock_unlock(&physicalMemoryRwlock);
		return 0;
	}

	int pagesOriginallyOwned = numberOfPagesOwnedByProcess(processID);

	int i;
	for (i = 0; i < numberOfPages; i++) {
		assignPageToProcess(processID, pagesOriginallyOwned + i);
	}

	pthread_rwlock_unlock(&physicalMemoryRwlock);

	return 1;
}

int mem_deinitProcess(int32_t processID) {
	pthread_rwlock_wrlock(&physicalMemoryRwlock);
	pthread_rwlock_wrlock(&cacheMemoryRwlock);

	// Primero destruimos las entradas de las páginas
	// de la memoria física
	int i;
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

	pthread_rwlock_unlock(&physicalMemoryRwlock);
	pthread_rwlock_unlock(&cacheMemoryRwlock);
	return 1;
}

//////// Fin de interfaz pública

//////// Log de size

void size_logMemorySize() {
	pthread_rwlock_rdlock(&physicalMemoryRwlock);

	int numberOfUsedFrames = k_numberOfFrames - k_numberOfPages;
	int i;
	for (i = 0; i < k_numberOfPages; i++) {
		mem_page_entry *entry = getPageEntryPointerForIndex(i);
		if (entry->processID != -1) {
			numberOfUsedFrames++;
		}
	}

	int numberOfAvailableFrames = k_numberOfFrames - numberOfUsedFrames;

	printf("Frames totales: %d; Frames utilizados: %d; Frames disponibles: %d. \n\n", k_numberOfFrames, numberOfUsedFrames, numberOfAvailableFrames);
	pthread_rwlock_unlock(&physicalMemoryRwlock);
}

//////// Fin de log de size

//////// Dumps

void dump_cache() {
	pthread_rwlock_rdlock(&cacheMemoryRwlock);
	char *logPath = "./src/cache_dump.txt";
	t_log *log = log_create(logPath, "memoria", 1, LOG_LEVEL_INFO);

	int i;
	for (i = 0; i < k_numberOfEntriesInCache; i++) {
		mem_cached_page_entry *entry = cache_getEntryPointerForIndex(i);
		char *buffer = malloc(k_frameSize + 1);
		memcpy(buffer, entry->pageContentPointer, k_frameSize);

		int j;
		for (j = 0; j < k_frameSize; j++) {
			if (buffer[j] == '\0') {
				buffer[j] = '-';
			}
		}
		buffer[k_frameSize] = '\0';

		log_info(log, "(Entrada de cache n° %d) processID: %d; processPageNumber: %d; lruCounter: %d; pageContent: %s", i, entry->processID, entry->processPageNumber, entry->lruCounter, buffer);

		free(buffer);
	}

	log_destroy(log);
	pthread_rwlock_unlock(&cacheMemoryRwlock);
}

void dump_pageEntriesAndActiveProcesses() {
	pthread_rwlock_rdlock(&physicalMemoryRwlock);
	char *logPath = "./src/pages_table_dump.txt";
	t_log *log = log_create(logPath, "memoria", 0, LOG_LEVEL_INFO);

	int i;
	for (i = 0; i < k_numberOfPages; i++) {
		mem_page_entry *entry = getPageEntryPointerForIndex(i);
		log_info(log, "(Entrada de tabla de páginas n° %d) processID: %d; processPageNumber: %d", i, entry->processID, entry->processPageNumber);
	}

	log_info(log, "Listado de procesos activos (processID):");
	int *processesIDs = activeProcessesIDs();
	for (i = 0; *(processesIDs + i) != -1; i++) {
		log_info(log, "%d", *(processesIDs + i));
	}
	free(processesIDs);
	log_destroy(log);
	pthread_rwlock_unlock(&physicalMemoryRwlock);
}

void dump_pagesContentForProcess(u_int32_t processID) {
	pthread_rwlock_rdlock(&physicalMemoryRwlock);
	char *logPath = "./src/process_memory_dump.txt";
	t_log *log = log_create(logPath, "memoria", 0, LOG_LEVEL_INFO);

	log_info(log, "Contenido de las páginas asignadas para proceso (processID: %d)", processID);
	int i;
	for (i = 0; i < k_numberOfPages; i++) {
		mem_page_entry *entry = getPageEntryPointerForIndex(i);
		if (entry->processID == processID) {
			void *frame = getFramePointerForPageIndex(i);
			char *buffer = malloc(k_frameSize + 1);
			memcpy(buffer, frame, k_frameSize);

			int j;
			for (j = 0; j < k_frameSize; j++) {
				if (buffer[j] == '\0') {
					buffer[j] = '-';
				}
			}
			buffer[k_frameSize] = '\0';

			log_info(log, "(Virtual Page n° %d) pageContent: %s", entry->processPageNumber, buffer);
			free(buffer);
		}
	}

	log_destroy(log);
	pthread_rwlock_unlock(&physicalMemoryRwlock);
}

void dump_assignedPagesContent() {
	pthread_rwlock_rdlock(&physicalMemoryRwlock);
	int *processesIDs = activeProcessesIDs();
	int i;
	for (i = 0; *(processesIDs + i) != -1; i++) {
		dump_pagesContentForProcess(*(processesIDs + i));
	}
	free(processesIDs);
	pthread_rwlock_unlock(&physicalMemoryRwlock);
}

//////// Fin de dumps

//////// Comienzo de consola

void menu_dumpSpecificProcessPagesContents() {
	printf("Introduzca el processID del proceso para el cual desea dumpear sus contenidos de memoria: \n> ");
	int processID = 0;
	scanf("%d", &processID);
	dump_pagesContentForProcess(processID);
}

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
		printf("¿Qué desea dumpear? (0 para cancelar): \n1. Memoria cache\n2. Tabla de páginas y listado de procesos activos\n3. Contenido de la memoria completa\n4. Contenido de la memoria para un proceso en particular\n> ");
		scanf("%d", &optionIndex);
		switch (optionIndex) {
		case 0: printf("No se dumpeó nada.\n\n"); break;
		case 1: dump_cache(); break;
		case 2: dump_pageEntriesAndActiveProcesses(); break;
		case 3: dump_assignedPagesContent(); break;
		case 4: menu_dumpSpecificProcessPagesContents(); break;
		default: printf("Opción inválida, vuelva a intentar.\n\n"); break;
		}
	} while (optionIndex > 3);
}

void menu_flush() {
	pthread_rwlock_wrlock(&cacheMemoryRwlock);
	int i;
	for (i = 0; i < k_numberOfEntriesInCache; i++) {
		mem_cached_page_entry *entry = cache_getEntryPointerForIndex(i);
		entry->processID = -1;
		entry->processPageNumber = -1;
		entry->lruCounter = 0;
		char *pageContentPointer = entry->pageContentPointer;
		int j;
		for (j = 0; j < k_frameSize; j++) {
			pageContentPointer[j] = '\0';
		}
	}
	printf("Se ha limpiado la memoria cache.\n\n");
	pthread_rwlock_unlock(&cacheMemoryRwlock);
}

void menu_size() {
	int optionIndex = 0;
		do {
			printf("¿El tamaño de qué desea obtener? (0 para cancelar): \n1. Memoria física completa\n2. Un proceso en particular\n> ");
			scanf("%d", &optionIndex);
			switch (optionIndex) {
			case 0: printf("Se canceló la consulta de tamaño.\n\n"); break;
			case 1: size_logMemorySize(); break;
			case 2: printf("TODO: pedir processID y dar el tamaño que ocupa.\n\n"); break;
			default: printf("Opción inválida, vuelva a intentar.\n\n"); break;
			}
		} while (optionIndex > 2);
}

//////// Fin de consola

void *serverThread_main(void *);

int main(int argc, char **argv) {
	pthread_rwlock_init(&physicalMemoryRwlock, NULL);
	pthread_rwlock_init(&cacheMemoryRwlock, NULL);

	{ // Log
		char *logPath = "./src/debug.txt";
		consoleLog = log_create(logPath, "memoria", 1, LOG_LEVEL_DEBUG);
	}

	{ // Configuración
		char *configPath = (argc > 1) ? argv[1] : "./src/config.txt";
		printf("Levantando configuración del archivo '%s'.\n", configPath);
		t_config *config = config_create(configPath);
		k_connectionPort = config_get_int_value(config, "PUERTO");
		k_numberOfFrames = config_get_int_value(config, "MARCOS");
		k_frameSize = config_get_int_value(config, "MARCO_SIZE");
		k_numberOfEntriesInCache = config_get_int_value(config, "ENTRADAS_CACHE");
		k_maxPagesForEachProcessInCache = config_get_int_value(config, "CACHE_X_PROC");
		k_physicalMemoryAccessDelay = config_get_int_value(config, "RETARDO_MEMORIA");
		config_destroy(config);
	}

	{ // Physical memory initialization
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
	}

	{ // Cache memory initialization
		printf("Inicializando memoria cache.\n");
		cache = malloc(k_numberOfEntriesInCache * (sizeof(mem_cached_page_entry) + k_frameSize));
		int i;
		for (i = 0; i < k_numberOfEntriesInCache; i++) {
			mem_cached_page_entry *entry = cache_getEntryPointerForIndex(i);
			entry->processID = -1;
			entry->processPageNumber = -1;
			entry->lruCounter = 0;
			entry->pageContentPointer = (void *)entry + sizeof(mem_cached_page_entry);
		}
	}

	{	// Server
		pthread_t threadID;
		pthread_create(&threadID, NULL, serverThread_main, NULL);
	}

	printf("Todo configurado y funcionando.\n\n");

	{ // Presentamos menú
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
	}

	pthread_rwlock_destroy(&physicalMemoryRwlock);
	pthread_rwlock_destroy(&cacheMemoryRwlock);
	free(physicalMemory);
	free(cache);
	log_destroy(consoleLog);

	return EXIT_SUCCESS;
}

void *connection_handler(void *shit) {
	int sockfd = *(int*)shit;
	ipc_header header;

	while (1) {
		recv(sockfd, &header, sizeof(ipc_header), MSG_PEEK);

		log_debug(consoleLog, "Operation identifier: %d", header.operationIdentifier);

		switch (header.operationIdentifier) {
		case MEMORY_INIT_PROGRAM: {
			ipc_struct_memory_init_program request;
			recv(sockfd, &request, sizeof(ipc_struct_memory_init_program), 0);
			log_debug(consoleLog, "Init program. pid: %d. numberOfPages: %d", request.pid, request.numberOfPages);
			int result = mem_initProcess(request.pid, request.numberOfPages);

			ipc_struct_memory_init_program_response response;
			response.header.operationIdentifier = MEMORY_INIT_PROGRAM_RESPONSE;
			response.success = result > 0 ? 1 : 0;

			send(sockfd, &response, sizeof(ipc_struct_memory_init_program_response), 0);
			break;
		}
		case MEMORY_READ: {
			ipc_struct_memory_read request;
			recv(sockfd, &request, sizeof(ipc_struct_memory_read), 0);
			log_debug(consoleLog, "Read page; pid: %d; pageNumber: %d; offset: %d; size: %d", request.pid, request.pageNumber, request.offset, request.size);
			void *buffer = mem_read(request.pid, request.pageNumber, request.offset, request.size);
			log_debug(consoleLog, "Content: %s", buffer);

			char success = buffer != NULL ? 1 : 0;

			int totalSize = sizeof(ipc_header) + sizeof(char) + sizeof(int) + request.size;
			void *buf = malloc(totalSize);

			ipc_header responseHeader;
			responseHeader.operationIdentifier = MEMORY_READ_RESPONSE;

			memcpy(buf, &responseHeader, sizeof(ipc_header));
			memcpy(buf + sizeof(ipc_header), &success, sizeof(char));
			memcpy(buf + sizeof(ipc_header) + sizeof(char), &(request.size), sizeof(int));
			memcpy(buf + sizeof(ipc_header) + sizeof(char) + sizeof(int), buffer, request.size);


			send(sockfd, buf, totalSize, 0);
			free(buf);
			free(buffer);

			break;
		}
		case MEMORY_WRITE: {
			recv(sockfd, &header, sizeof(ipc_header), 0);
			ipc_struct_memory_write request;

			recv(sockfd, &(request.pid), sizeof(int), 0);
			recv(sockfd, &(request.pageNumber), sizeof(int), 0);
			recv(sockfd, &(request.offset), sizeof(int), 0);
			recv(sockfd, &(request.size), sizeof(int), 0);

			void *buffer = malloc(request.size);
			recv(sockfd, buffer, request.size, 0);
			request.buffer = buffer;

			log_debug(consoleLog, "Write. pid: %d. pageNumber: %d. offset: %d. size: %d. buffer: %s", request.pid, request.pageNumber, request.offset, request.size, request.buffer);
			int result = mem_write(request.pid, request.pageNumber, request.offset, request.size, request.buffer);

			ipc_struct_memory_write_response response;
			response.header.operationIdentifier = MEMORY_WRITE_RESPONSE;
			response.success = result > 0 ? 1 : 0;

			send(sockfd, &response, sizeof(ipc_struct_memory_write_response), 0);
			break;
		}
		case MEMORY_REQUEST_MORE_PAGES: {
			ipc_struct_memory_request_more_pages request;
			recv(sockfd, &request, sizeof(ipc_struct_memory_request_more_pages), 0);
			log_debug(consoleLog, "Request more pages for program. pid: %d. numberOfPages: %d", request.pid, request.numberOfPages);
			int result = mem_addPagesToProcess(request.pid, request.numberOfPages);

			ipc_struct_memory_request_more_pages_response response;
			response.header.operationIdentifier = MEMORY_REQUEST_MORE_PAGES_RESPONSE;
			response.success = result > 0 ? 1 : 0;

			send(sockfd, &response, sizeof(ipc_struct_memory_request_more_pages_response), 0);
			break;
		}
		case MEMORY_DEINIT_PROGRAM: {
			ipc_struct_memory_deinit_program request;
			recv(sockfd, &request, sizeof(ipc_struct_memory_deinit_program), 0);
			log_debug(consoleLog, "Denit program. pid: %d.", request.pid);
			int result = mem_deinitProcess(request.pid);

			ipc_struct_memory_deinit_program_response response;
			response.header.operationIdentifier = MEMORY_DEINIT_PROGRAM_RESPONSE;
			response.success = result > 0 ? 1 : 0;

			send(sockfd, &response, sizeof(ipc_struct_memory_deinit_program_response), 0);
			break;
		}
		default:
			break;
		}
	}

	return NULL;
}

void *serverThread_main(void *mierda) {
	int socket_desc, client_sock, c;
	struct sockaddr_in server, client;

	//Create socket
	socket_desc = socket(AF_INET, SOCK_STREAM, 0);
	if (socket_desc == -1) {
		log_error(consoleLog, "No se pudo crear el socket");
	}
	log_debug(consoleLog, "Se creo el socket");

	//Prepare the sockaddr_in structure
	server.sin_family = AF_INET;
	server.sin_addr.s_addr = INADDR_ANY;
	server.sin_port = htons(k_connectionPort);

	//Bind
	if (bind(socket_desc, (struct sockaddr *) &server, sizeof(server)) < 0) {
		//print the error message
		log_error(consoleLog, "Falló el bind");
		return NULL;
	}

	//Listen
	listen(socket_desc, 3);

	//Accept and incoming connection
	log_debug(consoleLog, "Waiting for incoming connections...");
	c = sizeof(struct sockaddr_in);

	pthread_t thread_id;

	while ((client_sock = accept(socket_desc, (struct sockaddr *) &client,
			(socklen_t*) &c))) {
		log_debug(consoleLog, "Connection accepted");

		if (pthread_create(&thread_id, NULL, connection_handler,
				(void*) &client_sock) < 0) {
			log_error(consoleLog, "Could not create thread");
			return NULL;
		}

		log_debug(consoleLog, "Handler assigned");
	}

	if (client_sock < 0) {
		log_error(consoleLog, "Accept failed");
		return NULL;
	}

	return 0;
}
