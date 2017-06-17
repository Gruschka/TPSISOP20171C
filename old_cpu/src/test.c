/*
 * cpu.c
 *
 *  Created on: 02/10/2014
 *      Author: utnso
 */

#include <stdio.h>
#include "tcb.h"
#include "memory.h"
#include <stdlib.h>
#include "parser.h"
#include "cpu.h"
#include <string.h>
#include <commons/collections/queue.h>
#include <commons/bitarray.h>
#include <commons/config.h>
#include <commons/log.h>
#include <socket.h>
#include <estructurasPackage.h>

CPU myCPU;
Memory myMemory;
t_config *configFile;
t_log *logFile;

int main(int argc, char **argv) {

	configFile = config_create("src/cpu.config");
	char *logFileLocation = config_get_string_value(configFile,"LOG_FILE");
	// LOG_LEVEL_DEBUG Shows all but Trace
	// LOG_LEVEL_INFO Shows Error,Info,Warning
	// LOG_LEVEL_ERROR Shows error
	// LOG_LEVEL_TRACE Shows all
	// LOG_LEVEL_WARNINIG shows error,warning

	logFile = log_create(logFileLocation,"CPU",1,LOG_LEVEL_TRACE);

	if(configFile!=NULL && logFile!=NULL)log_trace(logFile,"Config and Log created");

	// connect to MSP
	myMemory = memory_create(1024);

	// create CPU
	cpu_create(&myCPU,VERBOSE);
	log_trace(logFile,"CPU Created");

	int socket = socket_crearCliente();

	if (socket == -1) {
			log_error(logFile, "Client error");
			return -1;
	}

	if (socket_conectarCliente(socket,"127.0.0.1",8001) == -1 ) {
			log_error(logFile, "Error al conectarse al kernel. Revisar que esté corriendo");
			return -1;
	}

	// Loader part
/*	char *besoScript = 0;
	int scriptSize = parser_getBesoFromFile("src/aritmetics.bc",&besoScript);
	t_TCB *aritmeticsTCB = tcb_createFromScriptWithSize(besoScript,scriptSize);
	memory_write(aritmeticsTCB->codeSegmentBase,0,scriptSize,besoScript);

	*besoScript = 0;
	scriptSize = parser_getBesoFromFile("src/flow.bc",&besoScript);
	t_TCB *flowTCB = tcb_createFromScriptWithSize(besoScript,scriptSize);
	memory_write(flowTCB->codeSegmentBase,0,scriptSize,besoScript);

	*besoScript = 0;
	scriptSize = parser_getBesoFromFile("src/cpyloop.bc",&besoScript);
	t_TCB *cpyloopTCB = tcb_createFromScriptWithSize(besoScript,scriptSize);
	memory_write(cpyloopTCB->codeSegmentBase,0,scriptSize,besoScript);

	*besoScript = 0;
	scriptSize = parser_getBesoFromFile("src/getmemory.bc",&besoScript);
	t_TCB *getmemoryTCB = tcb_createFromScriptWithSize(besoScript,scriptSize);
	memory_write(getmemoryTCB->codeSegmentBase,0,scriptSize,besoScript);

	*besoScript = 0;
	scriptSize = parser_getBesoFromFile("src/labels.bc",&besoScript);
	t_TCB *labelsTCB = tcb_createFromScriptWithSize(besoScript,scriptSize);
	memory_write(labelsTCB->codeSegmentBase,0,scriptSize,besoScript);

	*besoScript = 0;
	scriptSize = parser_getBesoFromFile("src/logic.bc",&besoScript);
	t_TCB *logicTCB = tcb_createFromScriptWithSize(besoScript,scriptSize);
	memory_write(logicTCB->codeSegmentBase,0,scriptSize,besoScript);

	*besoScript = 0;
	scriptSize = parser_getBesoFromFile("src/stack.bc",&besoScript);
	t_TCB *stackTCB = tcb_createFromScriptWithSize(besoScript,scriptSize);
	memory_write(stackTCB->codeSegmentBase,0,scriptSize,besoScript);

	*besoScript = 0;
	scriptSize = parser_getBesoFromFile("src/shifter.bc",&besoScript);
	t_TCB *shifterTCB = tcb_createFromScriptWithSize(besoScript,scriptSize);
	memory_write(shifterTCB->codeSegmentBase,0,scriptSize,besoScript);

	char *besoScript = 0;
	int scriptSize = parser_getBesoFromFile("src/bigStack.bc",&besoScript);
	t_TCB *bigStackTCB = tcb_createFromScriptWithSize(besoScript,scriptSize);
	memory_write(bigStackTCB->codeSegmentBase,0,scriptSize,besoScript);
	*/

	// kernel part
	t_queue *readyQueue = queue_create();
/*	queue_push(readyQueue,aritmeticsTCB);
	queue_push(readyQueue,flowTCB);
	queue_push(readyQueue,cpyloopTCB);
	queue_push(readyQueue,getmemoryTCB);
	queue_push(readyQueue,labelsTCB);
	queue_push(readyQueue,logicTCB);
	queue_push(readyQueue,stackTCB);
	queue_push(readyQueue,shifterTCB);
	queue_push(readyQueue,bigStackTCB);*/

	while (1) {
			log_trace(logFile, "Awaiting TCB");

			t_tipoEstructura tipoEstructura;
			void *buffer;

			// Se bloquea hasta recibir TCB
			socket_recibir(socket,&tipoEstructura,&buffer);

			t_struct_tcb *tcb = buffer;

			// Es una villereada pero funciona, el packed pesa menos que el otro
			// y no se puede castear bien
			t_TCB *tcbToExecute = malloc(sizeof(t_TCB));

			tcbToExecute->codeSegmentBase = tcb->codeSegmentBase;
			tcbToExecute->codeSegmentSize = tcb->codeSegmentSize;
			tcbToExecute->instructionPointer = tcb->instructionPointer;
			tcbToExecute->km = tcb->km;
			tcbToExecute->pid = tcb->pid;
			(tcbToExecute->registers)[0] = (tcb->registers)[0];
			(tcbToExecute->registers)[1] = (tcb->registers)[1];
			(tcbToExecute->registers)[2] = (tcb->registers)[2];
			(tcbToExecute->registers)[3] = (tcb->registers)[3];
			(tcbToExecute->registers)[4] = (tcb->registers)[4];
			tcbToExecute->stackBase = tcb->stackBase;
			tcbToExecute->stackCursor = tcb->stackCursor;
			tcbToExecute->tid = tcb->tid;

			log_trace(logFile, "Got TCB <TID:%d>",tcbToExecute->tid);

			cpu_getTCB(&myCPU,tcbToExecute);

			kernel_sendInterruption(&myCPU, 50, socket);
			// Execution cycle
			while(myCPU.status == RUNNING && myCPU.quantum > 0) {
				// 1 set registers with current pcb: done before ec
				// 2 request next instruction to msp.(FETCH)
				int fetchedInstructionSize = cpu_fetch(&myCPU);
				log_trace(logFile,"CPU Fetched new instruction");

				cpu_executeInstruction(&myCPU, myCPU.currentInstruction);
				log_trace(logFile,"CPU Executed an instruction");

				if(myCPU.status == RUNNING){ //might have executed XXXX, so it's may not be necessary to update TCB and inc.
					// 5 Increment IP
					cpu_incrementInstructionPointer(&myCPU,fetchedInstructionSize);
					log_trace(logFile,"CPU Incremented IP");

					// 4 update TCB
					cpu_updateAssignedTCB(&myCPU);

					// 6 consume quantum
					if(myCPU.assignedTCB->km == 0){
						cpu_spendQuantumUnit(&myCPU);
						log_trace(logFile,"CPU Spent a quantum unit");
					}


					if(myCPU.quantum == 0){
						// return updated TCB to kernel.
						log_trace(logFile,"CPU ran out of quantum");
						// return TCB to Kernel
						queue_push(readyQueue,myCPU.assignedTCB);
						log_trace(logFile,"CPU returned assigned TCB");

						cpu_setWaitingMode(&myCPU);
						log_trace(logFile,"CPU in Waiting mode");

					}
				}
			}
		} // Termina while(1)

	/*
	// get PCB and set context
	while(myCPU.status == WAITING ){
		t_TCB *TCBtoRun = (t_TCB *) queue_pop(readyQueue);// receive tcb from kernel
		if(TCBtoRun != NULL){
			// found new TCB to run, get context and execute.
			cpu_getTCB(&myCPU,TCBtoRun);
			log_trace(logFile,"CPU got new TCB");

		}
		//a
		// execution cycle
		while(myCPU.status == RUNNING && myCPU.quantum > 0){
			// ec steps according to tp
			// 1 set registers with current pcb: done before ec
			// 2 request next instruction to msp.(FETCH)
			int fetchedInstructionSize = cpu_fetch(&myCPU);
			log_trace(logFile,"CPU Fetched new instruction");

			// 3 execute beso instruction
			cpu_executeInstruction(&myCPU, myCPU.currentInstruction);
			log_trace(logFile,"CPU Executed an instruction");


			if(myCPU.status == RUNNING){ //might have executed XXXX, so it's may not be necessary to update TCB and inc.
				// 5 Increment IP
				cpu_incrementInstructionPointer(&myCPU,fetchedInstructionSize);
				log_trace(logFile,"CPU Incremented IP");

				// 4 update TCB
				cpu_updateAssignedTCB(&myCPU);


				// 6 consume quantum
				if(myCPU.assignedTCB->km == 0){
					cpu_spendQuantumUnit(&myCPU);
					log_trace(logFile,"CPU Spent a quantum unit");
				}


				if(myCPU.quantum == 0){
					// return updated TCB to kernel.
					log_trace(logFile,"CPU ran out of quantum");
					// return TCB to Kernel
					queue_push(readyQueue,myCPU.assignedTCB);
					log_trace(logFile,"CPU returned assigned TCB");

					cpu_setWaitingMode(&myCPU);
					log_trace(logFile,"CPU in Waiting mode");

				}
			}





		}
	}*/



	return 0;

}

