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
sem_t elementos_ready; // contador de ready, si no hay, no podes planificar.
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
		t_strings_instruccion *instruccion_de_desalojo = malloc(sizeof(t_strings_instruccion));; // creo q no  debería generar conflicto con los desalojos sin instruccion
		log_warning(logger, "antes DEL WAIT");
		sem_wait(&elementos_ready); // este sem debería es un contador de procesos en ready
		log_error(logger, "DESPUES DEL WAIT");
		int motivo_desalojo = -1;
		if (strcmp(algoritmo, "FIFO") == 0)
		{
			motivo_desalojo = planificar_fifo(conexion_fd, instruccion_de_desalojo);
			pcb_t *pcb_prueba = list_get(lista_pcbs_exec, 0); // creo q esto no va
		}
		else if (strcmp(algoritmo, "RR") == 0)
		{
			motivo_desalojo = planificar_rr(conexion_fd,instruccion_de_desalojo);
		}
		else
		{
			// planificar_vrr();
		}

		switch (motivo_desalojo)
		{
		case FIN:
			pcb_t *pcb = list_get(lista_pcbs_exec, 0);
			list_remove(lista_pcbs_exec, 0);
			pcb->state = EXIT_S;
			break;
		case RELOJ:
			pcb_t *pcb_reloj = list_get(lista_pcbs_exec, 0);
			list_remove(lista_pcbs_exec, 0);
			pcb_reloj->state = READY_S;
			list_add(lista_pcbs_ready, pcb_reloj);
			break;
		case IO_SLEEP: // CAPAZ ES UN SOLO CASE PARA TODAS LAS IO_
			pcb_t *pcb_a_bloquear = list_get(lista_pcbs_exec, 0);
			list_remove(lista_pcbs_exec, 0);
			pcb_a_bloquear->state = BLOCK_S;
			// list_add(lista_pcbs_bloqueado, pcb_a_bloquear); asi debeŕia ser
			list_add(lista_pcbs_ready, pcb_a_bloquear);
			sem_post(&elementos_ready);

			int status = try_io_task(pcb_a_bloquear->pid, instruccion_de_desalojo); // instruccion sale del planificarfifo
			if (status == -1)
			{
				// bloquear proceso
			}
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
{
	sem_init(&elementos_ready, 0, 0); // no se si es el lugar correcto para inicializarlo

	pthread_mutex_init(&mutex_socket_memoria, NULL);
	pthread_mutex_lock(&mutex_socket_memoria);
	sem_init(&hay_IO, 0, 0);		  // no se si es el lugar correcto para inicializarlo
	lista_pcbs_ready = list_create(); // la crea aca pero cuando entra el hilo se borran los datos
	lista_pcbs_bloqueado = list_create();
	lista_pcbs_exec = list_create();
	dictionary_ios = dictionary_create();
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

	socket_memoria = inicializar_cliente_memoria();

	consola();

	pthread_join(tid[0], NULL);
	pthread_detach(tid[1]);
	pthread_detach(tid[2]);
	return 0;
}