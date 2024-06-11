#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <commons/log.h>
#include "utils.h"
#include <commons/config.h>
t_config *config;

int main(int argc, char *argv[])
{
	/// definicion lista tabla paginas:
	lista_tablas_paginas = list_create(); // elems de tipo elemento_lista_tablas

	///
	config = config_create("memoria.config");
	RETARDO_RESPUESTA = config_get_int_value(config, "RETARDO_RESPUESTA");
	TAM_MEMORIA = config_get_int_value(config, "TAM_MEMORIA");
	TAM_PAGINA = config_get_int_value(config, "TAM_PAGINA");
	sems_espera_creacion_codigos = list_create();
	espacio_usuario = malloc(TAM_MEMORIA);
	memset(espacio_usuario, 0, TAM_MEMORIA);
	/* 	u_int8_t intp8 =38;
		u_int32_t intp32 =432;
		int tam8 = sizeof(u_int8_t);
		int tam32 = sizeof(u_int32_t);
		memcpy(espacio_usuario+1,&intp8,tam8);
		memcpy(espacio_usuario+(2*4)+2,&intp32,tam32/2);
		memcpy(espacio_usuario+(3*4)+0,((void*)(&intp32))+2,tam32/2); */
	marcos = TAM_MEMORIA / TAM_PAGINA; // son multiplos
	int bytes = marcos / 8;
	if (marcos % 8 != 0)
	{
		bytes++; // se crea el bitarray con mas bytes de la cantidad de marcos pero se debe validar siempre usar hasta el bit marcos-1;
	}
	void *map_space = malloc(bytes);
	memset(map_space, 0, bytes);
	bitarray = bitarray_create_with_mode(map_space, bytes, LSB_FIRST);
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
