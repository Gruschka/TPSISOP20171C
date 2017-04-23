/*
 ============================================================================
 Name        : memoria.c
 Author      : Federico Trimboli
 Version     :
 Copyright   : Copyright
 Description : Insanely great memory.
 ============================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <limits.h>

typedef unsigned char mem_bool;
static u_int32_t k_numberOfPages;

//////// Fin de código de memoria física

typedef struct mem_page_entry {
    int32_t processID;
    int32_t processPageNumber;
    mem_bool isAvailable;
} mem_page_entry;

static const u_int32_t k_numberOfFrames = 500;
static const u_int32_t k_frameSize = 256;
static void *physicalMemory;
static const u_int32_t k_physicalMemoryAccessDelay = 100;

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
		if (possiblyAvailableEntry->isAvailable) {
			availableEntry = possiblyAvailableEntry;
			break;
		}
	}

	if (availableEntry != NULL) {
		return i;
	}

	for (i = hash - 1; i >= 0; i--) {
		mem_page_entry *possiblyAvailableEntry = getPageEntryPointerForIndex(i);
			if (possiblyAvailableEntry->isAvailable) {
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
		if (entry->isAvailable) {
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
	entry->isAvailable = 0;
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

static const u_int32_t k_maxPagesInCache = 15;
static const u_int32_t k_maxPagesForEachProcessInCache = 3;
static void *cache;
static u_int32_t cache_currentCacheLRUCounter = 0;

mem_cached_page_entry *cache_getEntryPointerForIndex(int index) {
	return cache + index * (sizeof(mem_cached_page_entry) + k_frameSize);
}

mem_cached_page_entry *cache_getEntryPointer(int32_t processID, int32_t processPageNumber) {
	int i;
	for (i = 0; i < k_maxPagesInCache; i++) {
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
	for (i = 1; i < k_maxPagesInCache; i++) {
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

void millisleep(u_int32_t milliseconds) {
	struct timespec ts;
	ts.tv_sec = milliseconds / 1000;
	ts.tv_nsec = (milliseconds % 1000) * 1000000;
	nanosleep(&ts, NULL);
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
			entry->isAvailable = 1;
		}
	}

	// Luego destruimos las entradas de las páginas
	// de la cache
	for (i = 0; i < k_maxPagesInCache; i++) {
		mem_cached_page_entry *entry = cache_getEntryPointerForIndex(i);
		if (entry->processID == processID) {
			entry->processID = -1;
			entry->processPageNumber = -1;
			entry->lruCounter = 0;
		}
	}
}

//////// Fin de interfaz pública

int main(void) {
	// Physical memory initialization
	int totalNumberOfBytes = k_numberOfFrames * k_frameSize;
	physicalMemory = malloc(totalNumberOfBytes);
	k_numberOfPages = floor(totalNumberOfBytes / (k_frameSize + sizeof(mem_page_entry)));

	int i;
	for (i = 0; i < k_numberOfPages; i++) {
		mem_page_entry *entry = getPageEntryPointerForIndex(i);
		entry->processID = -1;
		entry->processPageNumber = -1;
		entry->isAvailable = 1;
	}

	// Cache memory initialization
	cache = malloc(k_maxPagesInCache * (sizeof(mem_cached_page_entry) + k_frameSize));
	for (i = 0; i < k_maxPagesInCache; i++) {
		mem_cached_page_entry *entry = cache_getEntryPointerForIndex(i);
		entry->processID = -1;
		entry->processPageNumber = -1;
		entry->lruCounter = 0;
		entry->pageContentPointer = (void *)entry + sizeof(mem_cached_page_entry);
	}

	int a = mem_initProcess(0, 10);
	int b = mem_initProcess(1, 10);
	int c = mem_initProcess(2, 10);
	int d = mem_initProcess(0, 1);

	char *texto = "esto es una prueba capo";
	int a1 = mem_write(0, 0, 0, 24, texto);
	int a2 = mem_write(0, 9, 0, 24, texto);
	int a3 = mem_write(1, 2, 0, 24, texto);
	int a4 = mem_write(0, 10, 0, 24, texto);

	char *meTraje1 = mem_read(0, 0, 0, 24);
	char *meTraje2 = mem_read(0, 9, 0, 24);
	char *meTraje3 = mem_read(1, 2, 0, 24);
	char *meTraje4 = mem_read(0, 10, 0, 24);

	int b1 = mem_addPagesToProcess(0, 1);
	int b2 = mem_write(0, 10, 0, 24, texto);
	char *b3 = mem_read(0, 10, 0, 24);
	char *b4 = mem_read(0, 11, 0, 24);

	mem_deinitProcess(0);

	char *c1 = mem_read(0, 0, 0, 24);
	char *c2 = mem_read(1, 2, 0, 24);

	return EXIT_SUCCESS;
}

