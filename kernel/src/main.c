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

pthread_mutex_t operacion_mutex;
int operacion;
int next_pid = 0; // REVISAR SI ESTO NECESITA SEMAFORO

void *consola(){ //------creo q la consola debería ir en otra carpeta / archivo, seguro tiene bastantes cositas
	char *linea;
    while (1) {
        linea = readline(">");
        if (!linea) {
            break;
        }
        
        free(linea);
	}
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

	//hay que enviar pcb//

	enviar_operacion(OPERACION_KERNEL_1, modulo, conexion_fd);
	enviar_operacion(MENSAJE, "SOY EL CLIENTE DISPATCH", conexion_fd);
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

	conexion_fd = crear_conexion(ip, puerto, logger);
	int resultado = handshake(conexion_fd);
	if (resultado == -1)
		return;
	enviar_operacion(OPERACION_KERNEL_1, modulo, conexion_fd);
	enviar_operacion(MENSAJE, "segundo mensaje", conexion_fd);
	pcb_t * pcb = crear_pcb(1);
	enviar_operacion_PCB(CREAR_PCB, *pcb, conexion_fd);
	enviar_operacion_PCB(ELIMINAR_PCB, *pcb, conexion_fd); 
	eliminar_pcb(pcb);
	esperar_iniciar_proceso("pathxd", conexion_fd);
	return 0;
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

// FUNCIONES DE CONSOLA
void esperar_iniciar_proceso(char *path, int conexion_fd)
{
	while (1) // revisar
	{
		pthread_mutex_lock(&operacion_mutex);

		if (operacion == 0)
		{
			next_pid++;
			//enviar_operacion(CREAR_PCB, "next_pid", conexion_fd);
		}

		// else{		//--algo así va a ser necesario cuando soportemos varias ops por consola. o consumidor productor
		// 	pthread_mutex_lock(&operacion_mutex); 
		// }
	}
}


int main(int argc, char const *argv[])
{

	pthread_mutex_init(&operacion_mutex, NULL);
	pthread_mutex_lock (&operacion_mutex);
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

	err = pthread_create(&(tid[2]), NULL, cliente_cpu_dispatch, NULL);
	if (err != 0)
	{
		log_error(logger, "Hubo un problema al crear el thread cliente_cpu:[%s]", strerror(err));
		return err;
	}
	log_info(logger, "El thread cliente_cpu_dispatch inició su ejecución");

	err = pthread_create(&(tid[3]), NULL, cliente_memoria, NULL);
	if (err != 0)
	{
		log_error(logger, "Hubo un problema al crear el thread cliente_memoria:[%s]", strerror(err));
		return err;
	}
	log_info(logger, "El thread cliente_memoria inició su ejecución");

	// esto se escribe por consola:
	operacion = INICIAR_PROCESO;
	pthread_mutex_unlock(&operacion_mutex);

	pthread_join(tid[0], NULL);
	pthread_detach(tid[1]);
	pthread_detach(tid[2]);
	pthread_join(tid[3], NULL);
	return 0;
}