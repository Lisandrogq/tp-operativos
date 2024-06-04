#include "client.h"
#include <readline/readline.h>
#include <semaphore.h>

t_log *logger;
t_config *config;
pthread_t tid[2];
int socket_kernel;
int *tiempo_Unidad_Trabajo;
t_list *lista_pedidos;
sem_t contador_pedidos;
pthread_mutex_t count_mutex;
int inicializar_cliente_memoria() // todavia no se usa
{
	int conexion_fd;
	char *ip;
	char *puerto;
	char *modulo;

	modulo = config_get_string_value(config, "MODULO");
	log_info(logger, "Este es el modulo: %s", modulo);
	ip = config_get_string_value(config, "IP_MEMORIA");
	puerto = config_get_string_value(config, "PUERTO_MEMORIA");

	conexion_fd = crear_conexion(ip, puerto, logger);
	int resultado = handshake(conexion_fd);
	if (resultado == -1)
		return;

	return conexion_fd;
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
void *resolvedor_de_peticiones() // gran nombre
{
	while (1)
	{
		log_warning(logger, "ESTOY ESPERANDO PARA MANDAR ALGO");

		sem_wait(&contador_pedidos);

		pthread_mutex_lock(&count_mutex);
		io_task *pedido = list_remove(lista_pedidos, 0);

		pthread_mutex_unlock(&count_mutex);

		GEN_SLEEP(pedido->instruccion->p2);
		informar_fin_de_tarea(socket_kernel, IO_OK, pedido->pid_solicitante);
	}
}
void *receptor_de_peticiones() // la idea es q este pushee una lista y el resolvedor saque, asi se puede escuchar y resolver simultaneamente
{
	tiempo_Unidad_Trabajo = config_get_int_value(config, "TIEMPO_UNIDAD_TRABAJO");
	t_interfaz *nueva_interfaz = crear_estrcutura_io(GENERICA);
	enviar_interfaz(CREACION_IO, *nueva_interfaz, socket_kernel);
	while (nueva_interfaz->estado == DISPONIBLE)
	{
		int cod_op = recibir_operacion(socket_kernel);
		if (cod_op == -1)
			return -1;
		io_task *pedido = recibir_pedido_io(socket_kernel);
		sem_post(&contador_pedidos);
		pthread_mutex_lock(&count_mutex);
		list_add(lista_pedidos, pedido); // podría usarse una queue pero esto anda
		pthread_mutex_unlock(&count_mutex);
		// push lista con mutex
	}
}
void GEN_SLEEP(char *p2)
{
	int tiempo = atoi(p2);
	sleep(tiempo * 1);
	return;
}

void informar_fin_de_tarea(int socket_kernel, int status, int pid) // esta podríía llegar a ser usada por todos los io,depende de los params q manden
{
	int codop = FIN_IO_TASK;

	log_info(logger, "PID: %i - Operacion: SLEEP", pid);

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

void iniciar_interfaz_generica()
{
	char *ip;
	char *puerto;
	char *modulo;
	char *tipo_Interfaz;

	ip = config_get_string_value(config, "IP_KERNEL");
	puerto = config_get_string_value(config, "PUERTO_KERNEL");

	tipo_Interfaz = config_get_string_value(config, "TIPO_INTERFAZ");

	socket_kernel = crear_conexion(ip, puerto, logger);
	int resultado = handshake(socket_kernel);
	if (resultado == -1)
		return;

	/// ESTA ESTRTUCTURA QUEDO DE CUANDO LAS IOS MANEJABAN SUS PEDIDOS PENDIENTES.
	/// AHORA ESO LO HACE KERNEL, BLOCKED SE DIVIDE EN IOS Y RECURSOS
	int err;
	err = pthread_create(&(tid[0]), NULL, inicializar_cliente_memoria, NULL); // todavia no se usa
	if (err != 0)
	{
		log_error(logger, "Hubo un problema al crear el thread cliente-memoria:[%s]", strerror(err));
		return err;
	}
	log_info(logger, "El thread cliente-memoria inició su ejecución");
	err = pthread_create(&(tid[1]), NULL, receptor_de_peticiones, NULL);
	if (err != 0)
	{
		log_error(logger, "Hubo un problema al crear el thread receptor_de_peticiones:[%s]", strerror(err));
		return err;
	}
	log_info(logger, "El thread receptor_de_peticiones inició su ejecución");

	err = pthread_create(&(tid[1]), NULL, resolvedor_de_peticiones, NULL);
	if (err != 0)
	{
		log_error(logger, "Hubo un problema al crear el thread resolvedor_de_peticiones:[%s]", strerror(err));
		return err;
	}

	log_info(logger, "El thread resolvedor_de_peticiones inició su ejecución");
}
void *hilo(){

}
int main(int argc, char *argv[])
{
	nombre = "Int1";
	// nombre=malloc(argc);
	// strcpy(nombre,argv);
	/*pthread_t hilo;
	pthread_create(&hilo, NULL, hilo_quantumVRR, pcb);
    pthread_detach(&hilo);
	*/
	sem_init(&contador_pedidos, 0, 0);
	pthread_mutex_init(&count_mutex, NULL);
	pthread_mutex_unlock(&count_mutex);
	lista_pedidos = list_create();
	logger = iniciar_logger();
	logger = log_create("IO.log", "IO_MateLavado", 1, LOG_LEVEL_INFO);
	config = iniciar_config();
	config = config_create("IO.config");
	char *tipo = config_get_string_value(config, "TIPO_INTERFAZ");
	if (strcmp(tipo, "GENERICA") == 0)
	{
		iniciar_interfaz_generica();
	}
	pthread_join(tid[0], NULL); // join para que no termine el main creo que puede llegar a terminar igual
	pthread_join(tid[1], NULL);
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
