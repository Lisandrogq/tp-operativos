#include <utils/utils_generales.h>
#include <errno.h>
#include <commons/collections/list.h>

void liberar_solicitud(solicitud_unitaria_t *sol)
{
    free(sol->datos);
    free(sol);
}
void liberar_y_eliminar_solicitudes(t_list *solicitudes)
{
    list_destroy_and_destroy_elements(solicitudes, liberar_solicitud);
}
void decir_hola(char *quien)
{
    printf("Hola desde %s!!\n", quien);
}
void crear_buffer(t_paquete *paquete)
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

pcb_t *recibir_paquete(int socket_cliente)
{
    t_paquete *paquete = malloc(sizeof(t_paquete));
    paquete->buffer = malloc(sizeof(t_buffer));

    recv(socket_cliente, &(paquete->buffer->size), sizeof(int), 0);
    paquete->buffer->stream = malloc(paquete->buffer->size);
    recv(socket_cliente, paquete->buffer->stream, paquete->buffer->size, 0);

    pcb_t *pcb = malloc(sizeof(pcb_t)); //Free en cpu
    pcb->registros = malloc(sizeof(registros_t));
    void *stream = paquete->buffer->stream;
    memcpy(&(pcb->pid), stream, sizeof(int));
    stream += sizeof(int);
    memcpy(&(pcb->quantum), stream, sizeof(int));
    stream += sizeof(int);
    memcpy(pcb->registros, stream, sizeof(registros_t));
    stream += sizeof(registros_t);
    memcpy(&(pcb->state), stream, sizeof(state_t));
    eliminar_paquete(paquete);
    return pcb;
}
int crear_conexion(char *ip, char *puerto, t_log *logger)
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
    if (socket_cliente == -1)
    {
        log_error(logger, "Error at socket: %s\n", strerror(errno));
        return -1;
    }

    // Ahora que tenemos el socket, vamos a conectarlo

    freeaddrinfo(server_info); /// creo q esto deberia ir al final, antes del return
    int result = connect(socket_cliente, server_info->ai_addr, server_info->ai_addrlen);
    if (result == -1)
    {
        log_error(logger, "Error at connect: %s\n", strerror(errno));
        return -1;
    }
    return socket_cliente;
}

int esperar_cliente(int socket_servidor, void *client_handler(void *))
{
    while (1) // reemplazar por condicion de terminacion de sistema/modulo
    {
        pthread_t thread;
        int *socket_cliente = malloc(sizeof(int)); // cuando liberar esto, en el handler??
        // Aceptamos un nuevo cliente
        *socket_cliente = accept(socket_servidor, NULL, NULL);
        pthread_create(&thread,
                       NULL,
                       client_handler,
                       socket_cliente);
        pthread_detach(thread); // creo q debería ser detach pq la condicion de terminacion de sistema es externa
    }
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

int iniciar_servidor(char *PUERTO, t_log *logger)
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
    if (socket_servidor == -1)
    {
        log_error(logger, "Error at socket: %s\n", strerror(errno));
        return -1;
    }
    // Asociamos el socket a un puerto
    int resultado_bind = bind(socket_servidor, server_info->ai_addr, server_info->ai_addrlen);
    if (resultado_bind == -1)
    {
        log_error(logger, "Error at bind: %s\n", strerror(errno));
        return -1;
    }
    // Escuchamos las conexiones entrantes
    int resultado_listen = listen(socket_servidor, SOMAXCONN); // error en el listen
    if (resultado_listen == -1)
    {
        log_error(logger, "Error at listen: %s\n", strerror(errno));
        return -1;
    }
    freeaddrinfo(server_info);

    return socket_servidor;
}
void eliminar_pcb(pcb_t *pcb)
{
    free(pcb->registros);
    free(pcb);
}
void eliminar_paquete(t_paquete *paquete)
{
    free(paquete->buffer->stream);
    free(paquete->buffer);
    free(paquete);
}
void liberar_conexion(int socket_cliente)
{
    close(socket_cliente);
}
void agregar_a_paquete(t_paquete *paquete, void *valor, int tamanio)
{
    paquete->buffer->stream = realloc(paquete->buffer->stream, paquete->buffer->size + tamanio + sizeof(int));

    memcpy(paquete->buffer->stream + paquete->buffer->size, &tamanio, sizeof(int));
    memcpy(paquete->buffer->stream + paquete->buffer->size + sizeof(int), valor, tamanio);

    paquete->buffer->size += tamanio + sizeof(int);
}
void enviar_paquete(t_paquete *paquete, int socket_cliente)
{
    int bytes = paquete->buffer->size + 2 * sizeof(int);
    void *a_enviar = serializar_paquete(paquete, bytes);

    send(socket_cliente, a_enviar, bytes, 0);

    free(a_enviar);
}