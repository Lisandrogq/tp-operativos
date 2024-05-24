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
t_list *lista_pcbs_ready;
t_list *lista_pcbs_bloqueado;
t_list *lista_pcbs_exec;

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
			sem_post(&hay_procesos);
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
	log_info(logger, "Este es el modulo: %s", modulo);
	ip = config_get_string_value(config, "IP_CPU");
	puerto = config_get_string_value(config, "PUERTO_CPU_DISPATCH");
	algoritmo = config_get_string_value(config, "ALGORITMO_PLANIFICACION");
	quantum = config_get_int_value(config, "QUANTUM");
	conexion_fd = crear_conexion(ip, puerto, logger);
	int resultado = handshake(conexion_fd);

	// hay que enviar pcb//

	enviar_operacion(OPERACION_KERNEL_1, modulo, conexion_fd);
	enviar_operacion(MENSAJE, "SOY EL CLIENTE DISPATCH", conexion_fd);
	
	sem_wait(&hay_procesos);
	int motivo_desalojo=0;
	if(algoritmo == "FIFO"){
		motivo_desalojo = planificar_fifo(conexion_fd);
	}else if(algoritmo=="RR"){
		motivo_desalojo = planificar_rr(conexion_fd);
	}else{
		//planificar_vrr();
	}
	
	switch (motivo_desalojo)
	{
	case FIN:
		pcb_t *pcb = list_get(lista_pcbs_exec, 0);
    	list_remove(lista_pcbs_exec, 0);
		pcb->state = EXIT_S;
		break;
	case RELOJ:
		pcb_t *pcb = list_get(lista_pcbs_exec, 0);
    	list_remove(lista_pcbs_exec, 0);
		pcb->state = READY_S;
		list_add(lista_pcbs_ready, pcb);
		break;
	case PRUEBA:
		// soy una prueba
		log_info(logger, "FUNCIONE");
		break;

	default:
		log_info(logger, "Entre al switch pero al default");
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

	/* Al recibir una petición de I/O de parte de la CPU primero se deberá validar que exista y esté conectada la interfaz solicitada, en caso contrario, se deberá enviar el proceso a EXIT.
	En caso de que la interfaz exista y esté conectada, se deberá validar que la interfaz admite la operación solicitada, en caso de que no sea así, se deberá enviar el proceso a EXIT.
	De cumplirse todos los requisitos anteriores, el Kernel enviará el proceso al estado BLOCKED y a partir de este punto pueden darse 2 situaciones:
	El caso en el que la interfaz de I/O esté libre: En este caso el Kernel deberá solicitar la operación al dispositivo correspondiente. 
	En el caso de que exista algún proceso haciendo uso de la Interfaz de I/O, el proceso que acaba de solicitar la operación de I/O deberá esperar la finalización del anterior antes de poder hacer uso de la misma.
	Una vez la operación finalice, el Kernel recibirá una notificación y desbloqueará dicho proceso para que esté listo para continuar con su ejecución cuando le toque según el algoritmo. */

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
	t_list *lista_pcbs_ready = list_create();
	t_list *lista_pcbs_bloqueado = list_create();
	t_list *lista_pcbs_exec = list_create();
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

	socket_memoria = inicializar_cliente_memoria();

	consola();

	pthread_join(tid[0], NULL);
	pthread_detach(tid[1]);
	pthread_detach(tid[2]);
	return 0;
}