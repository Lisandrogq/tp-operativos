#include "utils.h"
extern t_log *logger;
int contador; // 0 es el contador para generica
char *nombre;
t_log *logger;
t_config *config;
int socket_kernel;
int socket_memoria;
int tiempo_Unidad_Trabajo;

t_list *decode_addresses_buffer(buffer_instr_io_t *buffer_instruccion, int *max_tam)
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
void enviar_operacion(int cod_op, char *mensaje, int socket_cliente)
{
	t_paquete *paquete = malloc(sizeof(t_paquete));

	paquete->codigo_operacion = cod_op;
	paquete->buffer = malloc(sizeof(t_buffer));
	paquete->buffer->size = strlen(mensaje) + 1;
	paquete->buffer->stream = malloc(paquete->buffer->size);
	memcpy(paquete->buffer->stream, mensaje, paquete->buffer->size);

	int bytes = paquete->buffer->size + 2 * sizeof(int); // ESTE *2 NO SE PUEDE TOCAR, ANDA ASÍ, PUNTO(.).

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
		interfaz->nombre = malloc(strlen(nombre) + 1);
		interfaz->nombre = nombre;
		interfaz->estado = DISPONIBLE;
		interfaz->tipo = GENERICA;
		interfaz->socket = 0;
		return interfaz;
		break;
	case STDIN:
		memset(interfaz, 0, sizeof(t_interfaz));
		interfaz->nombre = malloc(strlen(nombre) + 1);
		interfaz->nombre = nombre;
		interfaz->estado = DISPONIBLE;
		interfaz->tipo = STDIN;
		interfaz->socket = 0;
		return interfaz;
		break;
	case STDOUT:
		memset(interfaz, 0, sizeof(t_interfaz));
		interfaz->nombre = malloc(strlen(nombre) + 1);
		interfaz->nombre = nombre;
		interfaz->estado = DISPONIBLE;
		interfaz->tipo = STDOUT;
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
