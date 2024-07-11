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
		log_info(logger, "PID: %i - Operacion: IO_STDOUT_WRITE", pedido->pid_solicitante);

		t_list *solicitudes = decode_addresses_buffer(pedido->buffer_instruccion, &max_tam);
		char *output_string = malloc(max_tam + 1); //+1 PARA AGREGAR /0
		memset(output_string, 0, max_tam + 1);
		leer_memoria(solicitudes, output_string);
		log_info(logger, "STDOUT PARA PID[%i]: %s", pedido->pid_solicitante, output_string);
		informar_fin_de_tarea(socket_kernel, IO_OK, pedido->pid_solicitante);
		liberar_y_eliminar_solicitudes(solicitudes);

		free(output_string);
		free(pedido->buffer_instruccion);
		free(pedido);
	}
}



