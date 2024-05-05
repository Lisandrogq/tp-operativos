#include <stdlib.h>
#include <stdio.h>
#include <utils/utils_generales.h>
#include <string.h>
#include <commons/log.h>
#include <commons/collections/dictionary.h>
#include <readline/readline.h>
#include <pthread.h>
#include <semaphore.h>
#include "utils.h"

pthread_t tid[2];

registros_t *contexto;
int pid;
bool terminar_modulo = false;
t_dictionary *dictionary;

void ejecutar_cliclos(int conexion_fd)
{
	while (!terminar_modulo)
	{
		t_instruccion *instruccion = fetch(conexion_fd,contexto->PC);//hace falta enviar el pid??
		decode(); // por ahora no sabemos q hacer en decode
		execute(instruccion);

		terminar_modulo = true;
		contexto->PC += 1;
	}
}
t_instruccion *fetch(int conexion_fd, int PC)
{
	// aca se haría send a socket de memoria y recv instruccion
	// esto es para ver si anda, sirve de base para la deserializacion.
	t_instruccion *instruccion;
	instruccion = malloc(sizeof(t_instruccion));
	instruccion->cod_instruccion = 0;
	instruccion->p1 = malloc(sizeof(char) * 3 + 1); // xej: EAX/0
	strcpy(instruccion->p1, "EDX");
	instruccion->p2 = 2 * 2 * 2 * 2 + 42;

	return instruccion;
}
void decode() {}
void execute(t_instruccion *instruccion)
{

	if (instruccion->cod_instruccion == SET)
	{
		u_int32_t *p1 = dictionary_get(dictionary, instruccion->p1);
		int p2 = instruccion->p2;
		cpu_set(p1, p2);
	}
	free((instruccion->p1));
	free(instruccion);
}

void *servidor_interrupt()
{
	logger = log_create("cpu.log", "Servidor", 1, LOG_LEVEL_DEBUG);
	int server_fd = iniciar_servidor(PUERTO_CPU_INTERRUPT, logger);
	log_info(logger, "Cpu-interruptlisto para recibir");
	int socket_cliente = esperar_cliente_cpu(server_fd);
	client_handler_interrupt(socket_cliente);
}

void *servidor_dispatch()
{
	logger = log_create("cpu.log", "Servidor", 1, LOG_LEVEL_DEBUG);
	int server_fd = iniciar_servidor(PUERTO_CPU_DISPATCH, logger);
	log_info(logger, "Cpu-dipatch listo para recibir");
	int socket_cliente = esperar_cliente_cpu(server_fd);
	client_handler_dispatch(socket_cliente);
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

void iniciar_thread_dispatch()
{
	int err;
	err = pthread_create(&(tid[0]), NULL, servidor_dispatch, NULL);
	if (err != 0)
	{
		printf("\nHubo un problema al crear el thread servidor-cpu-dispatch:[%s]", strerror(err));
		return err;
	}
	printf("\nEl thread servidor-cpu-dispatch inició su ejecución\n");
}

void iniciar_thread_interrupt()
{
	int err = pthread_create(&(tid[1]), NULL, servidor_interrupt, NULL);
	if (err != 0)
	{
		printf("\nHubo un problema al crear el thread servidor-cpu-interrupt:[%s]", strerror(err));
		return err;
	}
	printf("\nEl thread servidor-cpu-interrupt inició su ejecución\n");
}
int iniciar_conexion_memoria(t_config *config, t_log *logger)
{
	int conexion_fd;
	char *ip;
	char *puerto;
	char *modulo;

	modulo = config_get_string_value(config, "MODULO");
	log_info(logger, modulo);
	ip = config_get_string_value(config, "IP_MEMORIA");
	puerto = config_get_string_value(config, "PUERTO_MEMORIA");

	conexion_fd = crear_conexion(ip, puerto, logger);
	int resultado = handshake(conexion_fd);

	return conexion_fd;
}
t_dictionary * inicializar_diccionario(){
	dictionary = dictionary_create();
	dictionary_put(dictionary, "AX", &(contexto->AX));
	dictionary_put(dictionary, "BX", &(contexto->BX));
	dictionary_put(dictionary, "CX", &(contexto->CX));
	dictionary_put(dictionary, "DX", &(contexto->DX));
	dictionary_put(dictionary, "EAX", &(contexto->EAX));
	dictionary_put(dictionary, "EBX", &(contexto->EBX));
	dictionary_put(dictionary, "ECX", &(contexto->ECX));
	dictionary_put(dictionary, "EDX", &(contexto->EDX));
	dictionary_put(dictionary, "SI", &(contexto->SI));
	dictionary_put(dictionary, "DI", &(contexto->DI));
}

int main(int argc, char const *argv[])
{
	contexto = malloc(sizeof(registros_t));
	memset(contexto, 0, sizeof(registros_t));
	dictionary = inicializar_diccionario();

	t_log *logger;
	t_config *config;
	logger = iniciar_logger();
	logger = log_create("cpu.log", "Cpu_MateLavado", 1, LOG_LEVEL_INFO);
	config = iniciar_config();
	config = config_create("cpu.config");

	int conexion_fd;

	iniciar_thread_dispatch();
	iniciar_thread_interrupt();
	conexion_fd = iniciar_conexion_memoria(config, logger);

	log_info(logger, "antes EDX:%i", contexto->EDX);
	ejecutar_cliclos(conexion_fd);
	log_info(logger, "despues EDX:%i", contexto->EDX);

	pthread_join(tid[0], NULL);
	pthread_join(tid[1], NULL);
	return 0;
}