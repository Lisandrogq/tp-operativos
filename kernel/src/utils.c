#include "utils.h"
#include <errno.h>

extern t_log *logger;
int next_pid;
t_list *lista_pcbs_ready;
t_list *lista_pcbs_bloqueado;
t_list *lista_pcbs_exec;
t_list *lista_IO;
pthread_mutex_t mutex_socket_memoria;
int operacion;
int contador;
int socket_memoria;
sem_t hay_IO;
void iniciar_proceso(char *path, int tam)
{

    pcb_t *nuevo_pcb = crear_pcb(next_pid);
    nuevo_pcb->pid = 0;
    nuevo_pcb->quantum = 35;
    nuevo_pcb->registros->AX = 58;
    nuevo_pcb->registros->BX = 0;
    nuevo_pcb->registros->CX = 54;
    nuevo_pcb->registros->EDX = 132;
    nuevo_pcb->registros->DI = 112;

    int error = list_add(lista_pcbs_ready, nuevo_pcb); // Esto debería estar en new. Y pasa a ready por el planificador de largo plazo
    next_pid++;
    pthread_mutex_lock(&mutex_socket_memoria); // capaz no es necesario
    solicitar_crear_estructuras_administrativas(tam, path, nuevo_pcb->pid, socket_memoria);
    pthread_mutex_unlock(&mutex_socket_memoria);
}
void finalizar_proceso(int pid){

    pthread_mutex_lock(&mutex_socket_memoria);
    solicitar_eliminar_estructuras_administrativas(pid,socket_memoria);
    pthread_mutex_unlock(&mutex_socket_memoria);
}

void solicitar_crear_estructuras_administrativas(int tam, char *path, int pid, int socket_memoria)
{

    t_paquete *paquete = malloc(sizeof(t_paquete));
    paquete->buffer = malloc(sizeof(t_buffer));
    paquete->codigo_operacion = CREAR_ESTRUC_ADMIN;
    paquete->buffer->size = sizeof(int) * 2 + tam;
    paquete->buffer->stream = malloc(paquete->buffer->size);
    paquete->buffer->offset = 0;

    memcpy(paquete->buffer->stream + paquete->buffer->offset, &tam, sizeof(int));
    paquete->buffer->offset += sizeof(int);

    memcpy(paquete->buffer->stream + paquete->buffer->offset, path, tam);
    paquete->buffer->offset += tam;

    memcpy(paquete->buffer->stream + paquete->buffer->offset, &pid, sizeof(int));
    int bytes = paquete->buffer->size + 2 * sizeof(int); // ESTE *2 NO SE PUEDE TOCAR, ANDA ASÍ, PUNTO(.).

    void *a_enviar = serializar_paquete(paquete, bytes);

    send(socket_memoria, a_enviar, bytes, 0);

    eliminar_paquete(paquete);
    free(a_enviar);
}

void solicitar_eliminar_estructuras_administrativas(int pid, int socket_memoria) {

    t_paquete *paquete = crear_paquete();
    paquete->codigo_operacion = ELIMINAR_ESTRUC_ADMIN;
    agregar_a_paquete(paquete, &pid, sizeof(int));
    enviar_paquete(paquete, socket_memoria);
    eliminar_paquete(paquete);
}

void ejecutar_script(const char *path) {

    FILE *archivo = fopen(path, "r");
    if (archivo == NULL) {
        printf("Error: No se pudo abrir el archivo");
        return;
    }

    char comando[100]; // Asumimos que ninguna línea del archivo tiene más de 100 caracteres
    while (fgets(comando, sizeof(comando), archivo) != NULL) {

        char *posicion_salto_linea = strchr(comando, '\n');
        if (posicion_salto_linea != NULL) {
            *posicion_salto_linea = '\0';
        }

        printf("Ejecutando comando: %s\n", comando);
        system(comando);
    }

    fclose(archivo);
}
int enviar_proceso_a_ejecutar(int cod_op, pcb_t *pcb, int socket_cliente)
{
    enviar_PCB(cod_op, *pcb, socket_cliente);
    int motivo_desalojo = -1;

    t_buffer *buffer = malloc(sizeof(t_buffer));
    int prueba = recibir_operacion(socket_cliente);

    recv(socket_cliente, &(buffer->size), sizeof(uint32_t), MSG_WAITALL);

    recv(socket_cliente, &motivo_desalojo, sizeof(int),0);
    buffer->stream = malloc(buffer->size);
    recv(socket_cliente, buffer->stream, buffer->size, 0);
    void *stream = buffer->stream;

    memcpy(&(pcb->pid), stream, sizeof(int));
    stream += sizeof(int);
    memcpy(&(pcb->quantum), stream, sizeof(int));
    stream += sizeof(int);
    memcpy(pcb->registros, stream, sizeof(registros_t));
    stream += sizeof(registros_t);
   // memcpy(&(pcb->state), stream, sizeof(state_t)); // no debería
    return motivo_desalojo;
}
void retirar_pcb_bloqueado(pcb_t pcb, int index)
{
    pcb.state = READY_S;
    list_add(lista_pcbs_ready, list_remove(lista_pcbs_bloqueado, index));
}
int planificar_fifo(int socket_cliente)
{

    pcb_t *pcb = list_get(lista_pcbs_ready, 0);
    list_remove(lista_pcbs_ready, 0);
    list_add(lista_pcbs_exec, pcb);
    pcb->state = EXEC_S;
    return enviar_proceso_a_ejecutar(DISPATCH, pcb, socket_cliente); // se encarga de enviar y recibir el nuevo contexto actualizando lo que haga falta y el motivo de desalojo
}
int planificar_rr(int socket_cliente)
{
    // Fifo pero con quantum
    pcb_t *pcb = list_get(lista_pcbs_ready, 0);
    list_remove(lista_pcbs_bloqueado, 0);
    list_add(lista_pcbs_exec, pcb);
    pcb->state = EXEC_S;
    return enviar_proceso_a_ejecutar(DISPATCH, pcb, socket_cliente);
}

void enviar_PCB(int cod_op, pcb_t pcb, int socket_cliente)
{
    log_warning(logger, "enviar operacion: codop:%i", cod_op);
    t_paquete *paquete = malloc(sizeof(t_paquete));

    paquete->codigo_operacion = cod_op;
    paquete->buffer = malloc(sizeof(t_buffer));
    paquete->buffer->size = sizeof(int) * 2 + sizeof(registros_t) + sizeof(state_t);
    paquete->buffer->stream = malloc(paquete->buffer->size);
    paquete->buffer->offset = 0;

    memcpy(paquete->buffer->stream + paquete->buffer->offset, &pcb.pid, sizeof(int));
    paquete->buffer->offset += sizeof(int);

    memcpy(paquete->buffer->stream + paquete->buffer->offset, &pcb.quantum, sizeof(int));
    paquete->buffer->offset += sizeof(int);

    memcpy(paquete->buffer->stream + paquete->buffer->offset, pcb.registros, sizeof(registros_t));
    paquete->buffer->offset += sizeof(registros_t);

    memcpy(paquete->buffer->stream + paquete->buffer->offset, &pcb.state, sizeof(state_t)); // esto no hace falta para la cpu
    int bytes = paquete->buffer->size + 2 * sizeof(int);

    void *a_enviar = serializar_paquete(paquete, bytes);

    send(socket_cliente, a_enviar, bytes, 0);

    free(a_enviar);
    eliminar_paquete(paquete);
}

int handshake(int socket_cliente)
{
    size_t bytes;

    int32_t handshake = HS_KERNEL; // PASAR ESTO A CONFIG en utils
    int32_t result;

    bytes = send(socket_cliente, &handshake, sizeof(int32_t), 0);
    bytes = recv(socket_cliente, &result, sizeof(int32_t), MSG_WAITALL);

    if (result != 0)
    {
        log_error(logger, "Error en el handshake");
        return -1; // capaz habría q hacer mas cosas
    }
    return result;
}

// Server

void *client_handler(void *arg)
{
    int socket_cliente = *(int *)arg;
    int modulo = handshake_Server(socket_cliente);
    switch (modulo) // se debería ejecutar un handler para cada modulo
    {
    case 1:
        log_info(logger, "se conecto el modulo cpu");
        break;
    case 2:
        log_info(logger, "se conecto el modulo memoria");
        break;
    case 3:
        log_info(logger, "se conecto el modulo io");
        break;
    }

    bool conexion_terminada = false;
    while (!conexion_terminada)
    {
        int cod_op = recibir_operacion(socket_cliente); // REVISARESTO
        switch (cod_op)
        {
        case CREACION_IO:
            t_interfaz *interfaz =recibir_IO(socket_cliente);
            list_add(lista_IO, interfaz); // talvez sea necesario una lista por tipo de io para hacer la busqueda mas rapido
            break;
        default:
            log_warning(logger, "Operacion desconocida. No quieras meter la pata");
            break;
        }
    }
    close(socket_cliente);
}
t_interfaz *recibir_IO(int socket_cliente){	
	t_buffer *buffer = malloc(sizeof(t_buffer));
	recv(socket_cliente, &(buffer->size), sizeof(uint32_t), 0);
	buffer->stream = malloc(buffer->size);
	recv(socket_cliente, buffer->stream, buffer->size, 0);

	t_interfaz *estructura = malloc(sizeof(t_interfaz));
	void *stream = buffer->stream;
	memcpy(&(estructura->largo), stream, sizeof(int));
	stream += sizeof(int);
	estructura->nombre = malloc(estructura->largo);
	memcpy((estructura->nombre), stream, estructura->largo); 
	stream += estructura->largo;
	memcpy(&(estructura->tipo), stream, sizeof(int)); 
	stream += sizeof(int);
	memcpy(&(estructura->estado), stream, sizeof(int)); 
	free(buffer->stream);
	free(buffer);
	return estructura;
}
int handshake_Server(int socket_cliente)
{

    size_t bytes;

    int32_t handshake;
    int32_t resultOk = 0;
    int32_t resultError = -1;

    bytes = recv(socket_cliente, &handshake, sizeof(int32_t), MSG_WAITALL);

    if (handshake == 3)
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