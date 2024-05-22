#include <stdlib.h>
#include <stdio.h>
#include <utils/utils_generales.h>
#include <string.h>
#include <commons/log.h>
#include <readline/readline.h>
#include <pthread.h>
#include <semaphore.h>
#include "utils.h"
#include <errno.h>

t_log *logger;
t_config *config;
pthread_t tid[3];
sem_t hay_procesos;

void *consola()
{ //------creo q la consola debería ir en otra carpeta / archivo, seguro tiene bastantes cositas
	char *linea;
	char *instruccion[2];
	while (1)
	{
		linea = readline(">");
		instruccion[0] = malloc(strlen(linea));//CAMBIAR 39 POR ALGO!!!!!
		instruccion[1] = malloc(strlen(linea));
		instruccion[0] = strtok(linea, " ");
		instruccion[1] = strtok(NULL, " ");
		if (strcmp(linea, "\0") == 0)
		{
			free(linea);
			continue;
		}

		if (!strcmp(instruccion[0], "INICIAR_PROCESO"))
		{	
			int tam = sizeof(strlen(linea));
			iniciar_proceso(instruccion[1], tam);
			sem_post(&hay_procesos);
		}

		free(linea);
	}

}
void planificar(){//to do esta funcion elegue el proceso a ser ejecutado y lo marca como exec
}
void *cliente_cpu_dispatch()
{

	int conexion_fd;
	char *ip;
	char *puerto;
	char *modulo;

	modulo = config_get_string_value(config, "MODULO");
	log_info(logger, "Este es el modulo: %s", modulo);
	ip = config_get_string_value(config, "IP_CPU");
	puerto = config_get_string_value(config, "PUERTO_CPU_DISPATCH");

	conexion_fd = crear_conexion(ip, puerto, logger);
	int resultado = handshake(conexion_fd);

	// hay que enviar pcb//

	enviar_operacion(OPERACION_KERNEL_1, modulo, conexion_fd);
	enviar_operacion(MENSAJE, "SOY EL CLIENTE DISPATCH", conexion_fd);
	sem_wait(&hay_procesos);
	planificar(); 
	log_info(logger, "DESPUES DE PLANIFICAR");
	int motivo_desalojo = proceso_CPU(DISPATCH,&lista_pcbs[0],conexion_fd); // se encarga de enviar y recibir el nuevo contexto actualizando lo que haga falta
	log_info(logger, "ARRIBA DEL SWIATCH:%i", motivo_desalojo);
	switch (motivo_desalojo)
	{
	case PRUEBA:
		//soy una prueba
		log_info(logger, "FUNCIONE");
		break;
	
	default:
		log_info(logger, "Entre al switch pero al defaulr");
		break;
	}
	return 0;
}

void *cliente_cpu_interrupt()
{

	int conexion_fd;
	char *ip;
	char *puerto;
	char *modulo;

	modulo = config_get_string_value(config, "MODULO");
	log_info(logger, "Este es el modulo: %s", modulo);
	ip = config_get_string_value(config, "IP_CPU");
	puerto = config_get_string_value(config, "PUERTO_CPU_INTERRUPT");

	conexion_fd = crear_conexion(ip, puerto, logger);
	int resultado = handshake(conexion_fd);

	enviar_operacion(OPERACION_KERNEL_1, modulo, conexion_fd);
	enviar_operacion(MENSAJE, "SOY EL CLIENTE INTERRUPT", conexion_fd);
	return 0;
}
int inicializar_cliente_memoria()
{

	int conexion_fd;
	char *ip;
	char *puerto;
	char *modulo;

	modulo = config_get_string_value(config, "MODULO");
	log_info(logger, "Este es el modulo: %s", modulo);
	ip = config_get_string_value(config, "IP_MEMORIA");
	puerto = config_get_string_value(config, "PUERTO_MEMORIA");

	conexion_fd = crear_conexion(ip, puerto, logger);
	int resultado = handshake(conexion_fd);
	if (resultado == -1)
		return;
	
	pthread_mutex_unlock(&mutex_socket_memoria);
	return conexion_fd;
}

int *servidor()
{
	int server_fd = iniciar_servidor(PUERTO_KERNEL, logger);
	if (server_fd == -1)
	{
		log_error(logger, "Error al iniciar el servidor");
		return -1;
	}
	log_info(logger, "kernel servidor-escucha iniciado correctamente");
	esperar_cliente(server_fd, client_handler);
	log_warning(logger, "Termino el servidor");
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

	pthread_mutex_init(&mutex_socket_memoria, NULL);
	pthread_mutex_lock(&mutex_socket_memoria);
	logger = iniciar_logger();
	logger = log_create("kernel.log", "Kernel_MateLavado", 1, LOG_LEVEL_INFO);
	config = iniciar_config();
	config = config_create("kernel.config");

	int err;
	err = pthread_create(&(tid[0]), NULL, servidor, NULL);
	if (err != 0)
	{
		log_error(logger, "Hubo un problema al crear el thread servidor:[%s]", strerror(err));
		return err;
	}
	log_info(logger, "El thread servidor inició su ejecución");

	err = pthread_create(&(tid[1]), NULL, cliente_cpu_interrupt, NULL);
	if (err != 0)
	{
		log_error(logger, "Hubo un problema al crear el thread cliente_cpu:[%s]", strerror(err));
		return err;
	}
	log_info(logger, "El thread cliente_cpu_interrupt inició su ejecución");

	sem_init(&hay_procesos, 0, 0); // no se si es el lugar correcto para inicializarlo
	err = pthread_create(&(tid[2]), NULL, cliente_cpu_dispatch, NULL);
	if (err != 0)
	{
		log_error(logger, "Hubo un problema al crear el thread cliente_cpu:[%s]", strerror(err));
		return err;
	}
	log_info(logger, "El thread cliente_cpu_dispatch inició su ejecución");

	socket_memoria =inicializar_cliente_memoria();


	consola();

	pthread_join(tid[0], NULL);
	pthread_detach(tid[1]);
	pthread_detach(tid[2]);
	return 0;
}