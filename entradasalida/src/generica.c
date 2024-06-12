#include "client.h"
#include <readline/readline.h>
#include <semaphore.h>
int decode_buffer_sleep(buffer_instr_io_t *buffer_instruccion)
{

	int cant_sleep = 0;
	memcpy(&cant_sleep, buffer_instruccion->buffer, sizeof(int));
	return cant_sleep;
}
void iniciar_interfaz_generica()
{
	inicializar_cliente_kernel();
	inicializar_cliente_memoria();
	tiempo_Unidad_Trabajo = config_get_int_value(config, "TIEMPO_UNIDAD_TRABAJO");
	t_interfaz *nueva_interfaz = crear_estrcutura_io(GENERICA);
	enviar_interfaz(CREACION_IO, *nueva_interfaz, socket_kernel);
	while (1) // xd
	{
		io_task *pedido = gen_recibir_peticion();
		int cant_sleep = decode_buffer_sleep(pedido->buffer_instruccion);
		gen_resolver_peticion(cant_sleep);
		informar_fin_de_tarea(socket_kernel, IO_OK, pedido->pid_solicitante, "IO_GEN_SLEEP");
	}
}

void gen_resolver_peticion(int cant_sleep) // gran nombre
{
	sleep(cant_sleep);
}
io_task *gen_recibir_peticion()
{

	int cod_op = recibir_operacion(socket_kernel);
	if (cod_op == -1)
		return -1;
	io_task *pedido = recibir_pedido_io(socket_kernel);
	return pedido;
}
