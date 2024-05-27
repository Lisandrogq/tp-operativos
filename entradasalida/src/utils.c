#include "utils.h"
int contador[4]; //0 es el contador para generica
void enviar_operacion(int cod_op,char *mensaje, int socket_cliente) 
{
 	t_paquete *paquete = malloc(sizeof(t_paquete));

    paquete->codigo_operacion = cod_op;
    paquete->buffer = malloc(sizeof(t_buffer));
    paquete->buffer->size = strlen(mensaje) + 1;
    paquete->buffer->stream = malloc(paquete->buffer->size);
    memcpy(paquete->buffer->stream, mensaje, paquete->buffer->size);

    int bytes = paquete->buffer->size + 2 * sizeof(int);//ESTE *2 NO SE PUEDE TOCAR, ANDA ASÍ, PUNTO(.).

    void *a_enviar = serializar_paquete(paquete, bytes);

    send(socket_cliente, a_enviar, bytes, 0);

    free(a_enviar);
    eliminar_paquete(paquete);
}
void enviar_interfaz(int cod_op, t_interfaz interfaz, int socket_cliente){
	t_paquete *paquete = malloc(sizeof(t_paquete));

    paquete->codigo_operacion = cod_op;
    paquete->buffer = malloc(sizeof(t_buffer));
    paquete->buffer->size = sizeof(int) * 3 + sizeof(interfaz.largo);
    paquete->buffer->stream = malloc(paquete->buffer->size);
    paquete->buffer->offset = 0;

    memcpy(paquete->buffer->stream + paquete->buffer->offset, &interfaz.largo, sizeof(int));
    paquete->buffer->offset += sizeof(int);

    memcpy(paquete->buffer->stream + paquete->buffer->offset, &interfaz.tipo, sizeof(int));
    paquete->buffer->offset += sizeof(int);

    memcpy(paquete->buffer->stream + paquete->buffer->offset, interfaz.nombre, interfaz.largo);
    paquete->buffer->offset += sizeof(interfaz.largo);

    memcpy(paquete->buffer->stream + paquete->buffer->offset, &interfaz.estado, sizeof(int)); 
    int bytes = paquete->buffer->size + 2 * sizeof(int);

    void *a_enviar = serializar_paquete(paquete, bytes);

    send(socket_cliente, a_enviar, bytes, 0);

    free(a_enviar);
    eliminar_paquete(paquete);
}
t_interfaz *crear_estrcutura_io(int tipo){
	switch (tipo)
	{
	case GENERICA:
		t_interfaz *interfaz = malloc(sizeof(t_interfaz));
        char *provisorio = malloc(20);
		provisorio = "Int" + atoi(contador[GENERICA]); //Chequear esto
		memset(interfaz, 0, sizeof(t_interfaz));
		interfaz->largo = strlen(provisorio);
		interfaz->nombre = malloc(sizeof(interfaz->largo));
		interfaz->estado = DISPONIBLE;
		interfaz->tipo = GENERICA;
		interfaz->nombre = provisorio;
        free(provisorio);
		return interfaz;
		break;
	/*case GENERICA:
		t_interfaz *interfaz = malloc(sizeof(t_interfaz));
		memset(interfaz, 0, sizeof(t_interfaz));
		interfaz->estado = DISPONIBLE;
		interfaz->tipo = GENERICA;
		interfaz->nombre = "Int" + contador[GENERICA];
		interfaz->largo = strlen(interfaz->nombre);
		return interfaz;
		break;
	case GENERICA:
		t_interfaz *interfaz = malloc(sizeof(t_interfaz));
		memset(interfaz, 0, sizeof(t_interfaz));
		interfaz->estado = DISPONIBLE;
		interfaz->tipo = GENERICA;
		interfaz->nombre = "Int" + contador[GENERICA];
		interfaz->largo = strlen(interfaz->nombre);
		return interfaz;
		break;
	case GENERICA:
		t_interfaz *interfaz = malloc(sizeof(t_interfaz));
		memset(interfaz, 0, sizeof(t_interfaz));
		interfaz->estado = DISPONIBLE;
		interfaz->tipo = GENERICA;
		interfaz->nombre = "Int" + contador[GENERICA];
		interfaz->largo = strlen(interfaz->nombre);
		return interfaz;
		break;*/
	default:
		break;
	}

}
int handshake(int socket_cliente)
{
    size_t bytes;

    int32_t handshake = HS_IO;//PASAR ESTO A CONFIG en utils
    int32_t result;

    bytes = send(socket_cliente, &handshake, sizeof(int32_t), 0);
    bytes = recv(socket_cliente, &result, sizeof(int32_t), MSG_WAITALL);

    if (result != 0)
    {
        exit(-1);
    }
    return result;
}
