#include "client.h"
#include <readline/readline.h>
#include <semaphore.h>


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
		log_info(logger, "PID: %i - Operacion:  IO:STDIN_READ", pedido->pid_solicitante);


		t_list *solicitudes = decode_addresses_buffer(pedido->buffer_instruccion, &max_tam);
		char *input_string = malloc(max_tam);
		input_string = readline(">");
		populate_solicitudes(solicitudes, input_string);
		escribir_memoria(solicitudes);
		informar_fin_de_tarea(socket_kernel, IO_OK, pedido->pid_solicitante);
		liberar_y_eliminar_solicitudes(solicitudes);
		
		free(input_string);
	}
}

