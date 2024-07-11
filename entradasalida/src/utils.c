#include "utils.h"
extern t_log *logger;
int contador; // 0 es el contador para generica
char *nombre;
t_log *logger;
t_config *config;
int socket_kernel;
int socket_memoria;
int tiempo_Unidad_Trabajo;
int RETRASO_COMPACTACION;
t_list *decode_addresses_buffer(buffer_instr_io_t *buffer_instruccion, int *max_tam) // max tam = suma de tam de cada sol
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
		memcpy(&(sol->pid), buffer + offset, sizeof(u_int32_t));
		offset += sizeof(u_int32_t);
		*max_tam += sol->tam;
		list_add(solicitudes, sol);
	}
	return solicitudes;
}
void enviar_operacion(int cod_op, char *mensaje, int socket_cliente)
{
	t_paquete *paquete = malloc(sizeof(t_paquete));

	paquete->codigo_operacion = cod_op;
	paquete->buffer = malloc(sizeof(t_buffer));
	paquete->buffer->size = strlen(mensaje) + 1;
	paquete->buffer->stream = malloc(paquete->buffer->size);
	memcpy(paquete->buffer->stream, mensaje, paquete->buffer->size);

	int bytes = paquete->buffer->size + 2 * sizeof(int); 

	void *a_enviar = serializar_paquete(paquete, bytes);

	send(socket_cliente, a_enviar, bytes, 0);

	free(a_enviar);
	eliminar_paquete(paquete);
}
void enviar_interfaz(int cod_op, t_interfaz interfaz, int socket_cliente)
{
	t_paquete *paquete = malloc(sizeof(t_paquete));
	int tam_nombre = strlen(interfaz.nombre) + 1; //\0
	paquete->codigo_operacion = cod_op;
	paquete->buffer = malloc(sizeof(t_buffer));
	paquete->buffer->size = sizeof(int) * 3 + tam_nombre;
	paquete->buffer->stream = malloc(paquete->buffer->size);
	paquete->buffer->offset = 0;

	memcpy(paquete->buffer->stream + paquete->buffer->offset, &tam_nombre, sizeof(int));
	paquete->buffer->offset += sizeof(int);

	memcpy(paquete->buffer->stream + paquete->buffer->offset, interfaz.nombre, tam_nombre);
	paquete->buffer->offset += tam_nombre;

	memcpy(paquete->buffer->stream + paquete->buffer->offset, &interfaz.tipo, sizeof(int));
	paquete->buffer->offset += sizeof(int);

	memcpy(paquete->buffer->stream + paquete->buffer->offset, &interfaz.estado, sizeof(int));
	int bytes = paquete->buffer->size + 2 * sizeof(int);

	void *a_enviar = serializar_paquete(paquete, bytes);

	send(socket_cliente, a_enviar, bytes, 0);

	free(a_enviar);
	eliminar_paquete(paquete);
}
t_interfaz *crear_estrcutura_io(int tipo)
{
	t_interfaz *interfaz = malloc(sizeof(t_interfaz));

	switch (tipo) // NO SE PAKE ESTA ESTO, son todos lo mismo
	{
	case GENERICA:
		memset(interfaz, 0, sizeof(t_interfaz));
		interfaz->nombre = nombre;
		interfaz->estado = DISPONIBLE;
		interfaz->tipo = GENERICA;
		interfaz->socket = 0;
		return interfaz;
		break;
	case STDIN:
		memset(interfaz, 0, sizeof(t_interfaz));
		interfaz->nombre = nombre;
		interfaz->estado = DISPONIBLE;
		interfaz->tipo = STDIN;
		interfaz->socket = 0;
		return interfaz;
		break;
	case STDOUT:
		memset(interfaz, 0, sizeof(t_interfaz));
		interfaz->nombre = nombre;
		interfaz->estado = DISPONIBLE;
		interfaz->tipo = STDOUT;
		interfaz->socket = 0;
		return interfaz;
		break;
	case DIALFS:
		memset(interfaz, 0, sizeof(t_interfaz));
		interfaz->nombre = nombre;
		interfaz->estado = DISPONIBLE;
		interfaz->tipo = DIALFS;
		interfaz->socket = 0;
		return interfaz;
		break;
	default:
		break;
	}
}
int handshake(int socket_cliente)
{
	size_t bytes;

	int32_t handshake = HS_IO; // PASAR ESTO A CONFIG en utils
	int32_t result;

	bytes = send(socket_cliente, &handshake, sizeof(int32_t), 0);
	bytes = recv(socket_cliente, &result, sizeof(int32_t), MSG_WAITALL);

	if (result != 0)
	{
		exit(-1);
	}
	return result;
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




void solicitar_escribir_memoria(void *datos, u_int32_t dir_fisica, int tam_r_datos, int pid)
{
	t_paquete *paquete = malloc(sizeof(t_paquete));

	paquete->codigo_operacion = WRITE_MEM;
	paquete->buffer = malloc(sizeof(t_buffer));
	paquete->buffer->size = sizeof(int) * 3 + tam_r_datos;
	paquete->buffer->stream = malloc(paquete->buffer->size);
	paquete->buffer->offset = 0;

	memcpy(paquete->buffer->stream + paquete->buffer->offset, &dir_fisica, sizeof(u_int32_t));
	paquete->buffer->offset += sizeof(int);

	memcpy(paquete->buffer->stream + paquete->buffer->offset, &tam_r_datos, sizeof(int));
	paquete->buffer->offset += sizeof(int);
	memcpy(paquete->buffer->stream + paquete->buffer->offset, &pid, sizeof(int));
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
	solicitar_escribir_memoria(sol->datos, sol->dir_fisica_base + sol->offset, sol->tam,sol->pid);
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
void populate_solicitudes(t_list *solicitudes, char *input_string) // gran nombre
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