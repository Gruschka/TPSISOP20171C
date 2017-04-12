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

typedef struct mem_virtual_page {
    int processID;
    int pageNumber;
    int nextPageIndex;
} mem_virtual_page;

mem_virtual_page *pages[10];

int cantor(int k1, int k2) {
	return 0.5 * (k1 + k2) * (k1 + k2 + 1) + k2;
}

int hash(int processID, int virtualPageNumber) {
	return cantor(processID, virtualPageNumber) % 10;
}

int isPageEmpty(mem_virtual_page *page) {
	return page->processID == -1 && page->pageNumber == -1;
}

mem_virtual_page *findUnusedPageForGivenIndex(int index) {
	mem_virtual_page *page = pages[index];

	if (isPageEmpty(page)) {
		return page;
	}

	if (page->nextPageIndex != -1) {
		return findUnusedPageForGivenIndex(page->nextPageIndex);
	}

	mem_virtual_page *unusedPage = NULL;
	int i;

	for (i = index + 1; i < 10; i++) {
		mem_virtual_page *possiblyUnusedPage = pages[i];
		if (isPageEmpty(possiblyUnusedPage)) {
			unusedPage = possiblyUnusedPage;
			break;
		}
	}

	if (unusedPage != NULL) {
		page->nextPageIndex = i;
		return unusedPage;
	}

	for (i = index - 1; i >= 0; i--) {
			mem_virtual_page *possiblyUnusedPage = pages[i];
			if (isPageEmpty(possiblyUnusedPage)) {
				unusedPage = possiblyUnusedPage;
				break;
			}
		}

	if (unusedPage != NULL) {
		page->nextPageIndex = i;
		return unusedPage;
	}

	return NULL;
}

int mem_alloc(int processID, int virtualPageNumber) {
	int index = hash(processID, virtualPageNumber);
	mem_virtual_page *page = findUnusedPageForGivenIndex(index);

	if (page == NULL) {
		return -1;
	}

	page->processID = processID;
	page->pageNumber = virtualPageNumber;
	return 0;
}

int main(void) {
	int i;
	for (i = 0; i < 10; i++) {
		 mem_virtual_page *page = malloc(sizeof(mem_virtual_page));
		 page->processID = -1;
		 page->pageNumber = -1;
		 page->nextPageIndex = -1;
		 pages[i] = page;
	}

	printf("%d", mem_alloc(0, 5));
	printf("%d", mem_alloc(10, 2));
	printf("%d", mem_alloc(2, 3));
	printf("%d", mem_alloc(10, 1));
	printf("%d", mem_alloc(3, 1));
	printf("%d", mem_alloc(1, 2));
	printf("%d", mem_alloc(21, 4));
	printf("%d", mem_alloc(2, 2));
	printf("%d", mem_alloc(31, 5));
	printf("%d", mem_alloc(100, 100));

	return EXIT_SUCCESS;
}
