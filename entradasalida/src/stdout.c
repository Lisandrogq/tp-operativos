#include "client.h"
#include <readline/readline.h>
#include <semaphore.h>

void iniciar_interfaz_stdout()
{

	inicializar_cliente_kernel();
	inicializar_cliente_memoria();
	t_interfaz *nueva_interfaz = crear_estrcutura_io(STDOUT);
	enviar_interfaz(CREACION_IO, *nueva_interfaz, socket_kernel);
	while (1) // xd
	{
		int max_tam = 0;
		io_task *pedido = recibir_peticion();
		log_debug(logger, "Llego un pedido STDOUT del pid %i", pedido->pid_solicitante);

		t_list *solicitudes = decode_addresses_buffer(pedido->buffer_instruccion, &max_tam);
		char *output_string = malloc(max_tam); // POR ALGUNA RAZON ESTO FUNCIONA SIN AGREGAR \0 AL FINAL
		leer_memoria(solicitudes, output_string);
		log_info(logger, "STDOUT: %s", output_string);
		informar_fin_de_tarea(socket_kernel, IO_OK, pedido->pid_solicitante, "IO_STDOUT_WRITE");
		liberar_y_eliminar_solicitudes(solicitudes);

		free(output_string);
	}
}
solicitud_unitaria_t *leer_memoria_unitario(solicitud_unitaria_t *sol)
{
	u_int32_t dir_fisica = sol->dir_fisica_base + sol->offset;
	int tam_r_datos = sol->tam;
	solicitar_leer_memoria(dir_fisica, tam_r_datos, sol->pid);
	int cod_op = recibir_operacion(socket_memoria); // waitall y codop
	void *datos_obtenidos = recibir_datos_leidos();
	sol->datos = malloc(sol->tam);
	memcpy(sol->datos, datos_obtenidos, sol->tam);
	int *logeable = malloc(sizeof(int));
	memset(logeable, 0, sizeof(int));
	memcpy(logeable, datos_obtenidos, sol->tam);
	log_debug(logger, "Lei en la Dir Física: %i - Valor: %i", sol->dir_fisica_base + sol->offset, *logeable);
	free(logeable);
	return sol;
}
void *recibir_datos_leidos()
{
	void *datos_leidos;
	int tam_leido = 0;
	t_paquete *paquete = malloc(sizeof(t_paquete));
	paquete->buffer = malloc(sizeof(t_buffer));

	recv(socket_memoria, &(paquete->buffer->size), sizeof(int), 0);
	paquete->buffer->stream = malloc(paquete->buffer->size);
	recv(socket_memoria, paquete->buffer->stream, paquete->buffer->size, 0);

	void *stream = paquete->buffer->stream;
	memcpy(&tam_leido, stream, sizeof(int));
	stream += sizeof(int);
	datos_leidos = malloc(tam_leido);
	memcpy(datos_leidos, stream, tam_leido);

	eliminar_paquete(paquete);

	return datos_leidos;
}

void solicitar_leer_memoria(u_int32_t dir_fisica, int tam_r_datos, int pid)
{
	t_paquete *paquete = malloc(sizeof(t_paquete));

	paquete->codigo_operacion = READ_MEM;
	paquete->buffer = malloc(sizeof(t_buffer));
	paquete->buffer->size = sizeof(int) * 3;
	paquete->buffer->stream = malloc(paquete->buffer->size);
	paquete->buffer->offset = 0;

	memcpy(paquete->buffer->stream + paquete->buffer->offset, &dir_fisica, sizeof(u_int32_t));
	paquete->buffer->offset += sizeof(int);

	memcpy(paquete->buffer->stream + paquete->buffer->offset, &tam_r_datos, sizeof(int));
	paquete->buffer->offset += sizeof(int);
	memcpy(paquete->buffer->stream + paquete->buffer->offset, &pid, sizeof(int));
	paquete->buffer->offset += sizeof(int);
	int bytes = paquete->buffer->size + 2 * sizeof(int);

	void *a_enviar = serializar_paquete(paquete, bytes);

	send(socket_memoria, a_enviar, bytes, 0);
	free(a_enviar);
	eliminar_paquete(paquete);
}
void leer_memoria(t_list *solicitudes, void *datos)
{
	list_map(solicitudes, leer_memoria_unitario);
	t_list_iterator *iterator = list_iterator_create(solicitudes);
	int write_offset = 0;
	while (list_iterator_has_next(iterator))
	{
		solicitud_unitaria_t *sol = list_iterator_next(iterator);
		memcpy(datos + write_offset, sol->datos, sol->tam);
		write_offset += sol->tam;
	}
	list_iterator_destroy(iterator);
}
