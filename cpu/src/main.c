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

bool terminar_modulo = false;

void ejecutar_cliclos()
{
	while (!terminar_modulo)
	{
		int status = STATUS_OK;
		sem_wait(&hay_proceso);
		while (status != STATUS_DESALOJADO)
		{
			log_error(logger,"%i",pcb_exec->registros->PC);
			t_strings_instruccion *instruccion = fetch(pcb_exec->registros->PC); // hace falta enviar el pid??

			decode(instruccion); // por ahora no sabemos q hacer en decode

			status = execute(instruccion);
			if (status == STATUS_OK) // si el proceso justo desalojo en execute, la interrupcion se leera en luego de
									 // que se ejecute la siguiente ejecucion
			{						 // revisar si esto es teoricamente correcto, creo qsi
				check_intr(&status); // asi puede marcar el proceso como desalojado
			}
		}
	}
}

void solicitar_siguiente_instruccion()
{

	t_paquete *paquete = malloc(sizeof(t_paquete));

	paquete->codigo_operacion = FETCH;
	paquete->buffer = malloc(sizeof(t_buffer));
	paquete->buffer->size = sizeof(int) * 2;
	paquete->buffer->stream = malloc(paquete->buffer->size);
	paquete->buffer->offset = 0;

	memcpy(paquete->buffer->stream + paquete->buffer->offset, &(pcb_exec->pid), sizeof(int));
	paquete->buffer->offset += sizeof(int);

	memcpy(paquete->buffer->stream + paquete->buffer->offset, &(pcb_exec->registros->PC), sizeof(int));
	paquete->buffer->offset += sizeof(int);

	int bytes = paquete->buffer->size + 2 * sizeof(int); // ESTE *2 NO SE PUEDE TOCAR, ANDA ASÍ, PUNTO(.).

	void *a_enviar = serializar_paquete(paquete, bytes);

	send(socket_memoria, a_enviar, bytes, 0);
	free(a_enviar);
	eliminar_paquete(paquete);
}

t_strings_instruccion *recibir_siguiente_instruccion()
{
	int codop = recibir_operacion(socket_memoria); // m lo saco de encima pq esta en el paquete xd

	t_buffer *buffer = malloc(sizeof(t_buffer));
	recv(socket_memoria, &(buffer->size), sizeof(int), 0);
	buffer->stream = malloc(buffer->size);
	recv(socket_memoria, buffer->stream, buffer->size, 0);

	t_strings_instruccion *palabras = malloc(sizeof(t_strings_instruccion));
	memset(palabras, 0, sizeof(t_strings_instruccion));

	void *stream = buffer->stream;
	memcpy(&(palabras->tamcod), stream, sizeof(int));
	stream += sizeof(int);
	palabras->cod_instruccion = malloc(palabras->tamcod);
	memcpy(palabras->cod_instruccion, stream, palabras->tamcod);
	stream += palabras->tamcod;

	memcpy(&(palabras->tamp1), stream, sizeof(int));
	stream += sizeof(int);

	palabras->p1 = malloc(palabras->tamp1);
	memset(palabras->p1, 0, 1); // se pone el unico byte alocado por malloc(0) en 0 para limpiar la basura(caso parametro vacio)
	memcpy((palabras->p1), stream, palabras->tamp1);
	stream += palabras->tamp1;

	memcpy(&(palabras->tamp2), stream, sizeof(int));
	stream += sizeof(int);

	palabras->p2 = malloc(palabras->tamp2);
	memset(palabras->p2, 0, 1); // se pone el unico byte alocado por malloc(0) en 0 para limpiar la basura(caso parametro vacio)

	memcpy((palabras->p2), stream, palabras->tamp2);
	stream += palabras->tamp2;

	memcpy(&(palabras->tamp3), stream, sizeof(int));
	stream += sizeof(int);

	palabras->p3 = malloc(palabras->tamp3);
	memset(palabras->p3, 0, 1); // se pone el unico byte alocado por malloc(0) en 0 para limpiar la basura(caso parametro vacio)

	memcpy((palabras->p3), stream, palabras->tamp3);
	stream += palabras->tamp3;

	memcpy(&(palabras->tamp4), stream, sizeof(int));
	stream += sizeof(int);

	palabras->p4 = malloc(palabras->tamp4);
	memset(palabras->p4, 0, 1); // se pone el unico byte alocado por malloc(0) en 0 para limpiar la basura(caso parametro vacio)

	memcpy((palabras->p4), stream, palabras->tamp4);
	stream += palabras->tamp4;

	memcpy(&(palabras->tamp5), stream, sizeof(int));
	stream += sizeof(int);

	palabras->p5 = malloc(palabras->tamp5);
	memset(palabras->p5, 0, 1); // se pone el unico byte alocado por malloc(0) en 0 para limpiar la basura(caso parametro vacio)

	memcpy((palabras->p5), stream, palabras->tamp5);
	stream += palabras->tamp5;

	// en algun lugar(afuera de esto) hay q hacerle malloc a las palabras.
	free(buffer->stream);
	free(buffer);
	return palabras;
}

t_strings_instruccion *fetch(int PC)
{
	// ACA IRÍA UN WAIT('HAY_PROCESO_PARA_EJECUTAR') QUE SE ACTUALIZA CUANDO SE RECIBE UN CONTEXTO EN DISPATCH;
	log_fetch_instruccion();
	solicitar_siguiente_instruccion();
	t_strings_instruccion *palabras = recibir_siguiente_instruccion();

	pcb_exec->registros->PC += 1; // EM LA CONSINGA DICE: HACER ESTO SI CORRESPONDE, NO SE Q SIGNIFICA

	return palabras;
}
void decode(t_strings_instruccion *instruccion)
{
	if (strcmp(instruccion->cod_instruccion, "MOV_IN") == 0) // MOV_IN (Registro Datos, Registro Dirección)
	{

		void *dir_logica = dictionary_get(dic_p_registros, instruccion->p2);
		int tam_r_dir = *((int *)dictionary_get(dic_tam_registros, instruccion->p2));

		int tam_r_datos = *((int *)dictionary_get(dic_tam_registros, instruccion->p1));
		void *datos = dictionary_get(dic_p_registros, instruccion->p1);

		u_int32_t dir_fisica = 0;
		dir_fisica = obtener_direccion_fisica(dir_logica); // esto sería obtener direcioneSSS fisicas

		// for(por cada dir fisica a acceder)
		log_info(logger, "dir_fisica a leer: %i", dir_fisica);
		execute_mov_in(datos, dir_fisica, tam_r_datos);
	}

	if (strcmp(instruccion->cod_instruccion, "MOV_OUT") == 0) // MOV_OUT (Registro Dirección, Registro Datos)
	{
		int tam_r_datos = *((int *)dictionary_get(dic_tam_registros, instruccion->p2));
		void *datos = dictionary_get(dic_p_registros, instruccion->p2);
		void *dir_logica = dictionary_get(dic_p_registros, instruccion->p1);
		int tam_r_dir = *((int *)dictionary_get(dic_tam_registros, instruccion->p1));

		u_int32_t dir_fisica = 0;
		dir_fisica = obtener_direccion_fisica(dir_logica); // esto sería obtener direcioneSSS fisicas

		log_info(logger, "dir_fisica_a_escribir: %i", dir_fisica);
		execute_mov_out(datos, dir_fisica, tam_r_datos);
	}
}

int obtener_direccion_fisica(void *dir_logica)
{
	int pagina = floor((*(int *)dir_logica) / tam_pagina);
	int offset = (*(int *)dir_logica) - pagina * tam_pagina;
	solicitar_frame(pagina);
	int cod_op = recibir_operacion(socket_memoria);
	int frame = recibir_frame();
	return frame * tam_pagina + offset; // inicio frame + offset
}
int recibir_frame()
{
	int frame = -1;
	t_paquete *paquete = malloc(sizeof(t_paquete));
	paquete->buffer = malloc(sizeof(t_buffer));

	recv(socket_memoria, &(paquete->buffer->size), sizeof(int), 0);
	paquete->buffer->stream = malloc(paquete->buffer->size);
	recv(socket_memoria, paquete->buffer->stream, paquete->buffer->size, 0);

	void *stream = paquete->buffer->stream;
	memcpy(&frame, stream, sizeof(int));

	eliminar_paquete(paquete);

	return frame;
}
int solicitar_frame(int pagina)
{
	t_paquete *paquete = malloc(sizeof(t_paquete));

	paquete->codigo_operacion = GET_FRAME;
	paquete->buffer = malloc(sizeof(t_buffer));
	paquete->buffer->size = sizeof(int) * 2;
	paquete->buffer->stream = malloc(paquete->buffer->size);
	paquete->buffer->offset = 0;

	memcpy(paquete->buffer->stream + paquete->buffer->offset, &(pcb_exec->pid), sizeof(u_int32_t));
	paquete->buffer->offset += sizeof(int);

	memcpy(paquete->buffer->stream + paquete->buffer->offset, &pagina, sizeof(int));
	paquete->buffer->offset += sizeof(int);

	int bytes = paquete->buffer->size + 2 * sizeof(int);

	void *a_enviar = serializar_paquete(paquete, bytes);

	send(socket_memoria, a_enviar, bytes, 0);
	free(a_enviar);
	eliminar_paquete(paquete);
}
int execute(t_strings_instruccion *instruccion)
{
	log_instruccion_ejecutada(instruccion); // en teoría debería ir al final de cada if,xd
	if (strcmp(instruccion->cod_instruccion, "SET") == 0)
	{

		int num = atoi((instruccion->p2));
		execute_set(instruccion->p1, num);
		free((instruccion->p1));
		free(instruccion);
		return STATUS_OK;
	}
	if (strcmp(instruccion->cod_instruccion, "SUM") == 0)
	{
		// u_int32_t *p1 = dictionary_get(dictionary, instruccion->p1);
		// u_int32_t *p2 = dictionary_get(dictionary, instruccion->p2);
		execute_sum(instruccion->p1, instruccion->p2);
		free((instruccion->p1));
		free((instruccion->p2));
		free(instruccion);
		return STATUS_OK;
	}
	if (strcmp(instruccion->cod_instruccion, "SUB") == 0)
	{
		execute_sub(instruccion->p1, instruccion->p2);
		free((instruccion->p1));
		free((instruccion->p2));
		free(instruccion);
		return STATUS_OK;
	}
	if (strcmp(instruccion->cod_instruccion, "JNZ") == 0)
	{
		u_int32_t p2 = instruccion->p2;
		execute_jnz(instruccion->p1, p2, pcb_exec->registros);
		free((instruccion->p1));
		free(instruccion);
		return STATUS_OK;
	}

	if ((strstr(instruccion->cod_instruccion, "EXIT") != NULL)) // Fix termporal a aparicion random de '%'o'5'
	{

		devolver_pcb(SUCCESS, *pcb_exec, socket_dispatch, instruccion); // habria que ponerle mutex a dispatch
		return STATUS_DESALOJADO;
	}
	if (strcmp(instruccion->cod_instruccion, "IO_GEN_SLEEP") == 0)
	{
		devolver_pcb(IO_TASK, *pcb_exec, socket_dispatch, instruccion); // habria que ponerle mutex a dispatch
		return STATUS_DESALOJADO;
	}
}
void check_intr(int *status)
{
	t_strings_instruccion *instruccion_vacia = malloc(sizeof(t_strings_instruccion));
	memset(instruccion_vacia, 0, sizeof(t_strings_instruccion));
	int cod_op = 0;
	recv(socket_interrupt, &cod_op, sizeof(int), MSG_DONTWAIT);
	if (cod_op == INTERRUPCION)
	{
		interrupcion_t *interrupcion = recibir_interrupcion(socket_interrupt);
		if (interrupcion->pid == pcb_exec->pid)
		{

			switch (interrupcion->motivo)
			{
			case INTERRUPTED_BY_USER:
				//	retornar por dispatch(mutex por las dudas)
				// marcar como desalojado
				devolver_pcb(INTERRUPTED_BY_USER, *pcb_exec, socket_dispatch, instruccion_vacia);
				// liberar_pcb(pcb_exec);
				*status = STATUS_DESALOJADO;
				log_debug(logger, "SE DESALOJO UN PROCESO POR TERMINAR_PROCESO_INTR");
				break;
			case CLOCK:
				// retornar por dispatch(mutex por las dudas)
				//	marcar como desalojado
				devolver_pcb(CLOCK, *pcb_exec, socket_dispatch, instruccion_vacia);
				// liberar_pcb(pcb_exec);
				*status = STATUS_DESALOJADO;
				log_debug(logger, "SE DESALOJO UN PROCESO POR CLOCK_INTR");
				break;
			}
		}
		else
			log_debug(logger, "Se recibio una intr para el pid %i, mientras ejecuta el pid%i motivo:%i", interrupcion->pid, pcb_exec->pid, interrupcion->motivo);
	}
	// else
	//  log_debug(logger, "No se recibio interrupciones");
}
void *servidor_interrupt()
{
	logger = log_create("cpu.log", "Servidor", 1, LOG_LEVEL_DEBUG);
	int server_fd = iniciar_servidor(PUERTO_CPU_INTERRUPT, logger);
	log_info(logger, "Cpu-interruptlisto para recibir");
	socket_interrupt = esperar_cliente_cpu(server_fd);
	// client_handler_interrupt(socket_interrupt);no hace falta,creo, las cosas se reciven en checkintr
}

void *servidor_dispatch()
{
	logger = log_create("cpu.log", "Servidor", 1, LOG_LEVEL_DEBUG);
	int server_fd = iniciar_servidor(PUERTO_CPU_DISPATCH, logger);
	log_info(logger, "Cpu-dipatch listo para recibir");
	socket_dispatch = esperar_cliente_cpu(server_fd);
	client_handler_dispatch(socket_dispatch);
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
	tam_pagina = handshake(conexion_fd);

	return conexion_fd;
}

void inicializar_diccionario_tams()
{
	int *tam8 = malloc(sizeof(int));
	int *tam32 = malloc(sizeof(int));
	*tam8 = 1;
	*tam32 = 4;

	dictionary_put(dic_tam_registros, "AX", tam8);
	dictionary_put(dic_tam_registros, "BX", tam8);
	dictionary_put(dic_tam_registros, "CX", tam8);
	dictionary_put(dic_tam_registros, "DX", tam8);
	dictionary_put(dic_tam_registros, "EAX", tam32);
	dictionary_put(dic_tam_registros, "EBX", tam32);
	dictionary_put(dic_tam_registros, "ECX", tam32);
	dictionary_put(dic_tam_registros, "EDX", tam32);
	dictionary_put(dic_tam_registros, "SI", tam32);
	dictionary_put(dic_tam_registros, "DI", tam32);
}
int main(int argc, char const *argv[])
{
	sem_init(&hay_proceso, 0, 0);
	sem_init(&desalojar, 0, 0);
	dic_p_registros = dictionary_create();
	dic_tam_registros = dictionary_create();
	inicializar_diccionario_tams();

	// dictionary = inicializar_diccionario(contexto); esto ya no hace falta, se inicializa al recibir pcb

	t_log *logger;
	t_config *config;
	logger = iniciar_logger();
	logger = log_create("cpu.log", "Cpu_MateLavado", 1, LOG_LEVEL_DEBUG);
	config = iniciar_config();
	config = config_create("cpu.config");

	iniciar_thread_dispatch();
	iniciar_thread_interrupt();
	socket_memoria = iniciar_conexion_memoria(config, logger);

	ejecutar_cliclos();

	pthread_join(tid[0], NULL);
	pthread_join(tid[1], NULL);
	return 0;
}