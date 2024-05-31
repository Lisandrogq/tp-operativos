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
void *consola()
{ //------creo q la consola debería ir en otra carpeta / archivo, seguro tiene bastantes cositas
	char *linea;
	char *instruccion[2];
	while (1)
	{
		linea = readline(">");
		instruccion[0] = malloc(strlen(linea)); // CAMBIAR 39 POR ALGO!!!!!
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
			int tam = 1 + sizeof(strlen(linea));
			iniciar_proceso(instruccion[1], tam);
			sem_post(&elementos_ready);
		}
		if (!strcmp(instruccion[0], "ddd"))
			return;
		free(linea);
	}
}
void *cliente_cpu_dispatch()
{

	int conexion_fd;
	char *ip;
	char *puerto;
	char *modulo;
	char *algoritmo;
	int quantum;
	modulo = config_get_string_value(config, "MODULO");
	log_info(logger, "Este es el modulo:%s", modulo);
	ip = config_get_string_value(config, "IP_CPU");
	puerto = config_get_string_value(config, "PUERTO_CPU_DISPATCH");
	algoritmo = config_get_string_value(config, "ALGORITMO_PLANIFICACION");
	quantum = config_get_int_value(config, "QUANTUM");
	conexion_fd = crear_conexion(ip, puerto, logger);
	int resultado = handshake(conexion_fd);

	// hay que enviar pcb//

	enviar_operacion(OPERACION_KERNEL_1, modulo, conexion_fd);
	enviar_operacion(MENSAJE, "SOY EL CLIENTE DISPATCH", conexion_fd);

	while (1)
	{
		t_strings_instruccion *instruccion_de_desalojo = malloc(sizeof(t_strings_instruccion));
		; // creo q no  debería generar conflicto con los desalojos sin instruccion
		log_debug(logger, "Esperando nuevos procesos en ready...");
		sem_wait(&elementos_ready); // este sem debería es un contador de procesos en ready
		int motivo_desalojo = -1;
		if (strcmp(algoritmo, "FIFO") == 0)
		{

			motivo_desalojo = planificar_fifo(conexion_fd, instruccion_de_desalojo);
		}
		else if (strcmp(algoritmo, "RR") == 0)
		{
			motivo_desalojo = planificar_rr(conexion_fd, instruccion_de_desalojo);
		}
		else
		{
			// planificar_vrr();
		}

		switch (motivo_desalojo)
		{
		case FIN:
			pcb_t *pcb = list_remove(lista_pcbs_exec, 0);
			pcb->state = EXIT_S;
			log_debug(logger, "Se desalojo un proceso por exit");

			break;
		case RELOJ:
			pcb_t *pcb_reloj = list_remove(lista_pcbs_exec, 0);
			pcb_reloj->state = READY_S;
			pthread_mutex_lock(&mutex_lista_ready);
			list_add(lista_pcbs_ready, pcb_reloj);
			pthread_mutex_unlock(&mutex_lista_ready);

			log_debug(logger, "Se desalojo un proceso por clock");

			break;
		case IO_SLEEP: // CAPAZ ES UN SOLO CASE PARA TODAS LAS IO_
			pcb_t *pcb_a_bloquear = list_remove(lista_pcbs_exec, 0);
			pcb_a_bloquear->state = BLOCK_S;
			pthread_mutex_lock(&mutex_lista_ready);
			list_add(lista_pcbs_bloqueado, pcb_a_bloquear);
			pthread_mutex_unlock(&mutex_lista_ready);
			int status = try_io_task(pcb_a_bloquear->pid, instruccion_de_desalojo); // instruccion_de_desalojo sale del planificarfifo
			if (status == -1)
			{
				// matar proceso
			}
			log_debug(logger, "Se desalojo un proceso por IO_TASK");

			break;

		default:
			log_info(logger, "Entre al switch pero al default");
			break;
		}
	}
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
{											//creo que no hace falta mutex para exec
	sem_init(&elementos_ready, 0, 0);
	pthread_mutex_init(&mutex_lista_ready, NULL);
	pthread_mutex_unlock(&mutex_lista_ready); // debe empezar desbloqueado, pq todos hacen lock primero
	pthread_mutex_init(&mutex_socket_memoria, NULL);
	pthread_mutex_lock(&mutex_socket_memoria); // se libera cuando se habilita el socket
	lista_pcbs_ready = list_create();		   // la crea aca pero cuando entra el hilo se borran los datos
	lista_pcbs_bloqueado = list_create();
	lista_pcbs_exec = list_create();
	dictionary_ios = dictionary_create();
	logger = iniciar_logger();
	logger = log_create("kernel.log", "Kernel_MateLavado", 1, LOG_LEVEL_DEBUG);
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

	socket_memoria = inicializar_cliente_memoria();

	consola();

	pthread_join(tid[0], NULL);
	pthread_detach(tid[1]);
	pthread_detach(tid[2]);
	return 0;
}