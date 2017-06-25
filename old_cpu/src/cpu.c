/*
 * variables.c
 *
 *  Created on: 27/04/2014
 *      Author: Guille
 */

#include "cpu.h"
#include <commons/tcb.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <socket.h>
//a
void cpu_create(CPU* newCPU, CPU_TALK talkType){
	newCPU->instructionPointer = 0;
	newCPU->stackBase = 0;
	newCPU->status = WAITING;
	newCPU->dirty = 0;
	newCPU->talk = talkType;
	newCPU->quantum = config_get_int_value(configFile,"PREDEFINED_QUANTUM");
	if(newCPU->talk == VERBOSE){
		printf("LOG: CPU created\n");
		cpu_dump(newCPU);
	}
}

void cpu_spendQuantumUnit(CPU *aCPU){
	aCPU->quantum -= 1;
}

void cpu_clearRegisters(CPU *aCPU){
	int iterator;
	for (iterator = 0; iterator < 5; ++iterator) {
		aCPU->registers[iterator] = 0;
	}

}

void cpu_setRunningMode(CPU *aCPU){
	aCPU->status = RUNNING;
}

void cpu_setWaitingMode(CPU *aCPU){
	aCPU->status = WAITING;
	//free(aCPU->assignedTCB);
	aCPU->stackBase= 0;
	free(aCPU->currentInstruction);
	cpu_setClean(aCPU);
	aCPU->instructionPointer = 0;
	aCPU->quantum = PREDEFINED_QUANTUM;
	aCPU->codeBase = 0;
	aCPU->stackCursor = 0;
	cpu_clearRegisters(aCPU);

}

void cpu_getTCB(CPU *aCPU,t_TCB *aTCB){
	aCPU->stackBase = aTCB->stackBase;
	aCPU->stackCursor = aTCB->stackCursor;
	aCPU->instructionPointer = aTCB->instructionPointer;

	aCPU->assignedTCB = aTCB;

	cpu_getRegistersFromTCB(aCPU);

	cpu_setRunningMode(aCPU);

/*	if(aCPU->talk == VERBOSE){
		cpu_printAssignedTCB(aCPU);
	}*/
}

int cpu_parameterSizeFromInstruction(char *instruction){
	char *buffer = malloc(sizeof(int)+sizeof(char));
	memcpy(buffer,instruction,sizeof(int));
	buffer[sizeof(int)] = 0;

	if(!strcmp(buffer,"LOAD")){
		free(buffer);
		return (sizeof(char) + sizeof(int));
	}

	if(!strcmp(buffer,"MULR")){
		free(buffer);
		return (sizeof(char) + sizeof(char));
	}

	if(!strcmp(buffer,"ADDR")){
		free(buffer);
		return (sizeof(char) + sizeof(char));
	}

	if(!strcmp(buffer,"DECR")){
		free(buffer);
		return (sizeof(char));
	}

	if(!strcmp(buffer,"DIVR")){
		free(buffer);
		return (sizeof(char) + sizeof(char));
	}

	if(!strcmp(buffer,"MODR")){
		free(buffer);
		return (sizeof(char) + sizeof(char));
	}

	if(!strcmp(buffer,"SUBR")){
		free(buffer);
		return (sizeof(char) + sizeof(char));
	}

	if(!strcmp(buffer,"INCR")){
		free(buffer);
		return (sizeof(char));
	}

	if(!strcmp(buffer,"XXXX")){
		free(buffer);
		return 0;
	}

	if(!strcmp(buffer,"GETM")){
		free(buffer);
		return sizeof(char) + sizeof(char);
	}

	if(!strcmp(buffer,"SETM")){
		free(buffer);
		return sizeof(char) + sizeof(char) + sizeof(int);
	}

	if(!strcmp(buffer,"MOVR")){
		free(buffer);
		return sizeof(char) + sizeof(char);
	}

	if(!strcmp(buffer,"COMP")){
		free(buffer);
		return sizeof(char) + sizeof(char);
	}

	if(!strcmp(buffer,"CGEQ")){
		free(buffer);
		return sizeof(char) + sizeof(char);
	}

	if(!strcmp(buffer,"CLEQ")){
		free(buffer);
		return sizeof(char) + sizeof(char);
	}

	if(!strcmp(buffer,"GOTO")){
		free(buffer);
		return sizeof(char);
	}

	if(!strcmp(buffer,"JMPZ")){
		free(buffer);
		return sizeof(int);
	}

	if(!strcmp(buffer,"JPNZ")){
		free(buffer);
		return sizeof(int);
	}

	if(!strcmp(buffer,"INTE")){
		free(buffer);
		return sizeof(int);
	}

	if(!strcmp(buffer,"FLCL")){
		free(buffer);
		return 0;
	}

	if(!strcmp(buffer,"SHIF")){
		free(buffer);
		return sizeof(int) + sizeof(char);
	}

	if(!strcmp(buffer,"NOPP")){
		free(buffer);
		return 0;
	}

	if(!strcmp(buffer,"PUSH")){
		free(buffer);
		return sizeof(int) + sizeof(char);
	}

	if(!strcmp(buffer,"TAKE")){
		free(buffer);
		return sizeof(int) + sizeof(char);
	}

	if(!strcmp(buffer,"INTE")){
		free(buffer);
		return sizeof(int);
	}
	return 0;
}

void cpu_getRegistersFromTCB(CPU *aCPU){
	int iterator;
	if(aCPU->assignedTCB != NULL){
		for (iterator = 0; iterator < 5; ++iterator) {
			aCPU->registers[iterator] = aCPU->assignedTCB->registers[iterator];
		}
	}
	aCPU->codeBase = aCPU->assignedTCB->codeSegmentBase; //M
	aCPU->instructionPointer = aCPU->assignedTCB->instructionPointer; //P
	aCPU->stackBase = aCPU->assignedTCB->stackBase; //X
	aCPU->stackCursor = aCPU->assignedTCB->stackCursor; //S

}

int cpu_fetch(CPU *aCPU){
	char *instructionName;
	char *parameters;

	instructionName = memory_read(aCPU->instructionPointer,0,sizeof(int));
	int parameterSize = cpu_parameterSizeFromInstruction(instructionName);

	parameters = memory_read(aCPU->instructionPointer,sizeof(int),parameterSize);

	char *result = malloc(sizeof(int) + parameterSize);

	memcpy(result,instructionName,sizeof(int));
	memcpy(result + sizeof(int),parameters,parameterSize);

	free(instructionName);
	free(parameters);

	aCPU->currentInstruction = result;

	return sizeof(int) + parameterSize;

}

void cpu_incrementInstructionPointer(CPU *aCPU,int offset){
	aCPU->instructionPointer += offset;
}

int cpu_setValueInRegister(CPU *aCPU, int value, char *toRegister){
	char *buffer = malloc(sizeof(char) *2);
	memcpy(buffer,toRegister,sizeof(char));
	buffer[sizeof(char)] = 0;

	if(!strcmp(buffer,"A")){
		aCPU->registers[0] = value;
		free(buffer);
		return value;
	}
	if(!strcmp(buffer,"B")){
		aCPU->registers[1] = value;
		free(buffer);
		return value;
	}
	if(!strcmp(buffer,"C")){
		aCPU->registers[2] = value;
		free(buffer);
		return value;
	}
	if(!strcmp(buffer,"D")){
		aCPU->registers[3] = value;
		free(buffer);
		return value;
	}
	if(!strcmp(buffer,"E")){
		aCPU->registers[4] = value;
		free(buffer);
		return value;
	}
	if(!strcmp(buffer,"M")){
		aCPU->codeBase = value;
		free(buffer);
		return value;
	}
	if(!strcmp(buffer,"P")){
		aCPU->instructionPointer = value;
		free(buffer);
		return value;
	}
	if(!strcmp(buffer,"X")){
		aCPU->stackBase = value;
		free(buffer);
		return value;
	}
	if(!strcmp(buffer,"S")){
		aCPU->stackCursor = value;
		free(buffer);
		return value;
	}

	return -1;
}

void cpu_incrementStackCursor(CPU *aCPU, int offset){
	aCPU->stackCursor += offset;
}

void cpu_decrementStackCursor(CPU *aCPU, int offset){
	aCPU->stackCursor -= offset;
}

int cpu_executeInstruction(CPU *aCPU, char *instruction){
	sleep((config_get_int_value(configFile,"RETARDO"))/1000);

	char *buffer = malloc(sizeof(int)+sizeof(char));
	memcpy(buffer,instruction,sizeof(int));
	buffer[sizeof(int)] = 0;


	if(!strcmp(buffer,"LOAD")){
		char *toRegister = malloc(sizeof(char));
		int *value = malloc(sizeof(int));
		memcpy(toRegister,instruction+sizeof(int),sizeof(char));
		memcpy(value,instruction+sizeof(int)+sizeof(char),sizeof(int));

		cpu_setValueInRegister(aCPU,*value,toRegister);
		cpu_setDirty(aCPU);

		free(buffer);
		free(toRegister);
		free(value);
		log_info(logFile,"CPU executed LOAD");
		return 0;
	}

	if(!strcmp(buffer,"MULR")){
		char *firstRegister = malloc(sizeof(char));
		char *secondRegister = malloc(sizeof(char));
		memcpy(firstRegister,instruction+sizeof(int),sizeof(char));
		memcpy(secondRegister,instruction+sizeof(int)+sizeof(char),sizeof(char));

		int firstRegisterValue = cpu_getValueFromRegister(aCPU,firstRegister);
		int secondRegisterValue = cpu_getValueFromRegister(aCPU,secondRegister);
		int result = firstRegisterValue * secondRegisterValue;

		cpu_setValueInRegister(aCPU,result,"A");
		cpu_setDirty(aCPU);

		free(firstRegister);
		free(secondRegister);
		free(buffer);
		log_info(logFile,"CPU executed MULR");
		return 0;
	}

	if(!strcmp(buffer,"ADDR")){
		char *firstRegister = malloc(sizeof(char));
		char *secondRegister = malloc(sizeof(char));
		memcpy(firstRegister,instruction+sizeof(int),sizeof(char));
		memcpy(secondRegister,instruction+sizeof(int)+sizeof(char),sizeof(char));

		int firstRegisterValue = cpu_getValueFromRegister(aCPU,firstRegister);
		int secondRegisterValue = cpu_getValueFromRegister(aCPU,secondRegister);
		int result = firstRegisterValue + secondRegisterValue;

		cpu_setValueInRegister(aCPU,result,"A");
		cpu_setDirty(aCPU);

		free(firstRegister);
		free(secondRegister);
		free(buffer);
		log_info(logFile,"CPU executed ADDR");
		return 0;
	}

	if(!strcmp(buffer,"DECR")){
		char *firstRegister = malloc(sizeof(char));
		memcpy(firstRegister,instruction+sizeof(int),sizeof(char));

		int registerValue = cpu_getValueFromRegister(aCPU,firstRegister);
		int result = registerValue - 1 ;

		cpu_setValueInRegister(aCPU,result,"A");
		cpu_setDirty(aCPU);

		free(firstRegister);
		free(buffer);
		log_info(logFile,"CPU executed DECR");
		return 0;
	}

	if(!strcmp(buffer,"DIVR")){
		char *firstRegister = malloc(sizeof(char));
		char *secondRegister = malloc(sizeof(char));
		memcpy(firstRegister,instruction+sizeof(int),sizeof(char));
		memcpy(secondRegister,instruction+sizeof(int)+sizeof(char),sizeof(char));

		int firstRegisterValue = cpu_getValueFromRegister(aCPU,firstRegister);
		int secondRegisterValue = cpu_getValueFromRegister(aCPU,secondRegister);

		if(secondRegisterValue != 0){
			int result = firstRegisterValue / secondRegisterValue;

			cpu_setValueInRegister(aCPU,result,"A");
			cpu_setDirty(aCPU);
		}else{
			printf("ERROR: ZERO_DIV");
		}


		free(firstRegister);
		free(secondRegister);
		free(buffer);
		log_info(logFile,"CPU executed DIVR");
		return 0;
	}

	if(!strcmp(buffer,"MODR")){
		char *firstRegister = malloc(sizeof(char));
		char *secondRegister = malloc(sizeof(char));
		memcpy(firstRegister,instruction+sizeof(int),sizeof(char));
		memcpy(secondRegister,instruction+sizeof(int)+sizeof(char),sizeof(char));

		int firstRegisterValue = cpu_getValueFromRegister(aCPU,firstRegister);
		int secondRegisterValue = cpu_getValueFromRegister(aCPU,secondRegister);

		if(secondRegisterValue != 0){
			int result = firstRegisterValue % secondRegisterValue;

			cpu_setValueInRegister(aCPU,result,"A");
			cpu_setDirty(aCPU);
		}else{
			printf("ERROR: ZERO_DIV");
		}


		free(firstRegister);
		free(secondRegister);
		free(buffer);
		log_info(logFile,"CPU executed MODR");
		return 0;
	}

	if(!strcmp(buffer,"SUBR")){
		char *firstRegister = malloc(sizeof(char));
		char *secondRegister = malloc(sizeof(char));
		memcpy(firstRegister,instruction+sizeof(int),sizeof(char));
		memcpy(secondRegister,instruction+sizeof(int)+sizeof(char),sizeof(char));

		int firstRegisterValue = cpu_getValueFromRegister(aCPU,firstRegister);
		int secondRegisterValue = cpu_getValueFromRegister(aCPU,secondRegister);
		int result = firstRegisterValue - secondRegisterValue;

		cpu_setValueInRegister(aCPU,result,"A");
		cpu_setDirty(aCPU);

		free(firstRegister);
		free(secondRegister);
		free(buffer);
		log_info(logFile,"CPU executed SUBR");
		return 0;
	}

	if(!strcmp(buffer,"INCR")){
		char *firstRegister = malloc(sizeof(char));
		memcpy(firstRegister,instruction+sizeof(int),sizeof(char));

		int registerValue = cpu_getValueFromRegister(aCPU,firstRegister);
		int result = registerValue + 1;

		cpu_setValueInRegister(aCPU,result,firstRegister);
		cpu_setDirty(aCPU);

		free(firstRegister);
		free(buffer);
		log_info(logFile,"CPU executed INCR");
		return 0;
	}

	if(!strcmp(buffer,"XXXX")){
		if(aCPU->talk == VERBOSE) cpu_printAssignedTCB(aCPU);
		cpu_setWaitingMode(aCPU);
		// todo return to kernel
		log_info(logFile,"CPU executed XXXX");
		return 0;
	}

	if(!strcmp(buffer,"GETM")){
		char *firstRegister = malloc(sizeof(char));
		char *secondRegister = malloc(sizeof(char));
		memcpy(firstRegister,instruction+sizeof(int),sizeof(char));
		memcpy(secondRegister,instruction+sizeof(int)+sizeof(char),sizeof(char));

		int secondRegisterValue = cpu_getValueFromRegister(aCPU,secondRegister);

		char *value = memory_read(secondRegisterValue,0,sizeof(char));

		cpu_setValueInRegister(aCPU,(int)*value,firstRegister);
		cpu_setDirty(aCPU);

		free(firstRegister);
		free(secondRegister);
		free(value);
		free(buffer);
		log_info(logFile,"CPU executed GETM");
		return 0;
	}

	if(!strcmp(buffer,"SETM")){
		int *number = malloc(sizeof(int));
		char *firstRegister = malloc(sizeof(char));
		char *secondRegister = malloc(sizeof(char));
		memcpy(number,instruction+sizeof(int),sizeof(int));
		memcpy(firstRegister,instruction+sizeof(int)+sizeof(int),sizeof(char));
		memcpy(secondRegister,instruction+(2*sizeof(int))+sizeof(char),sizeof(char));

		int firstRegisterValue = cpu_getValueFromRegister(aCPU,firstRegister);

		memory_write(firstRegisterValue,0,*number,secondRegister);

		cpu_setDirty(aCPU);

		free(firstRegister);
		free(number);
		free(secondRegister);
		free(buffer);
		log_info(logFile,"CPU executed SETM");
		return 0;
	}

	if(!strcmp(buffer,"MOVR")){
		char *firstRegister = malloc(sizeof(char));
		char *secondRegister = malloc(sizeof(char));
		memcpy(firstRegister,instruction+sizeof(int),sizeof(char));
		memcpy(secondRegister,instruction+sizeof(int)+sizeof(char),sizeof(char));

		int secondRegisterValue = cpu_getValueFromRegister(aCPU,secondRegister);

		cpu_setValueInRegister(aCPU,secondRegisterValue,firstRegister);
		cpu_setDirty(aCPU);

		free(firstRegister);
		free(secondRegister);
		free(buffer);
		log_info(logFile,"CPU executed MOVR");
		return 0;
	}

	if(!strcmp(buffer,"COMP")){
		char *firstRegister = malloc(sizeof(char));
		char *secondRegister = malloc(sizeof(char));
		memcpy(firstRegister,instruction+sizeof(int),sizeof(char));
		memcpy(secondRegister,instruction+sizeof(int)+sizeof(char),sizeof(char));

		int firstRegisterValue = cpu_getValueFromRegister(aCPU,firstRegister);
		int secondRegisterValue = cpu_getValueFromRegister(aCPU,secondRegister);

		if(firstRegisterValue == secondRegisterValue){
			cpu_setValueInRegister(aCPU,1,"A");
		}else{
			cpu_setValueInRegister(aCPU,0,"A");
		}

		cpu_setDirty(aCPU);

		free(firstRegister);
		free(secondRegister);
		free(buffer);
		log_info(logFile,"CPU executed COMP");
		return 0;
	}

	if(!strcmp(buffer,"CGEQ")){
		char *firstRegister = malloc(sizeof(char));
		char *secondRegister = malloc(sizeof(char));
		memcpy(firstRegister,instruction+sizeof(int),sizeof(char));
		memcpy(secondRegister,instruction+sizeof(int)+sizeof(char),sizeof(char));

		int firstRegisterValue = cpu_getValueFromRegister(aCPU,firstRegister);
		int secondRegisterValue = cpu_getValueFromRegister(aCPU,secondRegister);

		if(firstRegisterValue >= secondRegisterValue){
			cpu_setValueInRegister(aCPU,1,"A");
		}else{
			cpu_setValueInRegister(aCPU,0,"A");
		}

		cpu_setDirty(aCPU);

		free(firstRegister);
		free(secondRegister);
		free(buffer);
		log_info(logFile,"CPU executed CGEQ");
		return 0;
	}

	if(!strcmp(buffer,"CLEQ")){
		char *firstRegister = malloc(sizeof(char));
		char *secondRegister = malloc(sizeof(char));
		memcpy(firstRegister,instruction+sizeof(int),sizeof(char));
		memcpy(secondRegister,instruction+sizeof(int)+sizeof(char),sizeof(char));

		int firstRegisterValue = cpu_getValueFromRegister(aCPU,firstRegister);
		int secondRegisterValue = cpu_getValueFromRegister(aCPU,secondRegister);

		if(firstRegisterValue <= secondRegisterValue){
			cpu_setValueInRegister(aCPU,1,"A");
		}else{
			cpu_setValueInRegister(aCPU,0,"A");
		}

		cpu_setDirty(aCPU);

		free(firstRegister);
		free(secondRegister);
		free(buffer);
		log_info(logFile,"CPU executed CLEQ");
		return 0;
	}

	if(!strcmp(buffer,"GOTO")){
		char *firstRegister = malloc(sizeof(char));
		memcpy(firstRegister,instruction+sizeof(int),sizeof(char));

		int firstRegisterValue = cpu_getValueFromRegister(aCPU,firstRegister);

		aCPU->instructionPointer = aCPU->assignedTCB->codeSegmentBase + firstRegisterValue;

		// substract GOTO size, since IP will be incremented in the EC.
		aCPU->instructionPointer -= sizeof(int) + sizeof(char);

		cpu_setDirty(aCPU);

		free(firstRegister);
		free(buffer);
		log_info(logFile,"CPU executed GOTO");
		return 0;
	}

	if(!strcmp(buffer,"JMPZ")){
		int *number = malloc(sizeof(int));
		memcpy(number,instruction+sizeof(int),sizeof(int));

		int registerAValue = cpu_getValueFromRegister(aCPU,"A");

		if(registerAValue == 0){
			aCPU->instructionPointer =  aCPU->codeBase + (*number);
			aCPU->instructionPointer -= 2 * sizeof(int);
		}

		cpu_setDirty(aCPU);

		free(number);
		free(buffer);
		log_info(logFile,"CPU executed JMPZ");
		return 0;
	}

	if(!strcmp(buffer,"JPNZ")){
		int *number = malloc(sizeof(int));
		memcpy(number,instruction+sizeof(int),sizeof(int));

		int registerAValue = cpu_getValueFromRegister(aCPU,"A");

		if(registerAValue != 0){
			aCPU->instructionPointer = aCPU->codeBase + (*number);
			aCPU->instructionPointer -= 2 * sizeof(int);
		}

		cpu_setDirty(aCPU);

		free(number);
		free(buffer);
		log_info(logFile,"CPU executed JPNZ");
		return 0;
	}

	if(!strcmp(buffer,"INTE")){
		int *address = malloc(sizeof(int));
		memcpy(address,instruction+sizeof(int),sizeof(int));

		// ToDo: Ver cómo traigo el nro de socket acá
		//kernel_sendInterruption(aCPU, *address,socket);

		cpu_setWaitingMode(aCPU);

		free(address);
		free(buffer);
		log_info(logFile,"CPU executed INTE");
		return 0;
	}

/*	if(!strcmp(buffer,"FLCL")){


		// todo: implementation

		cpu_setDirty(aCPU);

		free(buffer);
		return 0;
	}*/

	if(!strcmp(buffer,"SHIF")){
		int *number = malloc(sizeof(int));
		char *aRegister = malloc(sizeof(char));
		memcpy(number,instruction+sizeof(int),sizeof(int));
		memcpy(aRegister,instruction+sizeof(int)+sizeof(int),sizeof(char));

		int aRegisterValue = cpu_getValueFromRegister(aCPU,aRegister);

		int shiftedRegisterValue = aRegisterValue >> *number;

		cpu_setValueInRegister(aCPU,shiftedRegisterValue,aRegister);

		cpu_setDirty(aCPU);

		free(number);
		free(aRegister);
		free(buffer);
		log_info(logFile,"CPU executed SHIF");
		return 0;
	}

	if(!strcmp(buffer,"NOPP")){
		log_info(logFile,"CPU executed NOPP");
		return 0;
	}

	if(!strcmp(buffer,"PUSH")){
		int *number = malloc(sizeof(int));
		char *aRegister = malloc(sizeof(char));
		memcpy(number,instruction+sizeof(int),sizeof(int));
		memcpy(aRegister,instruction+sizeof(int)+sizeof(int),sizeof(char));

		int aRegisterValue = cpu_getValueFromRegister(aCPU,aRegister);

		memory_write(aCPU->stackCursor,0,*number,&aRegisterValue);

		cpu_incrementStackCursor(aCPU,*number);

		cpu_setDirty(aCPU);

		free(number);
		free(aRegister);
		free(buffer);
		log_info(logFile,"CPU executed PUSH");
		return 0;
	}

	if(!strcmp(buffer,"TAKE")){
		int *number = malloc(sizeof(int));
		char *aRegister = malloc(sizeof(char));
		memcpy(number,instruction+sizeof(int),sizeof(int));
		memcpy(aRegister,instruction+sizeof(int)+sizeof(int),sizeof(char));

		int *stackValue = (int *) memory_read(aCPU->stackCursor - (*number),0,*number);

		cpu_setValueInRegister(aCPU,*stackValue,aRegister);

		cpu_decrementStackCursor(aCPU,*number);

		cpu_setDirty(aCPU);

		free(number);
		free(aRegister);
		free(buffer);
		log_info(logFile,"CPU executed TAKE");
		return 0;
	}

	// protected

	if(!strcmp(buffer,"MALC")){
		if(aCPU->assignedTCB->km == '0'){
			cpu_abortWithError(aCPU,"Non Kernel TCB tried to execute protected instruction, CPU will ABORT");
			//todo return tcb with error to kernel
			cpu_setWaitingMode(aCPU);
			return 0;
		}
		//todo request segment to msp
		int address = memory_requestSegment(cpu_getValueFromRegister(aCPU,"A"));

		cpu_setValueInRegister(aCPU,address,"A");

		cpu_setDirty(aCPU);

		log_info(logFile,"CPU executed MALC");
		return 0;
	}

	if(!strcmp(buffer,"FREE")){
		if(aCPU->assignedTCB->km == '0'){
			cpu_abortWithError(aCPU,"Non Kernel TCB tried to execute protected instruction, CPU will ABORT");
			//todo return tcb with error to kernel
			cpu_setWaitingMode(aCPU);
			return 0;
		}
		//todo request memory to destroy segment pointed by A register

		log_info(logFile,"CPU executed FREE");
		return 0;
	}

	if(!strcmp(buffer,"INNN")){
		if(aCPU->assignedTCB->km == '0'){
			cpu_abortWithError(aCPU,"Non Kernel TCB tried to execute protected instruction, CPU will ABORT");
			//todo return tcb with error to kernel
			cpu_setWaitingMode(aCPU);
			return 0;
		}
		//todo request kernel keyboard input (number)

		int keyboardEnteredNumber = 4; // hardcoded for dummy purposes

		cpu_setValueInRegister(aCPU,keyboardEnteredNumber,"A");

		cpu_setDirty(aCPU);

		log_info(logFile,"CPU executed INNN");
		return 0;
	}

	if(!strcmp(buffer,"INNC")){
		if(aCPU->assignedTCB->km == '0'){
			cpu_abortWithError(aCPU,"Non Kernel TCB tried to execute protected instruction, CPU will ABORT");
			//todo return tcb with error to kernel
			cpu_setWaitingMode(aCPU);
			return 0;
		}
		//todo request kernel keyboard input (string shorter than B register)

		int bRegisterValue = cpu_getValueFromRegister(aCPU,"B");

		char *keyboardEnteredString = "HardcodedString"; // hardcoded for dummy purposes

		int aRegisterValue = cpu_getValueFromRegister(aCPU,"A");

		//todo write string into msp, to address stored in A register
		// it is not specified if we need to use CPU instructions
		// so I just use the CPU<->MSP Interface
		memory_write(aRegisterValue,0,bRegisterValue,keyboardEnteredString);

		log_info(logFile,"CPU executed INNC");
		return 0;
	}

	if(!strcmp(buffer,"OUTN")){
		if(aCPU->assignedTCB->km == '0'){
			cpu_abortWithError(aCPU,"Non Kernel TCB tried to execute protected instruction, CPU will ABORT");
			//todo return tcb with error to kernel
			cpu_setWaitingMode(aCPU);
			return 0;
		}
		//todo send A register value to kernel for console printing
		int aRegisterValue = cpu_getValueFromRegister(aCPU,"A");

		log_info(logFile,"CPU executed OUTN");
		return 0;
	}

	if(!strcmp(buffer,"OUTC")){
		if(aCPU->assignedTCB->km == '0'){
			cpu_abortWithError(aCPU,"Non Kernel TCB tried to execute protected instruction, CPU will ABORT");
			//todo return tcb with error to kernel
			cpu_setWaitingMode(aCPU);
			return 0;
		}
		int aRegisterValue = cpu_getValueFromRegister(aCPU,"A");
		int bRegisterValue = cpu_getValueFromRegister(aCPU,"B");

		//todo read string size B, from msp located in A
		char *readString = memory_read(aRegisterValue,0,bRegisterValue);

		//todo send A register value to kernel for console printing

		free(readString);

		log_info(logFile,"CPU executed OUTC");
		return 0;
	}

	if(!strcmp(buffer,"OUTC")){
		if(aCPU->assignedTCB->km == '0'){
			cpu_abortWithError(aCPU,"Non Kernel TCB tried to execute protected instruction, CPU will ABORT");
			//todo return tcb with error to kernel
			cpu_setWaitingMode(aCPU);
			return 0;
		}
		int aRegisterValue = cpu_getValueFromRegister(aCPU,"A");
		int bRegisterValue = cpu_getValueFromRegister(aCPU,"B");

		//todo read string size B, from msp located in A
		char *readString = memory_read(aRegisterValue,0,bRegisterValue);

		//todo send A register value to kernel for console printing

		free(readString);

		log_info(logFile,"CPU executed OUTC");
		return 0;
	}

	if(!strcmp(buffer,"CREA")){
		if(aCPU->assignedTCB->km == '0'){
			cpu_abortWithError(aCPU,"Non Kernel TCB tried to execute protected instruction, CPU will ABORT");
			//todo return tcb with error to kernel
			cpu_setWaitingMode(aCPU);
			return 0;
		}
		t_TCB *newTCB = malloc(sizeof(t_TCB));

		int bRegisterValue = cpu_getValueFromRegister(aCPU,"B");
		int aRegisterValue = cpu_getValueFromRegister(aCPU,"A");

		newTCB->pid = aCPU->assignedTCB->pid;
		newTCB->instructionPointer = bRegisterValue;
		newTCB->tid = aRegisterValue;
		newTCB->km = '0';
		tcb_clearRegisters(newTCB);

		// todo duplicate stack ??

		// todo invoke kernel service (send new tcb)

		free(newTCB);
		log_info(logFile,"CPU executed CREA");
		return 0;
	}

	if(!strcmp(buffer,"JOIN")){
		if(aCPU->assignedTCB->km == '0'){
			cpu_abortWithError(aCPU,"Non Kernel TCB tried to execute protected instruction, CPU will ABORT");
			//todo return tcb with error to kernel
			cpu_setWaitingMode(aCPU);
			return 0;
		}

		int aRegisterValue = cpu_getValueFromRegister(aCPU,"A");

		// todo send current TCB to Blocked until TCB with TDI aRegisterValue
		// ends
		printf("%d",aRegisterValue); // to avoid warning

		cpu_setWaitingMode(aCPU);

		log_info(logFile,"CPU executed JOIN");
		return 0;
	}

	if(!strcmp(buffer,"BLOK")){
		if(aCPU->assignedTCB->km == '0'){
			cpu_abortWithError(aCPU,"Non Kernel TCB tried to execute protected instruction, CPU will ABORT");
			//todo return tcb with error to kernel
			cpu_setWaitingMode(aCPU);
			return 0;
		}
		//todo implement

		log_info(logFile,"CPU executed BLOK");
		return 0;
	}

	return 0;

	if(!strcmp(buffer,"WAKE")){
		if(aCPU->assignedTCB->km == '0'){
			cpu_abortWithError(aCPU,"Non Kernel TCB tried to execute protected instruction, CPU will ABORT");
			//todo return tcb with error to kernel
			cpu_setWaitingMode(aCPU);
			return 0;
		}
		//todo implement

		log_info(logFile,"CPU executed WAKE");
		return 0;
	}

	return 0;
}

void cpu_updateAssignedTCB(CPU *aCPU){
	if(aCPU->dirty){
		cpu_setRegistersToTCB(aCPU);
		aCPU->assignedTCB->instructionPointer = aCPU->instructionPointer;
		aCPU->assignedTCB->stackCursor = aCPU->stackCursor;
		log_trace(logFile,"CPU Updated assigned TCB");

	}else{
		log_trace(logFile,"CPU Did not update assigned TCB");
	}

	cpu_setClean(aCPU);

}

void cpu_setRegistersToTCB(CPU *aCPU){
	int iterator;
	if(aCPU->assignedTCB != NULL){
		for (iterator = 0; iterator < 5; ++iterator) {
			aCPU->assignedTCB->registers[iterator] = aCPU->registers[iterator];
		}
	}
}

void cpu_setDirty(CPU *aCPU){
	aCPU->dirty = 1;
}

void cpu_setClean(CPU *aCPU){
	aCPU->dirty = 0;
}

int cpu_getValueFromRegister(CPU *aCPU, char *aRegister){
	char *buffer = malloc(sizeof(char) *2);
	memcpy(buffer,aRegister,sizeof(char));
	buffer[sizeof(char)] = 0;

	if(!strcmp(buffer,"A")){
		return aCPU->registers[0];
	}
	if(!strcmp(buffer,"B")){
		return aCPU->registers[1];
	}
	if(!strcmp(buffer,"C")){
		return aCPU->registers[2];
	}
	if(!strcmp(buffer,"D")){
		return aCPU->registers[3];
	}
	if(!strcmp(buffer,"E")){
		return aCPU->registers[4];
	}
	if(!strcmp(buffer,"M")){
		return aCPU->codeBase;
	}
	if(!strcmp(buffer,"P")){
		return aCPU->instructionPointer;
	}
	if(!strcmp(buffer,"X")){
		return aCPU->stackBase;
	}
	if(!strcmp(buffer,"S")){
		return aCPU->stackCursor;
	}

	return -1;

}

void cpu_abortWithError(CPU *aCPU, char *error){
	log_error(logFile,error);
}

void cpu_dump(CPU *aCPU){
	printf("CPU DUMP\n");
	printf("==========\n");
	printf("CPU STATUS:  %d\n",aCPU->status);
	printf("CPU DIRTY:  %d\n", aCPU->dirty);
	printf("CPU STACK BASE:  %d\n", aCPU->stackBase);
	printf("CPU STACK CURSOR:  %d\n", aCPU->stackCursor);
	printf("CPU IP:  %d\n", aCPU->instructionPointer);
	printf("CPU REM_QUANTUM: %d\n", aCPU->quantum);
	printf("CPU REGISTERS\n");
	printf("=============\n");
	printf("CPU A: %d\n", aCPU->registers[0]);
	printf("CPU B: %d\n", aCPU->registers[1]);
	printf("CPU C: %d\n", aCPU->registers[2]);
	printf("CPU D: %d\n", aCPU->registers[3]);
	printf("CPU E: %d\n", aCPU->registers[4]);





}

void cpu_printAssignedTCB(CPU *aCPU){
	printf("ASSIGNED TCB\n");
	printf("============\n");
	printf("PID: %d\n",aCPU->assignedTCB->pid);
	printf("TID: %d\n",aCPU->assignedTCB->tid);
	printf("KM: %c\n",aCPU->assignedTCB->km);
	printf("IP: %d\n",aCPU->assignedTCB->instructionPointer);
	printf("ASSIGNED TCB SEGMENTS\n");
	printf("=====================\n");
	printf("CODE BASE: %d\n",aCPU->assignedTCB->codeSegmentBase);
	printf("CODE SIZE: %d\n",aCPU->assignedTCB->codeSegmentSize);
	printf("STACK BASE: %d\n",aCPU->assignedTCB->stackBase);
	printf("STACK CURSOR: %d\n",aCPU->assignedTCB->stackCursor);
	printf("ASSIGNED TCB REGISTERS\n");
	printf("=====================\n");
	printf("CPU A: %d\n", aCPU->assignedTCB->registers[0]);
	printf("CPU B: %d\n", aCPU->assignedTCB->registers[1]);
	printf("CPU C: %d\n", aCPU->assignedTCB->registers[2]);
	printf("CPU D: %d\n", aCPU->assignedTCB->registers[3]);
	printf("CPU E: %d\n\n", aCPU->assignedTCB->registers[4]);
}

void kernel_sendInterruption(CPU *cpu, int address, int socket) {
	//extern int socket; //Definido en test.c

	t_struct_interruption *interruptionToSend = malloc(sizeof(t_struct_interruption));
	t_struct_tcb *tcbToSend = malloc(sizeof(t_struct_tcb));

	tcbToSend->codeSegmentBase = cpu->assignedTCB->codeSegmentBase;
	tcbToSend->codeSegmentSize = cpu->assignedTCB->codeSegmentSize;
	tcbToSend->instructionPointer = cpu->assignedTCB->instructionPointer;
	tcbToSend->km = cpu->assignedTCB->km;
	tcbToSend->pid = cpu->assignedTCB->pid;
	(tcbToSend->registers)[0] = (cpu->assignedTCB->registers)[0];
	(tcbToSend->registers)[1] = (cpu->assignedTCB->registers)[1];
	(tcbToSend->registers)[2] = (cpu->assignedTCB->registers)[2];
	(tcbToSend->registers)[3] = (cpu->assignedTCB->registers)[3];
	(tcbToSend->registers)[4] = (cpu->assignedTCB->registers)[4];
	tcbToSend->stackBase = cpu->assignedTCB->stackBase;
	tcbToSend->stackCursor = cpu->assignedTCB->stackCursor;
	tcbToSend->tid = cpu->assignedTCB->tid;

	interruptionToSend->tcb = tcbToSend;
	interruptionToSend->address = address;

	socket_enviar(socket, D_STRUCT_INTERRUPTION, interruptionToSend);
}
