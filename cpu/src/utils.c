#include "utils.h"
#include <errno.h>

t_dictionary *dictionary;
sem_t hay_proceso;
sem_t desalojar;
pcb_t *pcb_exec;
int socket_dispatch;
int socket_interrupt;

interrupcion_t*recibir_interrupcion(int socket_interrupt){
    t_buffer *buffer = malloc(sizeof(t_buffer));
	recv(socket_interrupt, &(buffer->size), sizeof(int), 0);
	buffer->stream = malloc(buffer->size);
	recv(socket_interrupt, buffer->stream, buffer->size, 0);

	interrupcion_t *interrupcion = malloc(sizeof(interrupcion_t));
	memset(interrupcion, 0, sizeof(interrupcion_t));

	void *stream = buffer->stream;//esto hace q no se pueda liberar la memoria de stream
	memcpy(&(interrupcion->pid), stream, sizeof(int));
	stream += sizeof(int);
	memcpy(&(interrupcion->motivo), stream, sizeof(int));
	stream += sizeof(int);
	
	free(buffer->stream);
	free(buffer);
	return interrupcion;	
}
void execute_set(char *nombre_r_destino, int valor)
{
    if (strlen(nombre_r_destino) == 3 || !strcmp(nombre_r_destino, "SI") || !strcmp(nombre_r_destino, "DI")) // caso registros de 4 byte
    {
        u_int32_t *r_destino = dictionary_get(dictionary, nombre_r_destino);
        *r_destino = valor;
    }
    else if (strlen(nombre_r_destino) == 2) // caso registros de 1 byte
    {
        u_int8_t *r_destino = dictionary_get(dictionary, nombre_r_destino);
        *r_destino = valor;
    }
}
void execute_sum(char *nombre_r_destino, char *nombre_r_origen)
{
    int sumando = 0;
    if (strlen(nombre_r_origen) == 3 || !strcmp(nombre_r_origen, "SI") || !strcmp(nombre_r_origen, "DI")) // caso registros de 4 byte
    {
        u_int32_t *r_origen = dictionary_get(dictionary, nombre_r_origen);
        sumando = *r_origen;
    }
    else if (strlen(nombre_r_origen) == 2) // caso registros de 1 byte
    {
        u_int8_t *r_origen = dictionary_get(dictionary, nombre_r_origen);
        sumando = *r_origen;
    }
    if (strlen(nombre_r_destino) == 3 || !strcmp(nombre_r_destino, "SI") || !strcmp(nombre_r_destino, "DI")) // caso registros de 4 byte
    {
        u_int32_t *r_destino = dictionary_get(dictionary, nombre_r_destino);
        *r_destino = *r_destino + sumando;
    }
    else if (strlen(nombre_r_destino) == 2) // caso registros de 1 byte
    {
        u_int8_t *r_destino = dictionary_get(dictionary, nombre_r_destino);
        *r_destino = *r_destino + sumando;
    }
}
void execute_sub(char *nombre_r_destino, char *nombre_r_origen) // CREO QUE INT8 ES UNSIGNED, NO CREO Q SEA UN PROBLEMA EN LAS PRUEBAS PERO XD
{
    int sustraendo = 0;
    if (strlen(nombre_r_origen) == 3 || !strcmp(nombre_r_origen, "SI") || !strcmp(nombre_r_origen, "DI")) // caso registros de 4 byte
    {
        u_int32_t *r_origen = dictionary_get(dictionary, nombre_r_origen);
        sustraendo = *r_origen;
    }
    else if (strlen(nombre_r_origen) == 2) // caso registros de 1 byte
    {
        u_int8_t *r_origen = dictionary_get(dictionary, nombre_r_origen);
        sustraendo = *r_origen;
    }
    if (strlen(nombre_r_destino) == 3 || !strcmp(nombre_r_destino, "SI") || !strcmp(nombre_r_destino, "DI")) // caso registros de 4 byte
    {
        u_int32_t *r_destino = dictionary_get(dictionary, nombre_r_destino);
        *r_destino = *r_destino - sustraendo;
    }
    else if (strlen(nombre_r_destino) == 2) // caso registros de 1 byte
    {
        u_int8_t *r_destino = dictionary_get(dictionary, nombre_r_destino);
        *r_destino = *r_destino - sustraendo;
    }
}
void execute_jnz(char *nombre_r, uint32_t nuevo_pc, registros_t *contexto) // habría que ver si hay alguna forma para no pasar el contexto
{

    if (strlen(nombre_r) == 3 || !strcmp(nombre_r, "SI") || !strcmp(nombre_r, "DI")) // caso registros de 4 byte
    {
        u_int32_t *registro = dictionary_get(dictionary, nombre_r);
        if (*registro != 0)
            contexto->PC = nuevo_pc;
    }
    else if (strlen(nombre_r) == 2) // caso registros de 1 byte
    {
        u_int8_t *registro = dictionary_get(dictionary, nombre_r);
        if (*registro != 0)
            contexto->PC = nuevo_pc;
    }
}
int esperar_cliente_cpu(int socket_servidor)
{
    // Aceptamos un nuevo cliente
    int socket_cliente = accept(socket_servidor, NULL, NULL);
    return socket_cliente;
}

void enviar_operacion(int cod_op, char *mensaje, int socket_cliente)
{
    // vamos a tener q retocar esta funcion cuando queramos mandar structs o cosas mas genericas(no solo strings).

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

int handshake(int socket_cliente)
{
    size_t bytes;

    int32_t handshake = HS_CPU; // PASAR ESTO A CONFIG en utils
    int32_t result;

    bytes = send(socket_cliente, &handshake, sizeof(int32_t), 0);
    bytes = recv(socket_cliente, &result, sizeof(int32_t), MSG_WAITALL);

    if (result != 0)
    {
        exit(-1);
    }
    return result;
}
void devolver_pcb_por_exit(int motivo_desalojo, pcb_t pcb, int socket_cliente)
{
    pcb.registros->SI = 99;
    pcb.registros->DI = 99;

    t_paquete *paquete = malloc(sizeof(t_paquete));
    paquete->buffer = malloc(sizeof(t_buffer));
    paquete->codigo_operacion = DISPATCH_RESPONSE;
    paquete->buffer->size = sizeof(int) * 3 + sizeof(registros_t);
    paquete->buffer->stream = malloc(paquete->buffer->size);
    paquete->buffer->offset = 0;

    memcpy(paquete->buffer->stream + paquete->buffer->offset, &pcb.pid, sizeof(int));
    paquete->buffer->offset += sizeof(int);

    memcpy(paquete->buffer->stream + paquete->buffer->offset, &pcb.quantum, sizeof(int));
    paquete->buffer->offset += sizeof(int);

    memcpy(paquete->buffer->stream + paquete->buffer->offset, pcb.registros, sizeof(registros_t));
    paquete->buffer->offset += sizeof(registros_t);

    memcpy(paquete->buffer->stream + paquete->buffer->offset, &motivo_desalojo, sizeof(int));
    paquete->buffer->offset += sizeof(int);

    int bytes = paquete->buffer->size + 2 * sizeof(int); //=tam(buffer)+tam(codop)+tam(buffer->size)

    void *a_enviar = serializar_paquete(paquete, bytes);

    send(socket_cliente, a_enviar, bytes, 0);

    free(a_enviar);
}

void devolver_pcb(int motivo_desalojo, pcb_t pcb, int socket_cliente, t_strings_instruccion *instruccion)
{
    pcb.registros->SI = 99;
    pcb.registros->DI = 99;
    int tam_instruccion = instruccion->tamcod + instruccion->tamp1 + instruccion->tamp2 + instruccion->tamp3 + instruccion->tamp4 + instruccion->tamp5;
    char *p1 = "Int1";

    int tam_p1 = strlen(p1) + 1;
    int tiempo = 12;

    t_paquete *paquete = malloc(sizeof(t_paquete));
    paquete->buffer = malloc(sizeof(t_buffer));
    paquete->codigo_operacion = DISPATCH_RESPONSE;
    paquete->buffer->size = sizeof(int) * 3 + sizeof(registros_t) + tam_instruccion + 6 * sizeof(int);
    paquete->buffer->stream = malloc(paquete->buffer->size);
    paquete->buffer->offset = 0;

    memcpy(paquete->buffer->stream + paquete->buffer->offset, &pcb.pid, sizeof(int));
    paquete->buffer->offset += sizeof(int);

    memcpy(paquete->buffer->stream + paquete->buffer->offset, &pcb.quantum, sizeof(int));
    paquete->buffer->offset += sizeof(int);

    memcpy(paquete->buffer->stream + paquete->buffer->offset, pcb.registros, sizeof(registros_t)); // capaz esto tiene quilombo con el padding
    paquete->buffer->offset += sizeof(registros_t);                                                // y hay que agregar de a uno

    memcpy(paquete->buffer->stream + paquete->buffer->offset, &motivo_desalojo, sizeof(int));
    paquete->buffer->offset += sizeof(int);
    // comienzo serialiacion de instruccion:

    // si alguno de los tams es 0, no se escribe nada en ese parametro osea queda todo 0
    memcpy(paquete->buffer->stream + paquete->buffer->offset, &instruccion->tamcod, sizeof(int));
    paquete->buffer->offset += sizeof(int);
    memcpy(paquete->buffer->stream + paquete->buffer->offset, instruccion->cod_instruccion, instruccion->tamcod);
    paquete->buffer->offset += instruccion->tamcod;

    memcpy(paquete->buffer->stream + paquete->buffer->offset, &instruccion->tamp1, sizeof(int));
    paquete->buffer->offset += sizeof(int);
    memcpy(paquete->buffer->stream + paquete->buffer->offset, instruccion->p1, instruccion->tamp1);
    paquete->buffer->offset += instruccion->tamp1;

    memcpy(paquete->buffer->stream + paquete->buffer->offset, &instruccion->tamp2, sizeof(int));
    paquete->buffer->offset += sizeof(int);
    memcpy(paquete->buffer->stream + paquete->buffer->offset, instruccion->p2, instruccion->tamp2);
    paquete->buffer->offset += instruccion->tamp2;

    memcpy(paquete->buffer->stream + paquete->buffer->offset, &instruccion->tamp3, sizeof(int));
    paquete->buffer->offset += sizeof(int);
    memcpy(paquete->buffer->stream + paquete->buffer->offset, instruccion->p3, instruccion->tamp3);
    paquete->buffer->offset += instruccion->tamp3;

    memcpy(paquete->buffer->stream + paquete->buffer->offset, &instruccion->tamp4, sizeof(int));
    paquete->buffer->offset += sizeof(int);
    memcpy(paquete->buffer->stream + paquete->buffer->offset, instruccion->p4, instruccion->tamp4);
    paquete->buffer->offset += instruccion->tamp4;

    memcpy(paquete->buffer->stream + paquete->buffer->offset, &instruccion->tamp5, sizeof(int));
    paquete->buffer->offset += sizeof(int);
    memcpy(paquete->buffer->stream + paquete->buffer->offset, instruccion->p5, instruccion->tamp5);
    paquete->buffer->offset += instruccion->tamp5;
    //

    int bytes = paquete->buffer->size + 2 * sizeof(int); //=tam(buffer)+tam(codop)+tam(buffer->size)

    void *a_enviar = serializar_paquete(paquete, bytes);

    send(socket_cliente, a_enviar, bytes, 0);

    free(a_enviar);
}

// Server
t_log *logger;

t_dictionary *inicializar_diccionario(registros_t *contexto)
{
    dictionary_put(dictionary, "AX", &(contexto->AX));
    dictionary_put(dictionary, "BX", &(contexto->BX));
    dictionary_put(dictionary, "CX", &(contexto->CX));
    dictionary_put(dictionary, "DX", &(contexto->DX));
    dictionary_put(dictionary, "EAX", &(contexto->EAX));
    dictionary_put(dictionary, "EBX", &(contexto->EBX));
    dictionary_put(dictionary, "ECX", &(contexto->ECX));
    dictionary_put(dictionary, "EDX", &(contexto->EDX));
    dictionary_put(dictionary, "SI", &(contexto->SI));
    dictionary_put(dictionary, "DI", &(contexto->DI));
}

void *client_handler_dispatch(int socket_cliente)
{
    int modulo = handshake_Server(socket_cliente);
    switch (modulo) // se debería ejecutar un handler para cada modulo
    {
    case HS_KERNEL:
        log_info(logger, "se conecto el modulo kernel(dispatch)");
        break;
    default:
        log_warning(logger, "Cliente desconocido por cpu server.");
        return -1;
    }

    bool conexion_terminada = false;
    while (!conexion_terminada)
    {
        int cod_op = recibir_operacion(socket_cliente);
        // log_warning(logger, "recibí la operacion: codop:%i", cod_op);
        switch (cod_op)
        {
        case OPERACION_KERNEL_1:
            // capaz habria q cambiar el nombre de recibir_(...) a manejar_(...)
            recibir_operacion1(socket_cliente);
            break;
        case OPERACION_CPU_1:
            // capaz habria q cambiar el nombre de recibir_(...) a manejar_(...)
            recibir_operacion1(socket_cliente);
            break;
        case OPERACION_IO_1:
            // capaz habria q cambiar el nombre de recibir_(...) a manejar_(...)
            recibir_operacion1(socket_cliente);
            break;
        case MENSAJE:
            // capaz habria q cambiar el nombre de recibir_(...) a manejar_(...)
            recibir_mensaje(socket_cliente);
            break;
        case DISPATCH:
            // TODO: liberar pcb sino es la primera ejecucion
            log_debug(logger, "Se recibio el proceso a ejecutar por dispatch");
            pcb_t *pcb = recibir_paquete(socket_cliente);
            sem_post(&hay_proceso);
            pcb_exec = pcb;
            inicializar_diccionario(pcb_exec->registros);
            break;
        case -1:
            log_info(logger, "Se desconecto algun cliente");
            conexion_terminada = true;
            break;
        default:
            log_warning(logger, "Operacion desconocida. No quieras meter la pata");
            break;
        }
    }
    close(socket_cliente);
}

void *client_handler_interrupt(int socket_cliente)
{
    int modulo = handshake_Server(socket_cliente);
    switch (modulo) // se debería ejecutar un handler para cada modulo
    {
    case HS_KERNEL:
        log_info(logger, "se conecto el modulo kernel(interrupt)");
        break;
    default:
        log_warning(logger, "Cliente desconocido por cpu server.");
        return -1;
    }

    bool conexion_terminada = false;
   
    close(socket_cliente);
}

int handshake_Server(int socket_cliente)
{

    size_t bytes;

    int32_t handshake;
    int32_t resultOk = 0;
    int32_t resultError = -1;

    bytes = recv(socket_cliente, &handshake, sizeof(int32_t), MSG_WAITALL);

    if (handshake == HS_KERNEL || handshake == HS_KERNEL) // handshake que enviará el kernel
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
void recibir_operacion1(int socket_cliente)
{
    int size;
    char *buffer = recibir_buffer(&size, socket_cliente);
    log_info(logger, "Me llego la operacion uno, la informacion enviada fue: %s", buffer);
    free(buffer);
}
void log_instruccion_ejecutada(t_strings_instruccion *palabras)
{
    log_info(logger, "PID: %i - Ejecutando: %s - %s %s %s %s %s", pcb_exec->pid, palabras->cod_instruccion, palabras->p1, palabras->p2, palabras->p3, palabras->p4, palabras->p5);
}
void log_fetch_instruccion()
{
    log_info(logger, "PID: %i - FETCH - Program Counter: %i", pcb_exec->pid, pcb_exec->registros->PC);
}