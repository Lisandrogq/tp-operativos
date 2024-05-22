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
int pid_exec;
bool terminar_modulo = false;
int socket_memoria;

void ejecutar_cliclos()
{
	while (!terminar_modulo)
	{
		t_instruccion *instruccion = fetch(contexto->PC); // hace falta enviar el pid??
		decode();										  // por ahora no sabemos q hacer en decode
		execute(instruccion);
		check_intr();
		terminar_modulo = true;
	}
}

t_instruccion *fetch(int PC)
{
	t_instruccion *instruccion;
	instruccion = malloc(sizeof(t_instruccion));
	memset(instruccion, 0, sizeof(t_instruccion));
	/////
	t_paquete *paquete = malloc(sizeof(t_paquete));

	paquete->codigo_operacion = FETCH;
	paquete->buffer = malloc(sizeof(t_buffer));
	paquete->buffer->size = sizeof(int) * 2;
	paquete->buffer->stream = malloc(paquete->buffer->size);
	paquete->buffer->offset = 0;

	memcpy(paquete->buffer->stream + paquete->buffer->offset, &pid_exec, sizeof(int));
	paquete->buffer->offset += sizeof(int);

	memcpy(paquete->buffer->stream + paquete->buffer->offset, &PC, sizeof(int));
	paquete->buffer->offset += sizeof(int);

	int bytes = paquete->buffer->size + 2 * sizeof(int); //ESTE *2 NO SE PUEDE TOCAR, ANDA ASÍ, PUNTO(.).

	void *a_enviar = serializar_paquete(paquete, bytes);
	/////

	log_info(logger, "programcounter: %i", PC);
	send(socket_memoria, a_enviar, bytes, 0);
	free(a_enviar);
	eliminar_paquete(paquete);

	recv(socket_memoria, instruccion, sizeof(t_instruccion), MSG_WAITALL);//DEBE ESPERAR LA RESPUESTA
	
	/// aca se haría send a socket de memoria y recv instruccion
	/// esto es para ver si anda, sirve de base para la deserializacion.
	// instruccion->cod_instruccion = SET;
	// instruccion->p1 = malloc(sizeof(char) * 3 + 1); // xej: EAX/0
	// strcpy(instruccion->p1, "EDX");
	// instruccion->p2 = 37;

	//  instruccion->cod_instruccion = SUB;
	//  instruccion->p1 = malloc(sizeof(char) * 3 + 1); // xej: EAX/0
	//  strcpy(instruccion->p1, "EBX");
	//  instruccion->p2 = malloc(sizeof(char) * 3 + 1); // xej: EAX/0
	//  strcpy(instruccion->p2, "SI");

	instruccion->cod_instruccion = JNZ;
	instruccion->p1 = malloc(sizeof(char) * 3 + 1); // xej: EAX/0
	strcpy(instruccion->p1, "BX");
	instruccion->p2 = 31;

	contexto->PC += 1; // EM LA CONSINGA DICE: HACER ESTO SI CORRESPONDE, NO SE Q SIGNIFICA
	return instruccion;
}
void decode() {}
void execute(t_instruccion *instruccion)
{

	if (instruccion->cod_instruccion == SET)
	{
		// u_int32_t *p1 = dictionary_get(dictionary, instruccion->p1);
		execute_set(instruccion->p1, instruccion->p2);
		free((instruccion->p1));
		free(instruccion);
	}
	if (instruccion->cod_instruccion == SUM)
	{
		// u_int32_t *p1 = dictionary_get(dictionary, instruccion->p1);
		// u_int32_t *p2 = dictionary_get(dictionary, instruccion->p2);
		execute_sum(instruccion->p1, instruccion->p2);
		free((instruccion->p1));
		free((instruccion->p2));
		free(instruccion);
	}
	if (instruccion->cod_instruccion == SUB)
	{
		execute_sub(instruccion->p1, instruccion->p2);
		free((instruccion->p1));
		free((instruccion->p2));
		free(instruccion);
	}
	if (instruccion->cod_instruccion == JNZ)
	{
		u_int32_t p2 = instruccion->p2;
		execute_jnz(instruccion->p1, p2, contexto);
		free((instruccion->p1));
		free(instruccion);
	}
	if (instruccion->cod_instruccion == IO_GEN_SLEEP)
	{
		// todo
	}
}
void check_intr() {}
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
t_dictionary *inicializar_diccionario()
{
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

	iniciar_thread_dispatch();
	iniciar_thread_interrupt();
	socket_memoria = iniciar_conexion_memoria(config, logger);

	contexto->AX = 1;
	contexto->BX = 0;
	contexto->CX = 3;
	contexto->DX = 4;
	contexto->EAX = 5;
	contexto->EBX = 6;
	contexto->ECX = 7;
	contexto->EDX = 8;
	contexto->SI = 9;
	contexto->DI = 10;
	contexto->PC = 0;
	// log_info(logger, "antes AX:%i", contexto->AX);
	// log_info(logger, "antes BX:%i", contexto->BX);
	// log_info(logger, "antes CX:%i", contexto->CX);
	// log_info(logger, "antes DX:%i", contexto->DX);
	// log_info(logger, "antes EAX:%i", contexto->EAX);
	// log_info(logger, "antes EBX:%i", contexto->EBX);
	// log_info(logger, "antes ECX:%i", contexto->ECX);
	// log_info(logger, "antes EDX:%i", contexto->EDX);
	// log_info(logger, "antes SI:%i", contexto->SI);
	// log_info(logger, "antes DI:%i", contexto->DI);
	// log_info(logger, "antes PC:%i", contexto->PC);
	ejecutar_cliclos();
	// log_info(logger, "despues AX:%i", contexto->AX);
	// log_info(logger, "despues BX:%i", contexto->BX);
	// log_info(logger, "despues CX:%i", contexto->CX);
	// log_info(logger, "despues DX:%i", contexto->DX);
	// log_info(logger, "despues EAX:%i", contexto->EAX);
	// log_info(logger, "despues EBX:%i", contexto->EBX);
	// log_info(logger, "despues ECX:%i", contexto->ECX);
	// log_info(logger, "despues EDX:%i", contexto->EDX);
	// log_info(logger, "despues SI:%i", contexto->SI);
	// log_info(logger, "despues DI:%i", contexto->DI);
	// log_info(logger, "despues PC:%i", contexto->PC);

	pthread_join(tid[0], NULL);
	pthread_join(tid[1], NULL);
	return 0;
}