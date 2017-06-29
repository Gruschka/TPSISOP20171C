/*
 ============================================================================
 Name        : filesystem.c
 Author      : Hernan Canzonetta
 Version     :
 Copyright   : Copyright
 Description : Hello World in C, Ansi-style
 ============================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <commons/config.h>
#include <commons/log.h>

t_config *config;
t_log *logger;

int main(int argc, char **argv) {
	char *logFile = tmpnam(NULL );

	logger = log_create(logFile, "KERNEL", 1, LOG_LEVEL_DEBUG);

	if (argc < 2) {
		log_error(logger, "Falta pasar la ruta del archivo de configuracion");
		return EXIT_FAILURE;
	} else {
		config = config_create(argv[1]);
	}
	puts("Holis"); /* prints Holis */
	return EXIT_SUCCESS;
}
