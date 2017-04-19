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

typedef unsigned char mem_bool;

typedef struct mem_page_entry {
    int32_t processID;
    int32_t processPageNumber;
    mem_bool isAvailable;
} mem_page_entry;

static const u_int32_t k_numberOfFrames = 500;
static const u_int32_t k_frameSize = 256;
static void *physicalMemory;
static u_int32_t k_numberOfPages;

int64_t calculatePair(int32_t k1, int32_t k2) {
	return ((int64_t)k1 << 32) + (int64_t)k2;
}

int32_t calculateHash(int32_t processID, int32_t pageNumber) {
	return calculatePair(processID, pageNumber) % k_numberOfFrames;
}

mem_page_entry *getPageEntryForIndex(int index) {
	return physicalMemory + index * sizeof(mem_page_entry);
}

void *getPagePointerForIndex(int index) {
	return physicalMemory + ((k_numberOfFrames - k_numberOfPages + index) * k_frameSize);
}

int findAvailablePageIndex(int32_t processID, int32_t pageNumber) {
	int hash = calculateHash(processID, pageNumber);

	mem_page_entry *availableEntry = NULL;
	int i;
	for (i = hash; i < k_numberOfPages; i++) {
		mem_page_entry *possiblyAvailableEntry = getPageEntryForIndex(i);
		if (possiblyAvailableEntry->isAvailable) {
			availableEntry = possiblyAvailableEntry;
			break;
		}
	}

	if (availableEntry != NULL) {
		return i;
	}

	for (i = hash - 1; i >= 0; i--) {
		mem_page_entry *possiblyAvailableEntry = getPageEntryForIndex(i);
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

mem_bool doesEntryMatchProcessAndPageNumber(mem_page_entry *entry, int32_t processID, int32_t pageNumber) {
	if (entry->processID == processID && entry->processPageNumber == pageNumber) {
		return 1;
	}

	return 0;
}

int findPageIndex(int32_t processID, int32_t pageNumber) {
	int32_t hash = calculateHash(processID, pageNumber);
	mem_page_entry *entry = NULL;

	int32_t i;
	for (i = hash; i < k_numberOfPages; i++) {
		mem_page_entry *possibleEntry = getPageEntryForIndex(i);
		if (doesEntryMatchProcessAndPageNumber(possibleEntry, processID, pageNumber)) {
			entry = possibleEntry;
			break;
		}
	}

	if (entry != NULL) {
		return i;
	}

	for (i = hash - 1; i >= 0; i--) {
		mem_page_entry *possibleEntry = getPageEntryForIndex(i);
		if (doesEntryMatchProcessAndPageNumber(possibleEntry, processID, pageNumber)) {
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
		mem_page_entry *entry = getPageEntryForIndex(i);
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
		mem_page_entry *entry = getPageEntryForIndex(i);
		if (entry->isAvailable) {
			availablePages += 1;
			if (availablePages == numberOfPages) {
				return 1;
			}
		}
	}
	return 0;
}

int assignPageToProcess(int32_t processID, int32_t pageNumber) {
	int pageIndex = findPageIndex(processID, pageNumber);
	if (pageIndex != -1) { return 0; }

	pageIndex = findAvailablePageIndex(processID, pageNumber);
	if (pageIndex == -1) { return 0; }
	mem_page_entry *entry = getPageEntryForIndex(pageIndex);
	entry->processID = processID;
	entry->processPageNumber = pageNumber;
	entry->isAvailable = 0;
	return 1;
}

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

void *mem_read(int32_t processID, int32_t pageNumber, int32_t offset, int32_t size) {
	if (!isProcessAlreadyInitialized(processID)) { return NULL; }
	if (!areOffsetAndSizeValid(offset, size)) { return NULL; }

	int pageIndex = findPageIndex(processID, pageNumber);
	if (pageIndex == -1) { return NULL; }

	void *pointer = getPagePointerForIndex(pageIndex) + offset;
	void *buffer = malloc(size);
	memcpy(buffer, pointer, size);
	return buffer;
}

int mem_write(int32_t processID, int32_t pageNumber, int32_t offset, int32_t size, void *buffer) {
	if (!isProcessAlreadyInitialized(processID)) { return 0; }
	if (!areOffsetAndSizeValid(offset, size)) { return 0; }

	int pageIndex = findPageIndex(processID, pageNumber);
	if (pageIndex == -1) { return 0; }
	mem_page_entry *entry = getPageEntryForIndex(pageIndex);
	entry->processID = processID;
	entry->processPageNumber = pageNumber;
	void *pointer = getPagePointerForIndex(pageIndex) + offset;
	memcpy(pointer, buffer, size);
	return 1;
}

void addPagesToExistingProcess(int32_t processID, int32_t numberOfPages) {

}

void deinitProcessMemory(int32_t processID) {

}

//////// Fin de interfaz pública

int main(void) {
	int totalNumberOfBytes = k_numberOfFrames * k_frameSize;
	physicalMemory = malloc(totalNumberOfBytes);
	k_numberOfPages = floor(totalNumberOfBytes / (k_frameSize + sizeof(mem_page_entry)));

	int i;
	for (i = 0; i < k_numberOfPages; i++) {
		mem_page_entry *entry = getPageEntryForIndex(i);
		entry->processID = -1;
		entry->processPageNumber = -1;
		entry->isAvailable = 1;
	}

	int a = mem_initProcess(0, 10);
	int b = mem_initProcess(1, 10);
	int c = mem_initProcess(2, 10);
	int d = mem_initProcess(0, 1);

	char *texto = "esto es una prueba capo";
	int a1 = mem_write(0, 0, 0, 23, texto);
	int a2 = mem_write(0, 9, 0, 23, texto);
	int a3 = mem_write(1, 2, 0, 23, texto);
	int a4 = mem_write(0, 10, 0, 23, texto);

	char *meTraje1 = mem_read(0, 0, 0, 23);
	char *meTraje2 = mem_read(0, 9, 0, 23);
	char *meTraje3 = mem_read(1, 2, 0, 23);
	char *meTraje4 = mem_read(0, 10, 0, 23);

	return EXIT_SUCCESS;
}
