#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <commons/log.h>
#include "utils.h"
#include <commons/config.h>
t_config *config;

int main(int argc, char *argv[])
{
	config = config_create("memoria.config");
	RETARDO_RESPUESTA = config_get_int_value(config, "RETARDO_RESPUESTA");
	sems_espera_creacion_codigos = list_create();

	dictionary_codigos = dictionary_create();

	logger = log_create("Memoria.log", "Servidor", 1, LOG_LEVEL_DEBUG);
	int server_fd = iniciar_servidor(PUERTO_MEMORIA, logger);
	if (server_fd == -1)
	{
		log_error(logger, "No se pudo iniciar el servidor.");
		return EXIT_FAILURE;
	}
	log_info(logger, "Memoria lista para recibir");
	esperar_cliente(server_fd, client_handler);
	// habría que ver cuando se termina el servidor, asi se cierra el server_fd
	return 1;
}

int terminarServidor(int server_fd, int client_fd)
{
	log_info(logger, "Terminando servidor");
	close(server_fd);
	close(client_fd);
	return EXIT_SUCCESS;
}
