#include "client.h"
#include <readline/readline.h>
#include <semaphore.h>


int inicializar_cliente_kernel()
{
	char *ip;
	char *puerto;

	ip = config_get_string_value(config, "IP_KERNEL");
	puerto = config_get_string_value(config, "PUERTO_KERNEL");

	socket_kernel = crear_conexion(ip, puerto, logger);
	int resultado = handshake(socket_kernel);
	if (resultado == -1)
		log_error(logger, "ERROR AL CONECTAR CON KERNEL");

	return -1;
}
void inicializar_cliente_memoria() // todavia no se usa
{

	char *ip;
	char *puerto;
	char *modulo;

	modulo = config_get_string_value(config, "MODULO");
	log_info(logger, "Este es el modulo: %s", modulo);
	ip = config_get_string_value(config, "IP_MEMORIA");
	puerto = config_get_string_value(config, "PUERTO_MEMORIA");

	socket_memoria = crear_conexion(ip, puerto, logger);
	int resultado = handshake(socket_memoria);
	if (resultado == -1)
		log_error(logger, "ERROR AL CONECTAR CON MEMORIA");
	return;
}

io_task *recibir_pedido_io(int socket_kernel)
{
	io_task *pedido = malloc(sizeof(io_task));
	t_buffer *buffer = malloc(sizeof(t_buffer));
	recv(socket_kernel, &(buffer->size), sizeof(int), 0);
	buffer->stream = malloc(buffer->size);
	recv(socket_kernel, buffer->stream, buffer->size, 0);

	t_strings_instruccion *palabras = malloc(sizeof(t_strings_instruccion));
	pedido->instruccion = palabras;
	memset(palabras, 0, sizeof(t_strings_instruccion));
	void *stream = buffer->stream;
	memcpy(&(pedido->pid_solicitante), stream, sizeof(int));
	stream += sizeof(int);
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

	free(buffer->stream);
	free(buffer);

	log_warning(logger, "LLEGO un pedido de sleep del pid %i", pedido->pid_solicitante);

	return pedido;
}

void informar_fin_de_tarea(int socket_kernel, int status, int pid, char *operacion) // esta podríía llegar a ser usada por todos los io,depende de los params q manden
{
	int codop = FIN_IO_TASK;

	log_info(logger, "PID: %i - Operacion: %s", pid, operacion);

	t_paquete *paquete = malloc(sizeof(t_paquete));
	int tam_nombre = strlen(nombre) + 1; //\0
	paquete->codigo_operacion = codop;
	paquete->buffer = malloc(sizeof(t_buffer));
	paquete->buffer->size = sizeof(int) * 2 + tam_nombre;
	paquete->buffer->stream = malloc(paquete->buffer->size);
	paquete->buffer->offset = 0;

	memcpy(paquete->buffer->stream + paquete->buffer->offset, &pid, sizeof(int));
	paquete->buffer->offset += sizeof(int);

	memcpy(paquete->buffer->stream + paquete->buffer->offset, &tam_nombre, sizeof(int));
	paquete->buffer->offset += sizeof(int);

	memcpy(paquete->buffer->stream + paquete->buffer->offset, nombre, tam_nombre);
	paquete->buffer->offset += tam_nombre;

	int bytes = paquete->buffer->size + 2 * sizeof(int);

	void *a_enviar = serializar_paquete(paquete, bytes);

	send(socket_kernel, a_enviar, bytes, 0);

	free(a_enviar);
	eliminar_paquete(paquete);
}

int main(int argc, char *argv[])
{

	logger = iniciar_logger();
	logger = log_create("IO.log", "IO_MateLavado", 1, LOG_LEVEL_INFO);
	config = iniciar_config();
	config = config_create("IO.config");
	// char *tipo = config_get_string_value(config, "TIPO_INTERFAZ");
	nombre = "Int1";
	char *tipo = "GENERICA";
	if (strcmp(tipo, "GENERICA") == 0)
	{
		iniciar_interfaz_generica();
	}
	if (strcmp(tipo, "STDIN") == 0)
	{
		// iniciar_interfaz_stdin();
	}
	if (strcmp(tipo, "STDOUT") == 0)
	{
		// iniciar_interfaz_stdout();
	}
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

void terminar_programa(int conexion, t_log *logger, t_config *config)
{
	log_destroy(logger);
	config_destroy(config);
	liberar_conexion(conexion);
}
