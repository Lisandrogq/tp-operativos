#include "utils.h"
#include <errno.h>
#include <commons/temporal.h>

t_dictionary *dic_p_registros; // tiene punteros a los registros
t_dictionary *dic_tam_registros;
t_list *tlb_list;
sem_t hay_proceso;
pcb_t *pcb_exec;
int socket_dispatch;
int socket_interrupt;
int socket_memoria;
int CANTIDAD_ENTRADAS_TLB;
char *ALGORITMO_TLB;
t_temporal *cronometro_lru;
int tam_pagina;
interrupcion_t *recibir_interrupcion(int socket_interrupt)
{
    t_buffer *buffer = malloc(sizeof(t_buffer));
    recv(socket_interrupt, &(buffer->size), sizeof(int), 0);
    buffer->stream = malloc(buffer->size);
    recv(socket_interrupt, buffer->stream, buffer->size, 0);

    interrupcion_t *interrupcion = malloc(sizeof(interrupcion_t));
    memset(interrupcion, 0, sizeof(interrupcion_t));

    void *stream = buffer->stream; // esto hace q no se pueda liberar la memoria de stream
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
    if (strlen(nombre_r_destino) == 3 || !strcmp(nombre_r_destino, "SI") || !strcmp(nombre_r_destino, "DI")|| !strcmp(nombre_r_destino, "PC")) // caso registros de 4 byte
    {
        u_int32_t *r_destino = dictionary_get(dic_p_registros, nombre_r_destino);
        *r_destino = valor;
    }
    else if (strlen(nombre_r_destino) == 2) // caso registros de 1 byte
    {
        u_int8_t *r_destino = dictionary_get(dic_p_registros, nombre_r_destino);
        *r_destino = valor; 
    }
}
void execute_sum(char *nombre_r_destino, char *nombre_r_origen)
{
    int sumando = 0;
    if (strlen(nombre_r_origen) == 3 || !strcmp(nombre_r_origen, "SI") || !strcmp(nombre_r_origen, "DI")||!strcmp(nombre_r_origen, "PC")) // caso registros de 4 byte
    {
        u_int32_t *r_origen = dictionary_get(dic_p_registros, nombre_r_origen);
        sumando = *r_origen;
    }
    else if (strlen(nombre_r_origen) == 2) // caso registros de 1 byte
    {
        u_int8_t *r_origen = dictionary_get(dic_p_registros, nombre_r_origen);
        sumando = *r_origen;
    }
    if (strlen(nombre_r_destino) == 3 || !strcmp(nombre_r_destino, "SI") || !strcmp(nombre_r_destino, "DI")|| !strcmp(nombre_r_destino, "PC")) // caso registros de 4 byte
    {
        u_int32_t *r_destino = dictionary_get(dic_p_registros, nombre_r_destino);
        *r_destino = *r_destino + sumando;
    }
    else if (strlen(nombre_r_destino) == 2) // caso registros de 1 byte
    {
        u_int8_t *r_destino = dictionary_get(dic_p_registros, nombre_r_destino);
        *r_destino = *r_destino + sumando;
    }
}
void execute_sub(char *nombre_r_destino, char *nombre_r_origen) // CREO QUE INT8 ES UNSIGNED, NO CREO Q SEA UN PROBLEMA EN LAS PRUEBAS PERO XD
{
    int sustraendo = 0;
    if (strlen(nombre_r_origen) == 3 || !strcmp(nombre_r_origen, "SI") || !strcmp(nombre_r_origen, "DI")|| !strcmp(nombre_r_origen, "PC")) // caso registros de 4 byte
    {
        u_int32_t *r_origen = dictionary_get(dic_p_registros, nombre_r_origen);
        sustraendo = *r_origen;
    }
    else if (strlen(nombre_r_origen) == 2) // caso registros de 1 byte
    {
        u_int8_t *r_origen = dictionary_get(dic_p_registros, nombre_r_origen);
        sustraendo = *r_origen;
    }
    if (strlen(nombre_r_destino) == 3 || !strcmp(nombre_r_destino, "SI") || !strcmp(nombre_r_destino, "DI")|| !strcmp(nombre_r_destino, "PC")) // caso registros de 4 byte
    {
        u_int32_t *r_destino = dictionary_get(dic_p_registros, nombre_r_destino);
        *r_destino = *r_destino - sustraendo;
    }
    else if (strlen(nombre_r_destino) == 2) // caso registros de 1 byte
    {
        u_int8_t *r_destino = dictionary_get(dic_p_registros, nombre_r_destino);
        *r_destino = *r_destino - sustraendo;
    }
}
void execute_jnz(char *nombre_r, uint32_t nuevo_pc, registros_t *contexto) // habría que ver si hay alguna forma para no pasar el contexto
{

    if (strlen(nombre_r) == 3 || !strcmp(nombre_r, "SI") || !strcmp(nombre_r, "DI")|| !strcmp(nombre_r, "PC")) // caso registros de 4 byte
    {
        u_int32_t *registro = dictionary_get(dic_p_registros, nombre_r);
        if (*registro != 0)
            contexto->PC = nuevo_pc;
    }
    else if (strlen(nombre_r) == 2) // caso registros de 1 byte
    {
        u_int8_t *registro = dictionary_get(dic_p_registros, nombre_r);
        if (*registro != 0)
            contexto->PC = nuevo_pc;
    }
}

void execute_mov_in(t_list *solicitudes, void *datos)
{
    list_map(solicitudes, execute_unitary_mov_in);
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
int execute_mov_out(t_list *solicitudes)
{
    t_list_iterator *iterator = list_iterator_create(solicitudes);
    int status = MEM_W_OK;
    while (list_iterator_has_next(iterator))
    {
        solicitud_unitaria_t *sol = list_iterator_next(iterator);
        int status = execute_unitary_mov_out(sol);

        if (status != MEM_W_OK) // NO SE INDICA Q HACER ANTE ESTOS CASOS(FINALZIAR PROCESO???)
        {
            log_error(logger, "ERROR EN LA ESCRITURA NUMERO %i", list_iterator_index(iterator));
            return MEM_W_NO_OK;
        }
    }
    return status;
}

solicitud_unitaria_t *execute_unitary_mov_in(solicitud_unitaria_t *sol)
{
    u_int32_t dir_fisica = sol->dir_fisica_base + sol->offset;
    int tam_r_datos = sol->tam;

    solicitar_leer_memoria(dir_fisica, tam_r_datos, sol->pid);
    int cod_op = recibir_operacion(socket_memoria); // waitall y codop
    void *datos_obtenidos = recibir_datos_leidos();
    memcpy(sol->datos, datos_obtenidos, sol->tam);
    int *logeable = malloc(sizeof(int));
    memset(logeable, 0, sizeof(int));
    memcpy(logeable, datos_obtenidos, sol->tam);
    log_info(logger, "PID: %i - Acción: LEER - Dirección Física: %i - Valor: %i", sol->pid, sol->dir_fisica_base + sol->offset, *logeable);
    free(logeable);
    return sol;
}
int execute_unitary_mov_out(solicitud_unitaria_t *sol)
{
    solicitar_escribir_memoria(sol->datos, sol->dir_fisica_base + sol->offset, sol->tam, sol->pid);
    int cod_op = recibir_operacion(socket_memoria); // waitall y codop
    int status_escritura = recibir_status_escritura();
    int *logeable = malloc(sizeof(int));
    memset(logeable, 0, sizeof(int));
    memcpy(logeable, sol->datos, sol->tam);
    log_info(logger, "PID: %i - Acción: ESCRIBIR - Dirección Física: %i - Valor: %i", pcb_exec->pid, sol->dir_fisica_base + sol->offset, *logeable);
    free(logeable);

    return status_escritura;
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

    int bytes = paquete->buffer->size + 2 * sizeof(int); 

    void *a_enviar = serializar_paquete(paquete, bytes);

    send(socket_cliente, a_enviar, bytes, 0);

    free(a_enviar);
    eliminar_paquete(paquete);
}

int handshake(int socket_cliente)
{
    size_t bytes;

    int32_t handshake = HS_CPU; // PASAR ESTO A CONFIG en utils
    int32_t tam_pagina;

    bytes = send(socket_cliente, &handshake, sizeof(int32_t), 0);
    bytes = recv(socket_cliente, &tam_pagina, sizeof(int32_t), MSG_WAITALL);

    if (tam_pagina == -1)
    {
        exit(-1);
    }
    return tam_pagina;
}

buffer_instr_io_t *serializar_truncate_sol(char *nombre, int bytes)
{
    int op = IO_FS_TRUNCATE;
    int tam_nombre = strlen(nombre);
    buffer_instr_io_t *buffer_instruccion = malloc(sizeof(buffer_instr_io_t));
    buffer_instruccion->size = tam_nombre + sizeof(int32_t) * 3;
    buffer_instruccion->buffer = malloc(buffer_instruccion->size);
    int offset = 0;
    memcpy(buffer_instruccion->buffer + offset, &op, sizeof(int32_t)); // CREATE/DELETE
    offset += sizeof(int32_t);
    memcpy(buffer_instruccion->buffer + offset, &(tam_nombre), sizeof(int32_t));
    offset += sizeof(int32_t);
    memcpy(buffer_instruccion->buffer + offset, nombre, tam_nombre);
    offset += tam_nombre;
    memcpy(buffer_instruccion->buffer + offset, &bytes, sizeof(int32_t));
    return buffer_instruccion;
}
buffer_instr_io_t *serializar_nombre(char *nombre, int op)
{
    int tam_nombre = strlen(nombre);
    buffer_instr_io_t *buffer_instruccion = malloc(sizeof(buffer_instr_io_t));
    buffer_instruccion->size = tam_nombre + sizeof(int32_t) * 2;
    buffer_instruccion->buffer = malloc(buffer_instruccion->size);
    int offset = 0;
    memcpy(buffer_instruccion->buffer + offset, &op, sizeof(int32_t)); // CREATE/DELETE
    offset += sizeof(int32_t);
    memcpy(buffer_instruccion->buffer + offset, &(tam_nombre), sizeof(int32_t));
    offset += sizeof(int32_t);
    memcpy(buffer_instruccion->buffer + offset, nombre, tam_nombre);
    return buffer_instruccion;
}
buffer_instr_io_t *serializar_solicitudes(t_list *solicitudes, int max_tam) // en io se va a generar elementos hasta q se llegue a size
{
    buffer_instr_io_t *buffer_instruccion = malloc(sizeof(buffer_instr_io_t));
    buffer_instruccion->size = 4 * list_size(solicitudes) * sizeof(u_int32_t);
    buffer_instruccion->buffer = malloc(buffer_instruccion->size);
    t_list_iterator *iterator = list_iterator_create(solicitudes);
    int offset = 0;

    while (list_iterator_has_next(iterator))
    {
        solicitud_unitaria_t *sol = list_iterator_next(iterator);

        memcpy(buffer_instruccion->buffer + offset, &(sol->dir_fisica_base), sizeof(u_int32_t));
        offset += sizeof(u_int32_t);
        memcpy(buffer_instruccion->buffer + offset, &(sol->offset), sizeof(u_int32_t));
        offset += sizeof(u_int32_t);
        memcpy(buffer_instruccion->buffer + offset, &(sol->tam), sizeof(u_int32_t));
        offset += sizeof(u_int32_t);
        memcpy(buffer_instruccion->buffer + offset, &(sol->pid), sizeof(u_int32_t));
        offset += sizeof(u_int32_t);
    }

    list_iterator_destroy(iterator);
    return buffer_instruccion;
}
buffer_instr_io_t *serializar_solicitudes_fs(t_list *solicitudes, char *nombre, int puntero_archivo, int OPERACION)
{
    int tam_nombre = strlen(nombre);
    buffer_instr_io_t *buffer_instruccion = malloc(sizeof(buffer_instr_io_t));
    buffer_instruccion->size = 4 * list_size(solicitudes) * sizeof(u_int32_t) + 3 * sizeof(u_int32_t) + tam_nombre;
    buffer_instruccion->buffer = malloc(buffer_instruccion->size);
    t_list_iterator *iterator = list_iterator_create(solicitudes);
    int offset = 0;
    memcpy(buffer_instruccion->buffer + offset, &OPERACION, sizeof(u_int32_t)); 
    offset += sizeof(u_int32_t);
    memcpy(buffer_instruccion->buffer + offset, &(puntero_archivo), sizeof(u_int32_t));
    offset += sizeof(u_int32_t);
    memcpy(buffer_instruccion->buffer + offset, &(tam_nombre), sizeof(u_int32_t));
    offset += sizeof(u_int32_t);
    memcpy(buffer_instruccion->buffer + offset, nombre, tam_nombre);
    offset += tam_nombre;

    while (list_iterator_has_next(iterator))
    {
        solicitud_unitaria_t *sol = list_iterator_next(iterator);

        memcpy(buffer_instruccion->buffer + offset, &(sol->dir_fisica_base), sizeof(u_int32_t));
        offset += sizeof(u_int32_t);
        memcpy(buffer_instruccion->buffer + offset, &(sol->offset), sizeof(u_int32_t));
        offset += sizeof(u_int32_t);
        memcpy(buffer_instruccion->buffer + offset, &(sol->tam), sizeof(u_int32_t));
        offset += sizeof(u_int32_t);
        memcpy(buffer_instruccion->buffer + offset, &(sol->pid), sizeof(u_int32_t));
        offset += sizeof(u_int32_t);
    }

    list_iterator_destroy(iterator);
    return buffer_instruccion;
}

void devolver_pcb(int motivo_desalojo, pcb_t pcb, int socket_cliente, t_strings_instruccion *instruccion, buffer_instr_io_t *buffer_instruccion_io)
{
    /*     pcb.registros->SI = 99;
        pcb.registros->DI = 99; */
    int tam_b = 0;
    if (buffer_instruccion_io != 0)
    {
        tam_b = buffer_instruccion_io->size;
    }
    int tam_adicional = sizeof(int) * 3 + instruccion->tamcod + instruccion->tamp1 + tam_b;

    t_paquete *paquete = malloc(sizeof(t_paquete));
    paquete->buffer = malloc(sizeof(t_buffer));
    paquete->codigo_operacion = motivo_desalojo;
    paquete->buffer->size = sizeof(int) * 3 + sizeof(registros_t) + tam_adicional;
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
    // comienzo serialiacion de buffer instruccion: es bastante una villa esto :D

    memcpy(paquete->buffer->stream + paquete->buffer->offset, &instruccion->tamcod, sizeof(int));
    paquete->buffer->offset += sizeof(int);
    memcpy(paquete->buffer->stream + paquete->buffer->offset, instruccion->cod_instruccion, instruccion->tamcod);
    paquete->buffer->offset += instruccion->tamcod;

    memcpy(paquete->buffer->stream + paquete->buffer->offset, &instruccion->tamp1, sizeof(int));
    paquete->buffer->offset += sizeof(int);
    memcpy(paquete->buffer->stream + paquete->buffer->offset, instruccion->p1, instruccion->tamp1);
    paquete->buffer->offset += instruccion->tamp1;

    memcpy(paquete->buffer->stream + paquete->buffer->offset, &tam_b, sizeof(int));
    paquete->buffer->offset += sizeof(int);

    if (buffer_instruccion_io != 0)
    {
        memcpy(paquete->buffer->stream + paquete->buffer->offset, buffer_instruccion_io->buffer, tam_b);
    }

    int bytes = paquete->buffer->size + 2 * sizeof(int); //=tam(buffer)+tam(codop)+tam(buffer->size)

    void *a_enviar = serializar_paquete(paquete, bytes);

    send(socket_cliente, a_enviar, bytes, 0);

    if (tam_b != 0)
    {
        free(buffer_instruccion_io->buffer);
    }
    free(buffer_instruccion_io);
    eliminar_paquete(paquete);
    free(a_enviar);
}

// Server
t_log *logger;

t_dictionary *inicializar_diccionario(registros_t *registros)
{
    dictionary_put(dic_p_registros, "PC", &(registros->PC)); // Nosotros pensamos que teoricamente un proceso no deberia poder cambiar su program counter, pero en las pruebas esta
    dictionary_put(dic_p_registros, "AX", &(registros->AX));
    dictionary_put(dic_p_registros, "BX", &(registros->BX));
    dictionary_put(dic_p_registros, "CX", &(registros->CX));
    dictionary_put(dic_p_registros, "DX", &(registros->DX));
    dictionary_put(dic_p_registros, "EAX", &(registros->EAX));
    dictionary_put(dic_p_registros, "EBX", &(registros->EBX));
    dictionary_put(dic_p_registros, "ECX", &(registros->ECX));
    dictionary_put(dic_p_registros, "EDX", &(registros->EDX));
    dictionary_put(dic_p_registros, "SI", &(registros->SI));
    dictionary_put(dic_p_registros, "DI", &(registros->DI));
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
    printf("SOCKET_KERNEL:%i\n", socket_cliente);
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
            log_info(logger, "Se recibio el proceso a ejecutar por dispatch");
            pcb_exec = recibir_paquete(socket_cliente);
            log_info(logger, "PCB VINO CON Q = %i", pcb_exec->quantum);
            sem_post(&hay_proceso);

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
void log_instruccion_ejecutada(t_strings_instruccion *palabras) // REVISAR SI ESTO REALMENTE ES UN INVALID READ O ES ERROR DE VALGRIND
{
    log_info(logger, "PID: %i - Ejecutando: %s - %s %s %s %s %s", pcb_exec->pid, palabras->cod_instruccion, palabras->p1, palabras->p2, palabras->p3, palabras->p4, palabras->p5);
}
void log_fetch_instruccion()
{
    log_info(logger, "PID: %i - FETCH - Program Counter: %i", pcb_exec->pid, pcb_exec->registros->PC);
}