#include "client.h"
#include <readline/readline.h>
#include <semaphore.h>
t_list *decode_buffer_stdin(buffer_instr_io_t *buffer_instruccion)
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
		list_add(solicitudes, sol);
	}
	return solicitudes;
}

void iniciar_interfaz_stdin()
{

	inicializar_cliente_kernel();
	inicializar_cliente_memoria();
	tiempo_Unidad_Trabajo = config_get_int_value(config, "TIEMPO_UNIDAD_TRABAJO");
	t_interfaz *nueva_interfaz = crear_estrcutura_io(STDIN);
	enviar_interfaz(CREACION_IO, *nueva_interfaz, socket_kernel);
	while (1) // xd
	{
		io_task *pedido = recibir_peticion();
		t_list *solicitudes = decode_buffer_stdin(pedido->buffer_instruccion);
		char *input_string = "HOLA";
		stin_resolver_peticion(solicitudes, input_string);
		informar_fin_de_tarea(socket_kernel, IO_OK, pedido->pid_solicitante, "IO_STDIN_READ");
	}
}

void stin_resolver_peticion(t_list *solicitudes, char *input_string) // gran nombre
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
}
