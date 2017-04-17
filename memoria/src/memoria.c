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

typedef struct mem_table_entry {
	int32_t frameNumber;
    int32_t processID;
    int32_t pageNumber;
} mem_table_entry;

static const int32_t k_numberOfFrames = 500;
static const int32_t k_frameSize = 256;
static void *memory;

//static int32_t k_numberOfAvailablePages = 100;
mem_table_entry *tableEntries[500];

int64_t pair(int32_t k1, int32_t k2) {
	return ((int64_t)k1 << 32) + (int64_t)k2;
}

int32_t hash(int32_t processID, int32_t pageNumber) {
	return pair(processID, pageNumber) % k_numberOfFrames;
}

int isEntryAvailable(mem_table_entry *entry) {
	return entry->processID == -1 && entry->pageNumber == -1;
}

mem_table_entry *findAvailableEntryForGivenIndex(int index) {
	mem_table_entry *entry = tableEntries[index];

	if (isEntryAvailable(entry)) {
		return entry;
	}

	mem_table_entry *availableEntry = NULL;
	int i;

	for (i = index + 1; i < k_numberOfFrames; i++) {
		mem_table_entry *possiblyAvailableEntry = tableEntries[i];
		if (isEntryAvailable(possiblyAvailableEntry)) {
			availableEntry = possiblyAvailableEntry;
			break;
		}
	}

	if (availableEntry != NULL) {
		return availableEntry;
	}

	for (i = index - 1; i >= 0; i--) {
		mem_table_entry *possiblyAvailableEntry = tableEntries[i];
			if (isEntryAvailable(possiblyAvailableEntry)) {
				availableEntry = possiblyAvailableEntry;
				break;
			}
		}

	if (availableEntry != NULL) {
		return availableEntry;
	}

	return NULL;
}

int doesEntryMatchProcessAndPageNumber(mem_table_entry *entry, int32_t processID, int32_t pageNumber) {
	if (entry->processID == processID && entry->pageNumber == pageNumber) {
		return 1;
	}

	return 0;
}

mem_table_entry *findEntry(int32_t processID, int32_t pageNumber) {
	int32_t index = hash(processID, pageNumber);
	mem_table_entry *entry = NULL;

	int32_t i = index;
	do {
		mem_table_entry *possibleEntry = tableEntries[i];
		if (doesEntryMatchProcessAndPageNumber(possibleEntry, processID, pageNumber)) {
			entry = possibleEntry;
			break;
		}
		i++;
	} while (index < k_numberOfFrames);

	if (entry != NULL) {
		return entry;
	}

	i = index - 1;
	if (i < 0) { return NULL; }
	do {
		mem_table_entry *possibleEntry = tableEntries[i];
		if (doesEntryMatchProcessAndPageNumber(possibleEntry, processID, pageNumber)) {
			entry = possibleEntry;
			break;
		}
		i--;
	} while (index >= 0);

	return entry;
}

int offsetAndSizeAreValid(int32_t offset, int32_t size) {
	if (size == 0) { return 0; }
	return (offset + size) <= k_frameSize;
}

//////// Interfaz pública

void initProcess(int32_t processID, int32_t numberOfPages) {

}

void *valueFor(int32_t processID, int32_t pageNumber, int32_t offset, int32_t size) {
	if (!offsetAndSizeAreValid(offset, size)) { return 0; }

	mem_table_entry *entry = findEntry(processID, pageNumber);
	void *pointer = memory + (entry->frameNumber * k_frameSize) + offset;
	void *buffer = malloc(size);
	memcpy(buffer, pointer, size);
	return buffer;
}

int saveValueFor(int32_t processID, int32_t pageNumber, int32_t offset, int32_t size, void *buffer) {
	if (!offsetAndSizeAreValid(offset, size)) { return 0; }

	int index = hash(processID, pageNumber);
	mem_table_entry *entry = findAvailableEntryForGivenIndex(index);
	if (entry == NULL) { return 0; }
	entry->processID = processID;
	entry->pageNumber = pageNumber;
	void *pointer = memory + (entry->frameNumber * k_frameSize) + offset;
	memcpy(pointer, buffer, size);
	return 1;
}

void addPagesToExistingProcess(int32_t processID, int32_t numberOfPages) {

}

void deinitProcessMemory(int32_t processID) {

}

//////// Fin de interfaz pública

int main(void) {
	memory = calloc(k_numberOfFrames, k_frameSize);

	int i;
	for (i = 0; i < k_numberOfFrames; i++) {
		mem_table_entry *entry = malloc(sizeof(mem_table_entry));
		entry->frameNumber = i;
		entry->processID = -1;
		entry->pageNumber = -1;
		tableEntries[i] = entry;
	}

	char *texto = "esto es una prueba capo";
	saveValueFor(0, 0, 233, 23, texto);

	char *meTraje = valueFor(0, 0, 233, 23);

	return EXIT_SUCCESS;
}
