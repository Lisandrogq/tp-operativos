#include "client.h"
#include <readline/readline.h>
#include <semaphore.h>

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
		gen_resolver_peticion(pedido);
	}
}

void gen_resolver_peticion(io_task *pedido) // gran nombre
{
	gen_sleep(pedido->instruccion->p2);
	informar_fin_de_tarea(socket_kernel, IO_OK, pedido->pid_solicitante, pedido->instruccion->cod_instruccion);
}
io_task *gen_recibir_peticion()
{

	int cod_op = recibir_operacion(socket_kernel);
	if (cod_op == -1)
		return -1;
	io_task *pedido = recibir_pedido_io(socket_kernel);
	return pedido;
}
void gen_sleep(char *p2)
{
	int tiempo = atoi(p2);
	sleep(tiempo * 1);
	return;
}