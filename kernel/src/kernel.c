/*
 ============================================================================
 Name        : kernel.c
 Author      : Hernan Canzonetta
 Version     :
 Copyright   : Copyright
 Description : Hello World in C, Ansi-style
 ============================================================================
 */

#include "kernel.h"

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <pthread.h>
#include <sys/socket.h>
#include <sys/inotify.h>
#include <unistd.h>
#include <netdb.h>
#include <signal.h>
#include <netinet/in.h>

#include "memory.h"
#include "scheduling.h"
#include "shared_variables.h"
#include <commons/config.h>
#include <commons/log.h>
#include <parser/metadata_program.h>
#include "semaphore.h"

#define EVENT_SIZE  ( sizeof (struct inotify_event) )
#define EVENT_BUF_LEN     ( 1024 * ( EVENT_SIZE + 16 ) )

//Private
static t_config *__config;

//Globals
t_kernel_config *configuration;
t_log *logger;
t_list *sharedVariables;
t_list *cpusList; //TODO: Sincronizar acceso a esta lista
t_list *semaphores; //TODO: Sincronizar acceso a esta lista
t_list *activeConsoles;

pthread_t consolesServerThread;
pthread_t cpusServerThread;
pthread_t schedulerThread;
pthread_t dispatcherThread;
pthread_t configurationWatcherThread;

uint32_t lastPID = 1;
uint32_t pageSize = 256;
uint32_t stackSize = 2; // FIXME: levantar de archivo de config

int memory_sockfd;

//void sigintHandler(int sig_num)
//{
//    /* Reset handler to catch SIGINT next time.
//       Refer http://en.cppreference.com/w/c/program/signal */
//    signal(SIGINT, sigintHandler);
//    printf("\n Cannot be terminated using Ctrl+C \n");
//    fflush(stdout);
//}

void semaphoreDidBlockProcess(t_PCB *pcb, char *identifier) {
	log_debug(logger, "[semaphore: %s] Process<PID:%d> did block", identifier, pcb->pid);
	pthread_mutex_lock(&blockQueue_mutex);
	blockQueue_addProcess(pcb);
	pthread_mutex_unlock(&blockQueue_mutex);
}

void semaphoreDidWakeProcess(t_PCB *pcb, char *identifier) {
	pthread_mutex_lock(&blockQueue_mutex);
	pthread_mutex_lock(&readyQueue_mutex);
	t_PCB *awakenPCB = blockQueue_popProcess(pcb->pid);
	readyQueue_addProcess(awakenPCB);
	sem_post(&readyQueue_programsCount);
	log_debug(logger, "[semaphores: %s] Process<PID:%d> did wake up", identifier, pcb->pid);
	pthread_mutex_unlock(&readyQueue_mutex);
	pthread_mutex_unlock(&blockQueue_mutex);
}

void initSharedVariables() {
	int i;

	for (i = 0; configuration->sharedVariableNames[i] != NULL; i++) {
		shared_variable *var = createSharedVariable(configuration->sharedVariableNames[i]);
		list_add(sharedVariables, var);
		log_debug(logger, "created shared variable: %s (value: %d)", var->identifier, var->value);
	}
}

void initSemaphores() {
	kernel_semaphores_init(semaphoreDidBlockProcess,semaphoreDidWakeProcess);
	int i;

	for (i = 0; configuration->semaphoreIDs[i] != NULL; i++) {
		char *identifier = configuration->semaphoreIDs[i];
		int value = atoi(configuration->semaphoreValues[i]);

		kernel_semaphore *sem = kernel_semaphore_make(identifier, value);
		list_add(semaphores, sem);
		log_debug(logger, "created semaphore: %s (value: %d)", sem->identifier, sem->count);
	}
}

void testMemory() {
	void *page = memory_createPage(pageSize);
//	void *block = memory_addBlock(page, 50);
	void *firstBlock = memory_addBlock(page, 200);
	void *secondBlock = memory_addBlock(page, 35);
	void *thirdBlock = memory_addBlock(page, 157);
	void *fourthBlock = memory_addBlock(page, 43);

	log_debug(logger, "fourthBlock: %p", fourthBlock);
	memory_dumpPage(page);

//	shared_variable *a = createSharedVariable("A");
//	shared_variable *b = createSharedVariable("B");
//	shared_variable *c = createSharedVariable("C");
//	shared_variable *global = createSharedVariable("Global");
//	list_add(sharedVariables, a);
//	list_add(sharedVariables, b);
//	list_add(sharedVariables, c);
//	list_add(sharedVariables, global);
//
//	log_debug(logger, "sharedVariables: A: %d. B: %d. C: %d. D: %d",
//			getSharedVariableValue("A"), getSharedVariableValue("B"),
//			getSharedVariableValue("C"), getSharedVariableValue("D"));
//
//	setSharedVariableValue("A", 1);
//	setSharedVariableValue("B", 2);
//	setSharedVariableValue("C", 3);
//	setSharedVariableValue("Global", 112);
//
//	log_debug(logger, "sharedVariables: A: %d. B: %d. C: %d. D: %d",
//			getSharedVariableValue("A"), getSharedVariableValue("B"),
//			getSharedVariableValue("C"), getSharedVariableValue("D"));
}

void testSemaphores() {
	// KERNEL SEMAPHORE TEST START
	// TODO: remove semaphore test
	// function pointer initialization

	// semaphores init
	kernel_semaphores_init(semaphoreDidBlockProcess,semaphoreDidWakeProcess);

	// generate test semaphore & pcb
	t_PCB * dummyPCB1 = malloc(sizeof(t_PCB));
	dummyPCB1->pid = 1;
	t_PCB * dummyPCB2 = malloc(sizeof(t_PCB));
	dummyPCB2->pid = 2;
	t_PCB * dummyPCB3 = malloc(sizeof(t_PCB));
	dummyPCB3->pid = 3;

	char *semaphoreId = malloc(sizeof(char)*2);
	sprintf(semaphoreId,"a\0");
	kernel_semaphore *testSemaphore = kernel_semaphore_make(semaphoreId, 1); // 1 instance

	kernel_semaphore_wait(testSemaphore, dummyPCB1);
	kernel_semaphore_wait(testSemaphore, dummyPCB3);
	kernel_semaphore_wait(testSemaphore, dummyPCB2);

	kernel_semaphore_signal(testSemaphore, dummyPCB1);
	kernel_semaphore_signal(testSemaphore, dummyPCB1);
	kernel_semaphore_signal(testSemaphore, dummyPCB1);

	//destroy semaphore
	kernel_semaphore_destroy(testSemaphore);
	// KERNEL SEMAPHORE TEST END
}

kernel_semaphore *getSemaphoreByIdentifier(char *identifier) {
	int i;
	for (i = 0; i < list_size(semaphores); i++) {
		kernel_semaphore *sem = list_get(semaphores, i);

		if (strcmp(sem->identifier, identifier) == 0) return sem;
	}

	return NULL;
}

int connectToMemory() {
	struct sockaddr_in serv_addr;
	struct hostent *server;

	memory_sockfd = socket(AF_INET, SOCK_STREAM, 0);

	if (memory_sockfd < 0) {
		log_error(logger, "Error opening memory socket");
		return -1;
	}

	server = gethostbyname("10.0.1.143");
	bzero((char *) &serv_addr, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	bcopy((char *) server->h_addr, (char *)&serv_addr.sin_addr.s_addr, server->h_length);
	serv_addr.sin_port = htons(5003);


	if (connect(memory_sockfd, (struct sockaddr*) &serv_addr, sizeof(serv_addr)) < 0) {
		log_error(logger, "Error connecting to memory");
		return -1;
	}

	ipc_client_sendHandshake(KERNEL, memory_sockfd);
	ipc_struct_handshake_response *response = ipc_client_waitHandshakeResponse(memory_sockfd);

	if (response->success == 0) {
		log_error(logger, "Error connecting to memory");
		return -1;
	}

	log_debug(logger, "[memory] connected. page size: %d", response->info);

	return 0;
}

int memory_sendInitProgram(int pid, int numberOfPages) {
	ipc_struct_memory_init_program request;
	request.header.operationIdentifier = MEMORY_INIT_PROGRAM;

	request.numberOfPages = numberOfPages;
	request.pid = pid;

	send(memory_sockfd, &request, sizeof(request), 0);

	ipc_struct_memory_init_program_response response;
	recv(memory_sockfd, &response, sizeof(ipc_struct_memory_init_program_response), 0);

	return (response.success != 0) ? 1 : -1;
}

int memory_sendRequestMorePages(int pid, int numberOfPages) {
	ipc_struct_memory_request_more_pages request;
	request.header.operationIdentifier = MEMORY_REQUEST_MORE_PAGES;

	request.numberOfPages = numberOfPages;
	request.pid = pid;

	send(memory_sockfd, &request, sizeof(request), 0);

	ipc_struct_memory_request_more_pages_response response;
	recv(memory_sockfd, &response, sizeof(ipc_struct_memory_request_more_pages_response), 0);

	return (response.success != 0) ? response.firstPageNumber : -1;
}

int memory_sendRemovePageFromProgram(int pid, int pageNumber) {
	ipc_struct_memory_remove_page_from_program request;
	request.header.operationIdentifier = MEMORY_REMOVE_PAGE_FROM_PROGRAM;
	request.pid = pid;
	request.pageNumber = pageNumber;

	send(memory_sockfd, &request, sizeof(request), 0);

	ipc_header responseHeader;
	recv(memory_sockfd, &responseHeader, sizeof(ipc_header), 0);

	if (responseHeader.operationIdentifier == MEMORY_REMOVE_PAGE_FROM_PROGRAM_RESPONSE) {
		char success;

		recv(memory_sockfd, &success, sizeof(char), 0);
		return (success != 0) ? 1 : -1;
	}

	return -1;
}

int memory_sendDeinitProgram(int pid) {
	ipc_struct_memory_deinit_program request;
	request.header.operationIdentifier = MEMORY_DEINIT_PROGRAM;
	request.pid = pid;

	send(memory_sockfd, &request, sizeof(request), 0);

	ipc_header responseHeader;
	recv(memory_sockfd, &responseHeader, sizeof(ipc_header), 0);

	if (responseHeader.operationIdentifier == MEMORY_DEINIT_PROGRAM_RESPONSE) {
		char success;

		recv(memory_sockfd, &success, sizeof(char), 0);
		return (success != 0) ? 1 : -1;
	}

	return -1;
}

void showOptions() {
	printf("\n\n");
	printf("Ingresar una opción\n");
	printf("1 - Listado de procesos\n");
	printf("2 - Información de un proceso\n");
	printf("3 - Tabla global de archivos\n");
	printf("4 - Modificar grado de multiprogramación\n");
	printf("5 - Finalizar un proceso\n");
	printf("6 - Detener la planificación\n");
	printf("0 - Salir\n");
	printf("\n\n");
}

void showMenu() {
	int optionIndex = 0;
	do {
		showOptions();
		scanf("%d", &optionIndex);
		switch (optionIndex) {
		case 0: printf("Exit"); break;
		case 1: {
			printf("1 - New\n2 - Ready\n3 - Exec\n4 - Block\n5 - Exit\n6- Todos\n");
			int option = 0;
			scanf("%d", &option);
			switch (option) {
				case 1: dump_list("new", newQueue->elements); break;
				case 2: dump_list("ready", readyQueue); break;
				case 3: dump_list("exec", execList); break;
				case 4: dump_list("block", blockQueue); break;
				case 5: dump_list("exit", exitQueue->elements); break;
				case 6: {
					dump_list("new", newQueue->elements);
					dump_list("ready", readyQueue);
					dump_list("exec", execList);
					dump_list("block", blockQueue);
					dump_list("exit", exitQueue->elements);
					break;
				}
				default: break;
			}
		}
		break;
		case 2: printf("TBD\n"); break;
		case 3: printf("TBD\n"); break;
		case 4: printf("TBD\n"); break;
		case 5: printf("TBD\n"); break;
		case 6: printf("TBD\n"); break;
		default: printf("Opción inválida, vuelva a intentar.\n"); break;
		}
	} while (optionIndex != 0);
}

typedef struct kernel_page_assignation {
    int32_t processID;
    int32_t processPageNumber;
    int32_t availableBytes;
} kernel_page_assignment;

t_list *kernel_page_assignments_list;

typedef struct kernel_heap_metadata  {
	int32_t size;
	char isFree;
} kernel_heap_metadata;

int main(int argc, char **argv) {
	// CÓDIGO DEL FEDE BIEN XETO?
	kernel_page_assignments_list = list_create();
	// TERMINA EL CÓDIGO DE FEDE BIEN XETO,

	char *logFile = tmpnam(NULL);

#ifdef DEBUG
	logger = log_create(logFile, "KERNEL", 1, LOG_LEVEL_DEBUG);
#else
	logger = log_create(logFile, "KERNEL", 1, LOG_LEVEL_INFO);
#endif

	log_info(logger, "Comenzó la ejecución");
	log_info(logger, "Log file: %s", logFile);

	if (argc < 2) {
		log_error(logger, "Falta pasar la ruta del archivo de configuracion");
		return EXIT_FAILURE;
	} else {
		__config = config_create(argv[1]);
		configuration = malloc(sizeof(t_kernel_config));
	}

	fetchConfiguration();

	newQueue = newQueue_create();
	readyQueue = readyQueue_create();
	execList = execList_create();
	blockQueue = blockQueue_create();
	exitQueue = exitQueue_create();
	activeConsoles = list_create();
	sharedVariables = list_create();
	cpusList = list_create();
	semaphores = list_create();

	initSharedVariables();
	initSemaphores();

	extern void testFS();
	//	testMemory();
	//	testSemaphores();
	//	testFS();

	if (connectToMemory() == -1) {
		log_error(logger, "La memoria no está corriendo");
		return EXIT_FAILURE;
	}

	int i;
	for (i = 0; i < configuration->multiprogrammingDegree; i++) {
		sem_post(&readyQueue_availableSpaces);
	}

	pthread_create(&consolesServerThread, NULL, consolesServer_main, NULL );
	pthread_create(&cpusServerThread, NULL, cpusServer_main, NULL );
	pthread_create(&schedulerThread, NULL, (void *) scheduler_mainFunction,
			NULL );
	pthread_create(&dispatcherThread, NULL, (void *) dispatcher_mainFunction,
			NULL );
	pthread_create(&configurationWatcherThread, NULL,
			(void*) configurationWatcherThread_mainFunction, NULL );

	showMenu();

	pthread_join(consolesServerThread, NULL );
	pthread_join(schedulerThread, NULL );
	pthread_join(dispatcherThread, NULL );

	return EXIT_SUCCESS;
}

void *consolesServer_main(void *args) {
	//TODO: Pasarle el puerto por archivo de config
	ipc_createServer("5000", consolesServerSocket_handleNewConnection,
			consolesServerSocket_handleDisconnection,
			consolesServerSocket_handleDeserializedStruct);
	return EXIT_SUCCESS;
}

void *cpusServer_main(void *args) {
	//TODO: Pasarle el puerto por archivo de config
	ipc_createServer("5001", cpusServerSocket_handleNewConnection,
			cpusServerSocket_handleDisconnection,
			cpusServerSocket_handleDeserializedStruct);
	return EXIT_SUCCESS;
}

void fetchConfiguration() {
	configuration->programsPort = config_get_int_value(__config, "PUERTO_PROG");
	configuration->cpuPort = config_get_int_value(__config, "PUERTO_CPU");
	configuration->memoryIP = config_get_string_value(__config, "IP_MEMORIA");
	configuration->memoryPort = config_get_int_value(__config,
			"PUERTO_MEMORIA");
	configuration->filesystemIP = config_get_string_value(__config, "IP_FS");
	configuration->filesystemPort = config_get_int_value(__config, "PUERTO_FS");
	configuration->quantum = config_get_int_value(__config, "QUANTUM");
	configuration->quantumSleep = config_get_int_value(__config,
			"QUANTUM_SLEEP");
	configuration->schedulingAlgorithm = ROUND_ROBIN; //TODO: Levantar string y crear enum a partir del valor
	configuration->multiprogrammingDegree = config_get_int_value(__config,
			"GRADO_MULTIPROG");
	configuration->stackSize = config_get_int_value(__config, "STACK_SIZE");
	configuration->sharedVariableNames = config_get_array_value(__config, "SHARED_VARS");
	configuration->semaphoreIDs = config_get_array_value(__config, "SEM_IDS");
	configuration->semaphoreValues = config_get_array_value(__config, "SEM_INIT");
}

///////////////////////////// Consoles server /////////////////////////////////

void consolesServerSocket_handleDeserializedStruct(int fd,
		ipc_operationIdentifier operationId, void *buffer) {
	switch (operationId) {
	case HANDSHAKE: {
		ipc_struct_handshake *handshake = buffer;
		log_info(logger, "Handshake received. Process identifier: %s",
				processName(handshake->processIdentifier));
		ipc_server_sendHandshakeResponse(fd, 1, 0);
		break;
	}
	case PROGRAM_START: {
		ipc_struct_program_start *programStart = buffer;
		char *codeString = malloc(
				sizeof(char) * (programStart->codeLength + 1));
		memcpy(codeString, programStart->code, programStart->codeLength);
		codeString[programStart->codeLength] = '\0';
		log_info(logger, "[consoles-server] Program received. Code length: %d. Code: %s",
				programStart->codeLength, codeString);
		t_PCB *newProgram = createPCBFromScript(codeString);
		newProgram->pid = ++lastPID;

		int numberOfPages = newProgram->codePages + stackSize;
		int pid = newProgram->pid;
		int total = programStart->codeLength;

		if (memory_sendInitProgram(pid, numberOfPages) == -1) {
			//FIXME: MANEJAR ERROR
			log_error(logger, "fallo el init program");
			break;
		}

		int currentPage;
		for (currentPage = 0; currentPage < newProgram->codePages; currentPage++) {
			int size;

			if (currentPage == newProgram->codePages - 1) { // la ultima pagina
				size = total - (currentPage * pageSize);
			} else {
				size = pageSize;
			}

			ipc_client_sendMemoryWrite(memory_sockfd, pid, currentPage, 0, size, codeString + currentPage * pageSize);
			ipc_struct_memory_write_response response;
			recv(memory_sockfd, &response, sizeof(ipc_struct_memory_write_response), 0);
			log_debug(logger, "ipc_struct_memory_write_response. success: %d", response.success);
		}

		ipc_sendStartProgramResponse(fd, newProgram->pid);
		t_active_console *console = malloc(sizeof(t_active_console));
		console->fd = fd;
		console->pid = newProgram->pid;
		list_add(activeConsoles, console);
		executeNewProgram(newProgram);

		break;
	}
	case PROGRAM_FINISH: {
		ipc_struct_program_finish *programFinish = buffer;
		log_info(logger, "PROGRAM_FINISH. fd: %d. pid: %d", fd, programFinish->pid);
		pthread_mutex_lock(&readyQueue_mutex);
		t_PCB *pcb = findPCB(readyQueue, programFinish->pid);
		if (pcb != NULL) {
			removePCB(readyQueue, pcb);
		}
		// todo: revisar las otras listas
		pthread_mutex_unlock(&readyQueue_mutex);
		//TODO: enviarle a la memoria para liberar los recursos
		break;
	}
	default:
		break;
	}
}

void consolesServerSocket_handleNewConnection(int fd) {
	log_info(logger, "New connection. fd: %d", fd);
}

void markCPUAsFree(int fd) {
	int i;
	t_CPUx *cpu = NULL;

	for (i = 0; i < list_size(cpusList); i++) {
		cpu = list_get(cpusList, i);

		if (cpu->fd == fd) {
			cpu->isAvailable = true;
			sem_post(&exec_spaces);
		}
	}
}

t_CPUx *getAvailableCPU() {
	int i;
	t_CPUx *cpu = NULL;

	for (i = 0; i < list_size(cpusList); i++) {
		cpu = list_get(cpusList, i);
	}

	cpu->isAvailable = false;

	return cpu;
}

void consolesServerSocket_handleDisconnection(int fd) {
	log_info(logger, "New disconnection. fd: %d", fd);

	// Si aca ya están finalizados no hago nada

	// busco todos los pcbs para esa consola
	// los finalizo y les pongo el exit code de desconexión
}

////// Heap

kernel_heap_metadata *getKernelHeapMetadata(kernel_page_assignment *assignment, int32_t size) {
	kernel_heap_metadata *metadata = malloc(sizeof(kernel_heap_metadata));
	ipc_client_sendMemoryRead(memory_sockfd, assignment->processID, assignment->processPageNumber, 0, sizeof(kernel_heap_metadata), metada);
}

///////////////////////////// CPUs server /////////////////////////////////
void cpusServerSocket_handleDeserializedStruct(int fd,
		ipc_operationIdentifier operationId, void *buffer) {
	switch (operationId) {
	case HANDSHAKE: {
		ipc_struct_handshake *handshake = buffer;
		log_info(logger, "Handshake received. Process identifier: %s",
				processName(handshake->processIdentifier));
		ipc_server_sendHandshakeResponse(fd, 1, configuration->stackSize);
		break;
	}
	case GET_SHARED_VARIABLE: {

		ipc_struct_get_shared_variable *request = buffer;
		log_info(logger, "get_shared_variable. identifier: %s", request->identifier);

		ipc_struct_get_shared_variable_response response;
		response.header.operationIdentifier = GET_SHARED_VARIABLE_RESPONSE;
		response.value = getSharedVariableValue(request->identifier);

		send(fd, &response, sizeof(ipc_struct_get_shared_variable_response), 0);
		break;
	}
	case KERNEL_ALLOC_HEAP: {
		ipc_struct_kernel_alloc_heap *request = buffer;
		log_info(logger, "KERNEL_ALLOC_HEAP. processID: %d; numberOfBytes: %d", request->processID, request->numberOfBytes);

		kernel_page_assignment *assignment = NULL;

		int i;
		for (i = 0; i < list_size(kernel_page_assignments_list); i++) {
			kernel_page_assignment *a = list_get(kernel_page_assignments_list, i);
			if (a->processID == request->processID && a->availableBytes >= (request->numberOfBytes + sizeof(kernel_heap_metadata))) {
				//FIXME: leer la memoria y analizar con los metadata
				assignment = a;
			}
		}

		if (assignment == NULL) {
			// Si es NULL, entonces no queda ninguna página de heap
			// para este proceso, que le queden disponibles X
			// bytes contiguos disponibles para almacenar lo pedido.
			// Entonces, se pide una nueva página a la memoria
			int pageNumber = memory_sendRequestMorePages(request->processID, 1);
			if (pageNumber == 0) {
				// Error de la memoria: hago rethrows
				ipc_struct_kernel_alloc_heap_response response;
				response.header.operationIdentifier = KERNEL_ALLOC_HEAP_RESPONSE;
				response.success = 0;
				response.pageNumber = -1;
				response.offset = -1;
				send(fd, &response, sizeof(ipc_struct_kernel_alloc_heap_response), 0);
				return;
			}

			// El pedido fue exitoso, generamos un registro de la asignación
			// y lo agregamos a la lista
			assignment = malloc(sizeof(kernel_page_assignment));
			assignment->processID = request->processID;
			assignment->processPageNumber = pageNumber;
			assignment->availableBytes = pageSize;

			kernel_heap_metadata *heap_metadata = malloc(sizeof(kernel_heap_metadata));
			heap_metadata->isFree = 1;
			heap_metadata->size = pageSize - sizeof(kernel_heap_metadata);
			ipc_client_sendMemoryWrite(memory_sockfd, request->processID, pageNumber, 0, sizeof(kernel_heap_metadata), heap_metadata);
			free(heap_metadata);

			list_add(kernel_page_assignments_list, assignment);
		}

		// Ya tengo la asignación indicada, ahora hay que obtener
		// un bloque libre de dicha página
		ipc_struct_kernel_alloc_heap_response response;
		response.header.operationIdentifier = KERNEL_ALLOC_HEAP_RESPONSE;
		response.success = 1;
		response.pageNumber = assignment->processPageNumber;
		response.offset = 0; //fixme

		send(fd, &response, sizeof(ipc_struct_kernel_alloc_heap_response), 0);
		break;
	}
	case PROGRAM_FINISH: {
		log_info(logger, "New program finish. fd: %d", fd);
		break;
	}
	case KERNEL_SEMAPHORE_WAIT: {
		ipc_struct_kernel_semaphore_wait *wait = buffer;
		log_info(logger, "kernel_semaphore_wait. identifier: %s", wait->identifier);
		t_PCB *waitPCB = wait->pcb;

		kernel_semaphore *sem = getSemaphoreByIdentifier(wait->identifier);
		ipc_struct_kernel_semaphore_wait_response response;
		response.header.operationIdentifier = KERNEL_SEMAPHORE_WAIT_RESPONSE;

		response.shouldBlock = kernel_semaphore_wait(sem, list_get(execList, 0)) == 0 ? 1 : 0;
		send(fd, &response, sizeof(ipc_struct_kernel_semaphore_wait_response), 0);

		if (response.shouldBlock == 1) {
			pthread_mutex_lock(&execList_mutex);
			markCPUAsFree(fd);
			list_takePCB(execList, waitPCB->pid);
			pthread_mutex_unlock(&execList_mutex);
		}
		break;
	}
	case KERNEL_SEMAPHORE_SIGNAL: {
		ipc_struct_kernel_semaphore_signal *signal = buffer;
		log_info(logger, "kernel_semaphore_signal. identifier: %s", signal->identifier);
		kernel_semaphore *sem = getSemaphoreByIdentifier(signal->identifier);
		kernel_semaphore_signal(sem, NULL);
		break;
	}
	default:
		break;
	}
}

void cpusServerSocket_handleNewConnection(int fd) {
	log_info(logger, "New connection. fd: %d", fd);

	t_CPUx *cpu = malloc(sizeof(t_CPUx));

	cpu->fd = fd;
	cpu->isAvailable = true;

	list_add(cpusList, cpu);
	sem_post(&exec_spaces);
}

void cpusServerSocket_handleDisconnection(int fd) {
	log_info(logger, "New disconnection. fd: %d", fd);

	// Si aca ya están finalizados no hago nada

	// busco todos los pcbs para esa consola
	// los finalizo y les pongo el exit code de desconexión
}

//////////////////////////////////////////////////////////////////////////////

void *scheduler_mainFunction(void) {
	while (1) {
		log_debug(logger, "[scheduler] waiting for programs in new q");
		sem_wait(&newQueue_programsCount);
		log_debug(logger, "[scheduler] waiting for spaces in ready q");
		sem_wait(&readyQueue_availableSpaces);
		pthread_mutex_lock(&readyQueue_mutex);
		pthread_mutex_lock(&newQueue_mutex);
		t_PCB *program = newQueue_popProcess();
		readyQueue_addProcess(program);
		pthread_mutex_unlock(&newQueue_mutex);
		pthread_mutex_unlock(&readyQueue_mutex);
		log_debug(logger, "[scheduler] new process <PID: %d> in ready q",
						program->pid);

		sem_post(&readyQueue_programsCount);
	}

	return 0;
}

void sendExecutePCB(int fd, t_PCB *pcb, int quantum) {
	void *pcbBuffer = pcb_serializePCB(pcb);

	ipc_header *header = malloc(sizeof(ipc_header));
	header->operationIdentifier = CPU_EXECUTE_PROGRAM;

	int pcbSize = pcb_getPCBSize(pcb);

	int totalSize = sizeof(ipc_header) + sizeof(int) + sizeof(int) + pcbSize;

	void *buffer = malloc(totalSize);
	memcpy(buffer, header, sizeof(ipc_header));
	memcpy(buffer + sizeof(ipc_header), &quantum, sizeof(int));
	memcpy(buffer + sizeof(ipc_header) + sizeof(int), &pcbSize, sizeof(int));
	memcpy(buffer + sizeof(ipc_header) + sizeof(int) + sizeof(int), pcbBuffer, pcbSize);

	send(fd, buffer, totalSize, 0);

	free(pcbBuffer);
	free(buffer);
}

void *dispatcher_mainFunction(void) {
	while (1) {
		log_debug(logger, "[dispatcher] waiting for programs in ready q");
		sem_wait(&readyQueue_programsCount);
		log_debug(logger, "[dispatcher] waiting for free cpu's");
		sem_wait(&exec_spaces);
		pthread_mutex_lock(&execList_mutex);
		pthread_mutex_lock(&readyQueue_mutex);
		t_PCB *program = readyQueue_popProcess();
		t_CPUx *availableCPU = getAvailableCPU();
		execList_addProcess(program);
		sendExecutePCB(availableCPU->fd, program, configuration->schedulingAlgorithm == ROUND_ROBIN ? configuration->quantum : -1);
		log_debug(logger,
				"[dispatcher] sent process <PID:%d> to CPU <FD:%d>. pcbSize: %d",
				program->pid, availableCPU->fd, pcb_getPCBSize(program));
		pthread_mutex_unlock(&readyQueue_mutex);
		pthread_mutex_unlock(&execList_mutex);
		sem_post(&exec_programs);
	}

	return 0;
}

void executeNewProgram(t_PCB *pcb) {
	pthread_mutex_lock(&newQueue_mutex);
	newQueue_addProcess(pcb);
	pthread_mutex_unlock(&newQueue_mutex);
	sem_post(&newQueue_programsCount);
}

void *configurationWatcherThread_mainFunction() {
	int fd = inotify_init();
	int length, i;
	char buffer[EVENT_BUF_LEN];

	if (fd == -1) {
		log_error(logger, "Error while initializing inotify");
		pthread_exit(NULL );
	}

	if (inotify_add_watch(fd, __config->path, IN_MODIFY) == -1) {
		log_error(logger, "Error adding inotify watch");
		pthread_exit(NULL );
	}

	log_debug(logger,
			"[configuration-watcher] waiting for changes in config file (path: %s)",
			__config->path);

	while (1) {
		length = read(fd, buffer, EVENT_BUF_LEN);

		if (length <= 0) {
			log_error(logger,
					"[configuration-watcher] error reading inotify event");
		}

		i = 0;
		while (i < length) {
			struct inotify_event *event = (struct inotify_event *) &buffer[i];
			if (event->mask & IN_MODIFY) {
				log_debug(logger,
						"[configuration-watcher] config file changed. reloading config");
				fetchConfiguration();
			}

			i += EVENT_SIZE+ event->len;
		}
	}

	return NULL ;
}

t_PCB *createPCBFromScript(char *script) {
	t_PCB *PCB = malloc(sizeof(t_PCB));

	// prepare PCB
	int programLength = string_length(script);
	t_metadata_program *program = metadata_desde_literal(script);

	int codePagesCount = ceil((float)programLength / (float)pageSize);
	int instructionCount = program->instrucciones_size;

	PCB->variableSize.stackArgumentCount = 0;
	PCB->variableSize.stackIndexRecordCount = 0;
	PCB->variableSize.stackVariableCount = 0;

	PCB->codePages = codePagesCount;

	PCB->variableSize.instructionCount = instructionCount;
	PCB->codeIndex = malloc(instructionCount * sizeof(t_codeIndex));
	memcpy(PCB->codeIndex,program->instrucciones_serializado,instructionCount * sizeof(t_codeIndex));

	PCB->variableSize.labelIndexSize = program->etiquetas_size;
	PCB->labelIndex = malloc(program->etiquetas_size);
	memcpy(PCB->labelIndex,program->etiquetas,program->etiquetas_size);

	PCB->ec = 0;
	PCB->filesTable = NULL;
	PCB->pc = 0;
	PCB->sp = PCB->codePages +1 * pageSize;
	PCB->stackIndex = NULL;

	return PCB;
}
