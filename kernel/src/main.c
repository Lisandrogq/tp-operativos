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

pthread_t tid[3];
void *consola()
{ //------creo q la consola debería ir en otra carpeta / archivo, seguro tiene bastantes cositas
	char *linea;
	char *instruccion[2];
	while (1)
	{
		linea = readline(">");
		instruccion[0] = malloc(strlen(linea));
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
			int tam = 1 + strlen(instruccion[1]);
			comando_iniciar_proceso(instruccion[1], tam);
			sem_post(&elementos_ready);
		}
		if (!strcmp(instruccion[0], "FINALIZAR_PROCESO"))
		{
			int tam = 1 + sizeof(strlen(linea));
			comando_finalizar_proceso(instruccion[1], INTERRUPTED_BY_USER);
		}
		if (!strcmp(instruccion[0], "ESTADO_PROCESO"))
		{
			comando_listar_procesos_por_estado();
		}
		if (!strcmp(instruccion[0], "ddd"))
			return;
		if (!strcmp(instruccion[0], "EJECUTAR_SCRIPT"))
		{
			log_info(logger, "Ejecutar Script de Comandos");
			FILE *archivo = fopen(instruccion[1], "r");
			if (archivo == NULL)
			{
				log_error(logger, "No se pudo abrir el archivo");
				free(linea);
				continue;
			}
			comando_ejecutar_script(instruccion[1], archivo);
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

	enviar_operacion(OPERACION_KERNEL_1, modulo, conexion_fd);
	enviar_operacion(MENSAJE, "SOY EL CLIENTE DISPATCH", conexion_fd);

	while (1)
	{
		t_strings_instruccion *instruccion_de_desalojo = malloc(sizeof(t_strings_instruccion));
		// creo q no  debería generar conflicto con los desalojos sin instruccion
		log_debug(logger, "Esperando nuevos procesos en ready...");
		sem_wait(&elementos_ready); // este sem debería es un contador de procesos en ready
		int motivo_desalojo = -1;
		motivo_desalojo = planificar(conexion_fd, instruccion_de_desalojo, algoritmo);
		pcb_t *pcb_desalojado = list_remove(lista_pcbs_exec, 0);

		switch (motivo_desalojo)
		{
/* 		case:
			WAIT break;

		case:
			SIGNAL
			list_add(lista_pcbs_exec, 0, pcb_desalojado);
			break; */
		case SUCCESS: // agregar resto de casos de fin

			pcb_desalojado->state = EXIT_S;
			list_add(lista_pcbs_exit, pcb_desalojado);

			if (pid_sig_term == pcb_desalojado->pid)
			{ // si justo se ejecuto exit, no tiene pedir eliminar, lo hace la consola
				log_debug(logger, "ENTRE AL MANEJO DE SIG_TERM");

				pthread_mutex_unlock(&mutex_pcb_desalojado);
				list_add(lista_pcbs_exit, pcb_desalojado);
			}
			else
			{
				solicitar_eliminar_estructuras_administrativas(pcb_desalojado->pid);
			}
			log_info(logger, "Finaliza el proceso %i - Motivo: SUCCESS", pcb_desalojado->pid);

			break;
		case INTERRUPTED_BY_USER: // agregar resto de casos de fin

			pcb_desalojado->state = EXIT_S;
			pthread_mutex_unlock(&mutex_pcb_desalojado); // este mutex se usa para los proceso matados por kernel(si sale solo noahce falta)
			list_add(lista_pcbs_exit, pcb_desalojado);
			log_info(logger, "Finaliza el proceso %i - Motivo: INTERRUPTED_BY_USER", pcb_desalojado->pid);

			break;
		case CLOCK:
			if (pid_sig_term == pcb_desalojado->pid)
			{ // si justo se salio por clock,,se lo mata igual. pq el comando pide matar.
				log_debug(logger, "ENTRE AL MANEJO DE SIG_TERM");
				pthread_mutex_unlock(&mutex_pcb_desalojado); // este mutex se usa para los proceso matados por kernel(si sale solo noahce falta)
				list_add(lista_pcbs_exit, pcb_desalojado);
			}
			else
			{
				pcb_desalojado->state = READY_S;
				pthread_mutex_lock(&mutex_lista_ready);
				list_add(lista_pcbs_ready, pcb_desalojado);
				pthread_mutex_unlock(&mutex_lista_ready);
				sem_post(&elementos_ready);
				log_debug(logger, "Se desalojo un proceso por clock");
			}
			break;
		case IO_TASK: // CAPAZ ES UN SOLO CASE PARA TODAS LAS IO_
			if (pid_sig_term == pcb_desalojado->pid)
			{ // si justo se salio por io,se lo mata igual. pq el comando pide matar.
				log_warning(logger, "ENTRE AL MANEJO DE SIG_TERM");

				pthread_mutex_unlock(&mutex_pcb_desalojado); // este mutex se usa para los proceso matados por kernel(si sale solo noahce falta)
				list_add(lista_pcbs_exit, pcb_desalojado);
			}
			else
			{
				if (es_una_io_valida(pcb_desalojado->pid, instruccion_de_desalojo) == -1)
				{
					solicitar_eliminar_estructuras_administrativas(pcb_desalojado->pid);
					list_add(lista_pcbs_exit, pcb_desalojado);
					pcb_desalojado->state = EXIT_S;
					log_info(logger, "Finaliza el proceso %i - Motivo: INVALID_INTERFACE", pcb_desalojado->pid);
				}
				else
				{
					t_interfaz *io = dictionary_get(dictionary_ios, instruccion_de_desalojo->p1);
					pcb_desalojado->state = BLOCK_S;
					// pthread_mutex_lock(&mutex_lista_bloqueado);
					t_cola_io *struct_io = dictionary_get(dictionary_pcbs_bloqueado, instruccion_de_desalojo->p1); //[1] segunda palabra
					t_list *cola_de_io_pedido = struct_io->cola_de_io_pedido;
					sem_t *elementos = struct_io->elementos_cola_io;
					elemento_cola_io *elemento = malloc(sizeof(elemento_cola_io));
					elemento->pcb = pcb_desalojado;
					elemento->instruccion_de_bloqueo = instruccion_de_desalojo;
					if (list_size(cola_de_io_pedido) == 0)
					{ // si esta vació la pide y agrega a blocked
						log_debug(logger, "ENTRE AL IF la cola esta vacia");
						pedir_io_task(pcb_desalojado->pid, io, instruccion_de_desalojo);
						list_add(cola_de_io_pedido, elemento);
						sem_post(elementos);
					}
					else
					{ // sino va a la cola blocked
						log_debug(logger, "ENTRE AL IF la cola tiene cosas");
						list_add(cola_de_io_pedido, elemento);
						sem_post(elementos);
					}
					log_info(logger, "PID: %i - Bloqueado por: INTERFAZ", pcb_desalojado->pid);
				}
			}
			break;

		default:
			log_info(logger, "Entre al switch pero al default");
			break;
		}
	}
}

void *cliente_cpu_interrupt() // esto podría no ser un hilo y simplemente genera la conexion
{
	char *ip;
	char *puerto;
	char *modulo;

	modulo = config_get_string_value(config, "MODULO");
	log_info(logger, "Este es el modulo: %s", modulo);
	ip = config_get_string_value(config, "IP_CPU");
	puerto = config_get_string_value(config, "PUERTO_CPU_INTERRUPT");

	socket_interrupt = crear_conexion(ip, puerto, logger);
	int resultado = handshake(socket_interrupt);

	enviar_operacion(OPERACION_KERNEL_1, modulo, socket_interrupt);
	enviar_operacion(MENSAJE, "SOY EL CLIENTE INTERRUPT", socket_interrupt);
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
{ // creo que no hace falta mutex para exec
	pid_sig_term = -1;
	sem_init(&elementos_ready, 0, 0);

	pthread_mutex_init(&mutex_lista_exit, NULL);
	pthread_mutex_unlock(&mutex_lista_exit); // debe empezar desbloqueado, pq todos hacen lock primero

	pthread_mutex_init(&mutex_lista_exec, NULL);
	pthread_mutex_unlock(&mutex_lista_exec); // debe empezar desbloqueado, pq todos hacen lock primero

	pthread_mutex_init(&mutex_pcb_desalojado, NULL);
	pthread_mutex_lock(&mutex_pcb_desalojado); // debe empezar bloqueado, solo es desbloqueado al desalojarse un proceso(por kernel)

	pthread_mutex_init(&mutex_lista_ready, NULL);
	pthread_mutex_unlock(&mutex_lista_ready); // debe empezar desbloqueado, pq todos hacen lock primero
	pthread_mutex_init(&mutex_socket_memoria, NULL);
	pthread_mutex_lock(&mutex_socket_memoria); // se libera cuando se habilita el socket
	lista_pcbs_exit = list_create();
	lista_pcbs_ready = list_create(); // la crea aca pero cuando entra el hilo se borran los datos
	dictionary_pcbs_bloqueado = dictionary_create();
	lista_pcbs_exec = list_create();
	lista_ready_mas = list_create();
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