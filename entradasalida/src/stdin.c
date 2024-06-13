#include "client.h"
#include <readline/readline.h>
#include <semaphore.h>
t_list *decode_buffer_stdin(buffer_instr_io_t *buffer_instruccion, int *max_tam)
{
	void *buffer = buffer_instruccion->buffer;
	t_list *solicitudes = list_create(); // solicitud_unitaria_t *
	int offset = 0;
	while (offset < buffer_instruccion->size) // en stdin, siempre se reciben sizes multiplos de 3
	{
		solicitud_unitaria_t *sol = malloc(sizeof(solicitud_unitaria_t));
		memset(sol, 0, sizeof(solicitud_unitaria_t));
		memcpy(&(sol->dir_fisica_base), buffer + offset, sizeof(u_int32_t));
		offset += sizeof(u_int32_t);
		memcpy(&(sol->offset), buffer + offset, sizeof(u_int32_t));
		offset += sizeof(u_int32_t);
		memcpy(&(sol->tam), buffer + offset, sizeof(u_int32_t));
		offset += sizeof(u_int32_t);
		*max_tam += sol->tam;
		list_add(solicitudes, sol);
	}
	return solicitudes;
}

void iniciar_interfaz_stdin()
{

	inicializar_cliente_kernel();
	inicializar_cliente_memoria();
	t_interfaz *nueva_interfaz = crear_estrcutura_io(STDIN);
	enviar_interfaz(CREACION_IO, *nueva_interfaz, socket_kernel);
	while (1) // xd
	{
		int max_tam = 0;
		io_task *pedido = recibir_peticion();
		log_debug(logger, "Llego un pedido STDIN del pid %i", pedido->pid_solicitante);

		t_list *solicitudes = decode_buffer_stdin(pedido->buffer_instruccion, &max_tam);
		char *input_string = malloc(max_tam);
		input_string = readline(">");
		popular_solicitudes(solicitudes, input_string);
		escribir_memoria(solicitudes);
		informar_fin_de_tarea(socket_kernel, IO_OK, pedido->pid_solicitante, "IO_STDIN_READ");
		free(input_string);
	}
}
void solicitar_escribir_memoria(void *datos, u_int32_t dir_fisica, int tam_r_datos)
{
	t_paquete *paquete = malloc(sizeof(t_paquete));

	paquete->codigo_operacion = WRITE_MEM;
	paquete->buffer = malloc(sizeof(t_buffer));
	paquete->buffer->size = sizeof(int) * 2 + tam_r_datos;
	paquete->buffer->stream = malloc(paquete->buffer->size);
	paquete->buffer->offset = 0;

	memcpy(paquete->buffer->stream + paquete->buffer->offset, &dir_fisica, sizeof(u_int32_t));
	paquete->buffer->offset += sizeof(int);

	memcpy(paquete->buffer->stream + paquete->buffer->offset, &tam_r_datos, sizeof(int));
	paquete->buffer->offset += sizeof(int);

	memcpy(paquete->buffer->stream + paquete->buffer->offset, datos, tam_r_datos);
	paquete->buffer->offset += tam_r_datos;

	int bytes = paquete->buffer->size + 2 * sizeof(int);

	void *a_enviar = serializar_paquete(paquete, bytes);

	send(socket_memoria, a_enviar, bytes, 0);
	free(a_enviar);
	eliminar_paquete(paquete);
}
int recibir_status_escritura()
{
	int status;
	t_paquete *paquete = malloc(sizeof(t_paquete));
	paquete->buffer = malloc(sizeof(t_buffer));

	recv(socket_memoria, &(paquete->buffer->size), sizeof(int), 0);
	paquete->buffer->stream = malloc(paquete->buffer->size);
	recv(socket_memoria, paquete->buffer->stream, paquete->buffer->size, 0);

	void *stream = paquete->buffer->stream;
	memcpy(&status, stream, sizeof(int));

	eliminar_paquete(paquete);

	return status;
}

void escribir_memoria_unitario(solicitud_unitaria_t *sol)
{
	solicitar_escribir_memoria(sol->datos, sol->dir_fisica_base + sol->offset, sol->tam);
	int cod_op = recibir_operacion(socket_memoria);	   // waitall y codop
	int status_escritura = recibir_status_escritura(); // es irrelevante el status(no aclaran q la escritura falle)
	int *logeable = malloc(sizeof(int));
	memset(logeable, 0, sizeof(int));
	memcpy(logeable, sol->datos, sol->tam);
	log_debug(logger, "Escribí en la Dir Física: %i - Valor: %i", sol->dir_fisica_base + sol->offset, *logeable);
	free(logeable);
}
void escribir_memoria(t_list *solicitudes)
{

	t_list_iterator *iterator = list_iterator_create(solicitudes);
	while (list_iterator_has_next(iterator))
	{
		solicitud_unitaria_t *sol = list_iterator_next(iterator);
		escribir_memoria_unitario(sol);
	}
	list_iterator_destroy(iterator);
	// destruir y liberar lista
}
void popular_solicitudes(t_list *solicitudes, char *input_string) // gran nombre
{
	t_list_iterator *iterator = list_iterator_create(solicitudes);
	int write_offset = 0;
	while (list_iterator_has_next(iterator))
	{
		solicitud_unitaria_t *sol = list_iterator_next(iterator);
		sol->datos = malloc(sol->tam);
		memcpy(sol->datos, input_string + write_offset, sol->tam);
		write_offset += sol->tam;
	}
	list_iterator_destroy(iterator);
}
