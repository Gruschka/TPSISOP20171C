/*
 * test.c
 *
 *  Created on: Apr 16, 2017
 *      Author: utnso
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "cpu.h"

t_CPU myCPU;

int main(int argc, char *argv[]) {
	cpu_start(&myCPU);

	cpu_connect(&myCPU,KERNEL);

	return 0;
}
