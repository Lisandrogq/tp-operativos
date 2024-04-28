#include <stdlib.h>
#include <stdio.h>
#include <utils/utils_generales.h>
#include <string.h>
#include <commons/log.h>
#include <readline/readline.h>
#include <pthread.h>
#include <semaphore.h>
#include "utils.h"

t_log *logger;
t_config *config;

pthread_t tid[3];
void *cliente_cpu()
{
	int conexion_fd;
	char *ip;
	char *puerto;
	char *modulo;

	modulo = config_get_string_value(config, "MODULO");
	log_info(logger, "Este es el modulo: %s", modulo);
	ip = config_get_string_value(config, "IP_CPU");
	puerto = config_get_string_value(config, "PUERTO_CPU");

	conexion_fd = crear_conexion(ip, puerto);
	int resultado = handshake(conexion_fd);

	enviar_operacion(OPERACION_KERNEL_1, modulo, conexion_fd);
	enviar_operacion(MENSAJE, "segundo mensaje", conexion_fd);
	return 0;
}
void *cliente_memoria()
{

	int conexion_fd;
	char *ip;
	char *puerto;
	char *modulo;

	modulo = config_get_string_value(config, "MODULO");
	log_info(logger, "Este es el modulo: %s", modulo);
	ip = config_get_string_value(config, "IP_MEMORIA");
	puerto = config_get_string_value(config, "PUERTO_MEMORIA");

	conexion_fd = crear_conexion(ip, puerto);
	int resultado = handshake(conexion_fd);

	enviar_operacion(OPERACION_KERNEL_1, modulo, conexion_fd);
	enviar_operacion(MENSAJE, "segundo mensaje", conexion_fd);
	return 0;
}

int *servidor()
{
	logger = log_create("kernel.log", "Servidor", 1, LOG_LEVEL_DEBUG);
	int server_fd = iniciar_servidor(PUERTO_KERNEL, logger);
	if (server_fd == -1)
	{
		log_error(logger, "Error al iniciar el servidor");
		return -1;
	}
	log_info(logger, "kernel servidor-escucha iniciado correctamente");
	esperar_cliente(server_fd, client_handler);
	// habría que ver cuando se termina el servidor, asi se cierra el server_fd
}
t_log *iniciar_logger(void)
{
	t_log *nuevo_logger;

	return nuevo_logger;
}

t_config *iniciar_config(void)
{
	t_config *nuevo_config;

	return nuevo_config;
}
int terminarServidor(int server_fd, int client_fd)
{
	log_info(logger, "Terminando servidor");
	close(server_fd);
	close(client_fd);
	return EXIT_SUCCESS;
}
void terminar_programa(int conexion, t_log *logger, t_config *config)
{
	log_destroy(logger);
	config_destroy(config);
	liberar_conexion(conexion);
}
int main(int argc, char const *argv[])
{
	logger = iniciar_logger();
	logger = log_create("kernel.log", "Kernel_MateLavado", 1, LOG_LEVEL_INFO);
	config = iniciar_config();
	config = config_create("kernel.config");

	int err;
	err = pthread_create(&(tid[0]), NULL, servidor, NULL);
	if (err != 0)
	{
		printf("\nHubo un problema al crear el thread servidor:[%s]", strerror(err));
		return err;
	}
	printf("\nEl thread servidor inició su ejecución\n");

	 err = pthread_create(&(tid[1]), NULL, cliente_cpu, NULL);
	  if (err != 0)
	  {
	  	printf("\nHubo un problema al crear el thread cliente_cpu:[%s]", strerror(err));
	  	return err;
	  }
	  printf("\nEl thread cliente_cpu inició su ejecución\n");

	//--- POR ALGUNA RAZON, SI EL KERNEL SE CONECTA A CPU PASA UNA DE LAS DOS COSAS:
	//--- 1. HACE HANDSHAKE PERO NO MANDA NADA Y SE DESCONECTA
	//--- 2. CORE DUMPED
	//--- POSIBLE SOLUCION: CREAR LAS DOS CONEXIONES EN MAIN Y LUEGO GENERAR HILOS Q LAS ADMINISTREN(TENGO SUEÑO) 

	//  err = pthread_create(&(tid[2]), NULL, cliente_memoria, NULL);
	//  if (err != 0)
	//  {
	//  	printf("\nHubo un problema al crear el thread cliente_memoria:[%s]", strerror(err));
	//  	return err;
	//  }
	//  printf("\nEl thread cliente_memoria inició su ejecución\n");

	 

	pthread_join(tid[0],NULL);
	//pthread_detach(tid[1]);
	pthread_detach(tid[2]);
	return 0;
}