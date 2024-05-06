#include "utils.h"

void *serializar_paquete(t_paquete *paquete, int bytes)
{
    void *magic = malloc(bytes);
    int desplazamiento = 0;

    memcpy(magic + desplazamiento, &(paquete->codigo_operacion), sizeof(int));
    desplazamiento += sizeof(int);
    memcpy(magic + desplazamiento, &(paquete->buffer->size), sizeof(int));
    desplazamiento += sizeof(int);
    memcpy(magic + desplazamiento, paquete->buffer->stream, paquete->buffer->size);
    desplazamiento += paquete->buffer->size;

    return magic;
}

void enviar_operacion(int cod_op,char *mensaje, int socket_cliente) 
{
//vamos a tener q retocar esta funcion cuando queramos mandar structs o cosas mas genericas(no solo strings).

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

    
    void crear_buffer(t_paquete * paquete)
    {
        paquete->buffer = malloc(sizeof(t_buffer));
        paquete->buffer->size = 0;
        paquete->buffer->stream = NULL;
    }

    t_paquete *crear_paquete(void)
    {
        t_paquete *paquete = malloc(sizeof(t_paquete));
        paquete->codigo_operacion = PAQUETE;
        crear_buffer(paquete);
        return paquete;
    }

    void agregar_a_paquete(t_paquete * paquete, void *valor, int tamanio)
    {
        paquete->buffer->stream = realloc(paquete->buffer->stream, paquete->buffer->size + tamanio + sizeof(int));

        memcpy(paquete->buffer->stream + paquete->buffer->size, &tamanio, sizeof(int));
        memcpy(paquete->buffer->stream + paquete->buffer->size + sizeof(int), valor, tamanio);

        paquete->buffer->size += tamanio + sizeof(int);
    }

    void enviar_paquete(t_paquete * paquete, int socket_cliente)
    {
        int bytes = paquete->buffer->size + 2 * sizeof(int);
        void *a_enviar = serializar_paquete(paquete, bytes);

        send(socket_cliente, a_enviar, bytes, 0);

        free(a_enviar);
    }

    void liberar_conexion(int socket_cliente)
    {
        close(socket_cliente);
    }
