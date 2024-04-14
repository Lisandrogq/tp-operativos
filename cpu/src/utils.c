#include "utils.h"

//Client//
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

int crear_conexion(char *ip, char *puerto)
{
    struct addrinfo hints;
    struct addrinfo *server_info;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    getaddrinfo(ip, puerto, &hints, &server_info);

    // Ahora vamos a crear el socket.
    int socket_cliente = socket(server_info->ai_family, server_info->ai_socktype, server_info->ai_protocol);

    // Ahora que tenemos el socket, vamos a conectarlo

    freeaddrinfo(server_info); /// creo q esto deberia ir al final, antes del return
    connect(socket_cliente, server_info->ai_addr, server_info->ai_addrlen);

    return socket_cliente;
}

void enviar_mensaje(char *mensaje, int socket_cliente)
{
    t_paquete *paquete = malloc(sizeof(t_paquete));

    paquete->codigo_operacion = MENSAJE;
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

    int32_t handshake = 1;//PASAR ESTO A CONFIG en utils
    int32_t result;

    bytes = send(socket_cliente, &handshake, sizeof(int32_t), 0);
    bytes = recv(socket_cliente, &result, sizeof(int32_t), MSG_WAITALL);

    if (result != 0)
    {
    exit(-1);
    }
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

    void eliminar_paquete(t_paquete * paquete)
    {
        free(paquete->buffer->stream);
        free(paquete->buffer);
        free(paquete);
    }

    void liberar_conexion(int socket_cliente)
    {
        close(socket_cliente);
    }

//Server//
t_log *logger;


int iniciar_servidor(void)
{
	int socket_servidor;

	struct addrinfo hints, *server_info; //, *p;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;

	getaddrinfo(NULL, PUERTO, &hints, &server_info);

	// Creamos el socket de escucha del servidor
	socket_servidor = socket(server_info->ai_family, server_info->ai_socktype, server_info->ai_protocol);

	// Asociamos el socket a un puerto
	bind(socket_servidor, server_info->ai_addr, server_info->ai_addrlen);
	// Escuchamos las conexiones entrantes
	listen(socket_servidor, SOMAXCONN);
	freeaddrinfo(server_info);
	log_info(logger, "Listo para escuchar a otro modulo"); // El profe puse log_trace

	return socket_servidor;
}

int esperar_cliente(int socket_servidor)
{
	// Aceptamos un nuevo cliente
	int socket_cliente = accept(socket_servidor, NULL, NULL);
	//log_info(logger, "Se conecto un modulo!");

	return socket_cliente;
}

int recibir_operacion(int socket_cliente)
{
	int cod_op;
	if (recv(socket_cliente, &cod_op, sizeof(int), MSG_WAITALL) > 0)
	{
		return cod_op;
	}
	else
	{
		close(socket_cliente);
		return -1;
	}
}

int handshake_Server(int socket_cliente)
{

	size_t bytes;

	int32_t handshake;
	int32_t resultOk = 0;
	int32_t resultError = -1;

	bytes = recv(socket_cliente, &handshake, sizeof(int32_t), MSG_WAITALL);
	
	if (handshake>=0 && handshake<=3)
	{
		bytes = send(socket_cliente, &resultOk, sizeof(int32_t), 0);
	}
	else
	{
		bytes = send(socket_cliente, &resultError, sizeof(int32_t), 0);
	}
	return handshake;
}

void recibir_mensaje(int socket_cliente)
{
	int size;
	char *buffer = recibir_buffer(&size, socket_cliente);
	log_info(logger, "Me llego el mensaje %s", buffer);
	free(buffer);
}

void *recibir_buffer(int *size, int socket_cliente)
{
	void *buffer;

	recv(socket_cliente, size, sizeof(int), MSG_WAITALL);
	buffer = malloc(*size);
	recv(socket_cliente, buffer, *size, MSG_WAITALL);

	return buffer;
}
