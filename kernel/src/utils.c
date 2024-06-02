#include "utils.h"
#include <errno.h>
#include <pthread.h>
int pid_sig_term;//sirve para cuando se mata un proceso y justo salio bloqueado

t_log *logger;
t_config *config;
int next_pid;
t_list *lista_pcbs_ready;
pthread_mutex_t mutex_lista_ready;
pthread_mutex_t mutex_pcb_desalojado;
pthread_mutex_t mutex_lista_bloqueado;
pthread_mutex_t mutex_lista_exit;
pthread_mutex_t mutex_lista_exec;
sem_t elementos_ready; // contador de ready, si no hay, no podes planificar.
t_dictionary *listas_pcbs_bloqueado;
t_list *lista_pcbs_exec;
t_list *lista_pcbs_exit;
t_dictionary *dictionary_ios;
pthread_mutex_t mutex_socket_memoria;
int operacion;
int contador;
int socket_memoria;
pthread_mutex_t mutex_socket_interrupt;
int socket_interrupt;
void comando_iniciar_proceso(char *path, int tam)
{

    pcb_t *nuevo_pcb = crear_pcb(next_pid); // se estan creando con el PID SIEMPRE EN 0

    pthread_mutex_lock(&mutex_socket_memoria); // capaz no es necesario
    solicitar_crear_estructuras_administrativas(tam, path, nuevo_pcb->pid, socket_memoria);
    pthread_mutex_unlock(&mutex_socket_memoria);

    pthread_mutex_lock(&mutex_lista_ready);
    int error = list_add(lista_pcbs_ready, nuevo_pcb); // Esto debería estar en new. Y pasa a ready por el planificador de largo plazo
    pthread_mutex_unlock(&mutex_lista_ready);
    next_pid++;
}
pcb_t *crear_pcb(int pid)
{
	pcb_t *nuevo_pcb = malloc(sizeof(pcb_t));
	memset(nuevo_pcb, 0, sizeof(pcb_t));
    nuevo_pcb->quantum = config_get_int_value(config, "QUANTUM");
    nuevo_pcb->pid = pid;
    printf("\nEl pid del nuevopcb es: %i\n",nuevo_pcb->pid);
    nuevo_pcb->state= READY_S;//ESTO DEBERÍA SER NEW, PERO TODAVIA NO HAY PLANIFICADOR DE LARGO PLAZO
	registros_t *registros = malloc(sizeof(registros_t));//CHEQUEAR esto, creo q esta bien
	memset(registros, 0, sizeof(registros_t));
	nuevo_pcb->registros= registros;
	return nuevo_pcb;
}
int get_pid_state(int pid_buscado)
{
    bool is_pid(void *pcb)
    {
        return ((pcb_t *)pcb)->pid == pid_buscado; // no hay colores pq vscode no se la banca, no es bug
    };
    int esta_ready = (int)list_any_satisfy(lista_pcbs_ready, is_pid);
    // int esta_bloqueado = (int)list_any_satisfy(lista_pcbs_bloqueado, is_pid);hay que iterar sobre dictionary
    //  int esta_new = (int)list_any_satisfy(lista_pcbs_new, is_pid);
    int esta_exec = (int)list_any_satisfy(lista_pcbs_exec, is_pid);

    // solo uno de los esta_ debería estar en 1
    int estado_encontrado = esta_ready * READY_S + esta_exec * EXEC_S;
    // log_debug("se encontro el %i en el estado %i", pid_buscado, estado_encontrado);
    return estado_encontrado;
}
void comando_finalizar_proceso(char *pid_str, int motivo)
{
    int pid = atoi(pid_str);
    int pid_state = get_pid_state(pid);
    if (pid_state == EXEC_S)
    {
        // intr a cpu y llevar a exit(creo q no hay mutex)
        enviar_interrupcion(motivo, pid);
        pid_sig_term =pid;
        pthread_mutex_lock(&mutex_pcb_desalojado); // posible solucion:"bool de: ELIMINAR ESTRUCTURAS CUANDO SALGA Y WAITEARLO EN PLANIGFICACION"
    }
    if (pid_state == BLOCK_S)
    {
        // revisar situacion con IOs pendientes(issue)
    }
    if (pid_state == READY_S)
    {
    }
    if (pid_state == NEW_S)
    {
    }
    if (pid_state != -1 && pid_state != NEW_S) // si el proceso existe y no esta en new.
    {
        pthread_mutex_lock(&mutex_socket_memoria);
        solicitar_eliminar_estructuras_administrativas(pid);
        pthread_mutex_unlock(&mutex_socket_memoria);        
    }
    log_info(logger, "Finaliza el proceso %i - Motivo: %i", pid, motivo); // hacerlo string
    // mandar pcb a exit
}

void* imprimir_pcb(pcb_t *pcb){
    log_info(logger, "PID: %i", pcb->pid);
    log_info(logger, "Quantum: %i", pcb->quantum);
    printf("\n");
    return 0;
}

void* imprimir_lista_bloqueado(char* nombre_io, t_list *lista){
    printf("IO: %s\n", nombre_io);
    list_iterate(lista, (void *)imprimir_pcb);
    printf("\n");
    return 0;
}

void comando_listar_procesos_por_estado(){
    log_info(logger, "Procesos en Ready:");
    pthread_mutex_lock(&mutex_lista_ready);
    list_iterate(lista_pcbs_ready, (void *)imprimir_pcb);
    pthread_mutex_unlock(&mutex_lista_ready);
    log_info(logger, "Procesos en Exec:");
    pthread_mutex_lock(&mutex_lista_exec);
    list_iterate(lista_pcbs_exec, (void *)imprimir_pcb);
    pthread_mutex_unlock(&mutex_lista_exec);
    log_info(logger, "Procesos en Blocked:");
    dictionary_iterator(listas_pcbs_bloqueado, (void *)imprimir_lista_bloqueado);
    pthread_mutex_lock(&mutex_lista_exit);
    log_info(logger, "Procesos en Exit:");
    pthread_mutex_unlock(&mutex_lista_exit);
    list_iterate(lista_pcbs_exit, (void *)imprimir_pcb);
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

void solicitar_eliminar_estructuras_administrativas(int pid)
{
    t_paquete *paquete = malloc(sizeof(t_paquete));
    paquete->buffer = malloc(sizeof(t_buffer));
    paquete->codigo_operacion = ELIMINAR_ESTRUC_ADMIN;
    paquete->buffer->size = sizeof(int);
    paquete->buffer->stream = malloc(paquete->buffer->size);
    paquete->buffer->offset = 0;

    memcpy(paquete->buffer->stream + paquete->buffer->offset, &pid, sizeof(int));
    int bytes = paquete->buffer->size + 2 * sizeof(int);

    void *a_enviar = serializar_paquete(paquete, bytes);

    send(socket_memoria, a_enviar, bytes, 0);

    eliminar_paquete(paquete);
    free(a_enviar);
}

void ejecutar_script(const char *path)
{

    FILE *archivo = fopen(path, "r");
    if (archivo == NULL)
    {
        printf("Error: No se pudo abrir el archivo");
        return;
    }

    char comando[100]; // Asumimos que ninguna línea del archivo tiene más de 100 caracteres
    while (fgets(comando, sizeof(comando), archivo) != NULL)
    {

        char *posicion_salto_linea = strchr(comando, '\n');
        if (posicion_salto_linea != NULL)
        {
            *posicion_salto_linea = '\0';
        }

        printf("Ejecutando comando: %s\n", comando);
        system(comando);
    }

    fclose(archivo);
}

void enviar_interrupcion(int motivo, int pid)
{
    t_paquete *paquete = malloc(sizeof(t_paquete));
    paquete->codigo_operacion = INTERRUPCION;
    paquete->buffer = malloc(sizeof(t_buffer));
    paquete->buffer->size = sizeof(int) * 2;
    paquete->buffer->stream = malloc(paquete->buffer->size);
    paquete->buffer->offset = 0;

    memcpy(paquete->buffer->stream + paquete->buffer->offset, &pid, sizeof(int));
    paquete->buffer->offset += sizeof(int);

    memcpy(paquete->buffer->stream + paquete->buffer->offset, &motivo, sizeof(int));
    paquete->buffer->offset += sizeof(int);

    int bytes = paquete->buffer->size + 2 * sizeof(int);
    void *a_enviar = serializar_paquete(paquete, bytes);

    send(socket_interrupt, a_enviar, bytes, 0);

    free(a_enviar);
    eliminar_paquete(paquete);
}
void *hilo_quantum(void *parametro){
    pcb_t *pcb = (pcb_t*)parametro;
    log_info(logger,"Quantum: %i", pcb->quantum);
    usleep(pcb->quantum);
    log_info(logger,"Sali del sleep, mando interrupcion");
    enviar_interrupcion(CLOCK, pcb->pid);
}
int enviar_proceso_a_ejecutar(int cod_op, pcb_t *pcb, int socket_cliente, t_strings_instruccion *palabras,  char *algoritmo)
{
    enviar_PCB(cod_op, *pcb, socket_cliente);
    int motivo_desalojo = -1;

    if(strcmp(algoritmo, "RR") == 0){
        pthread_t quantum;
        pthread_create(&quantum,NULL, hilo_quantum, pcb);
        pthread_detach(&quantum);
        if(recibir_operacion(socket_cliente) != -1){
            log_info(logger,"Cancelo");
            pthread_cancel(&quantum);
        }
    }else if(strcmp(algoritmo, "VRR") == 0){
        //Cositas
    }
    
    t_buffer *buffer = malloc(sizeof(t_buffer));
    //int prueba = recibir_operacion(socket_cliente);

    recv(socket_cliente, &(buffer->size), sizeof(int), MSG_WAITALL);
    buffer->stream = malloc(buffer->size);

    recv(socket_cliente, buffer->stream, buffer->size, 0);
    void *stream = buffer->stream;

    memcpy(&(pcb->pid), stream, sizeof(int));
    stream += sizeof(int);
    memcpy(&(pcb->quantum), stream, sizeof(int));
    stream += sizeof(int);
    memcpy(pcb->registros, stream, sizeof(registros_t));
    stream += sizeof(registros_t);
    memcpy(&motivo_desalojo, stream, sizeof(int));
    stream += sizeof(int);
    // inicio deserializacion de palabras:

    // palabras = malloc(sizeof(t_strings_instruccion));--este malloc se hace afuera.
    memset(palabras, 0, sizeof(t_strings_instruccion));

    memcpy(&(palabras->tamcod), stream, sizeof(int));
    stream += sizeof(int);
    palabras->cod_instruccion = malloc(palabras->tamcod);
    memset(palabras->cod_instruccion, 0, 1); // se pone el unico byte alocado por malloc(0) en 0 para limpiar la basura(caso parametro vacio)
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

    memcpy(&(palabras->tamp3), stream, sizeof(int));
    stream += sizeof(int);

    palabras->p3 = malloc(palabras->tamp3);
    memset(palabras->p3, 0, 1); // se pone el unico byte alocado por malloc(0) en 0 para limpiar la basura(caso parametro vacio)

    memcpy((palabras->p3), stream, palabras->tamp3);
    stream += palabras->tamp3;

    memcpy(&(palabras->tamp4), stream, sizeof(int));
    stream += sizeof(int);

    palabras->p4 = malloc(palabras->tamp4);
    memset(palabras->p4, 0, 1); // se pone el unico byte alocado por malloc(0) en 0 para limpiar la basura(caso parametro vacio)

    memcpy((palabras->p4), stream, palabras->tamp4);
    stream += palabras->tamp4;

    memcpy(&(palabras->tamp5), stream, sizeof(int));
    stream += sizeof(int);

    palabras->p5 = malloc(palabras->tamp5);
    memset(palabras->p5, 0, 1); // se pone el unico byte alocado por malloc(0) en 0 para limpiar la basura(caso parametro vacio)

    memcpy((palabras->p5), stream, palabras->tamp5);
    stream += palabras->tamp5;

    // en algun lugar(afuera de esto) hay q hacerle malloc a las palabras.
    return motivo_desalojo;
    // memcpy(&(pcb->state), stream, sizeof(state_t)); // no debería
}
bool is_io_connected(int socket)
{
    return true;
}
bool io_acepta_operacion(t_interfaz *io, char *cod_instruccion)
{
    switch (io->tipo)
    {
    case GENERICA:
        if (strcmp(cod_instruccion, "IO_GEN_SLEEP") == 0)
            return true;
        break;

    default:
        break;
    }
    return false;
}
int es_una_io_valida(int pid, t_strings_instruccion *instruccion)
{
    char *nombre_interfaz = instruccion->p1;

    if (!dictionary_has_key(dictionary_ios, nombre_interfaz))
        return -1;
    t_interfaz *io = dictionary_get(dictionary_ios, nombre_interfaz);
    if (!is_io_connected(io->socket))
        return -1;

    if (!io_acepta_operacion(io, instruccion->cod_instruccion))
        return -1;

    return 1;
}
pedir_io_task(int pid, t_interfaz *io, t_strings_instruccion *instruccion) //!!!TODO: HACERLA GENERICA PARA TODAS LAS INSTRUCCIONES
{                                                                          // ahora funciona solo con sleep
    t_paquete *paquete = malloc(sizeof(t_paquete));

    paquete->codigo_operacion = IO_GEN_SLEEP;
    paquete->buffer = malloc(sizeof(t_buffer));
    paquete->buffer->size = sizeof(int) + instruccion->tamcod + instruccion->tamp1 + instruccion->tamp2 + 3 * sizeof(int);
    paquete->buffer->stream = malloc(paquete->buffer->size);
    paquete->buffer->offset = 0;

    memcpy(paquete->buffer->stream + paquete->buffer->offset, &pid, sizeof(int));
    paquete->buffer->offset += sizeof(int);

    memcpy(paquete->buffer->stream + paquete->buffer->offset, &(instruccion->tamcod), sizeof(int));
    paquete->buffer->offset += sizeof(int);

    memcpy(paquete->buffer->stream + paquete->buffer->offset, instruccion->cod_instruccion, instruccion->tamcod);
    paquete->buffer->offset += instruccion->tamcod;

    memcpy(paquete->buffer->stream + paquete->buffer->offset, &(instruccion->tamp1), sizeof(int));
    paquete->buffer->offset += sizeof(int);

    memcpy(paquete->buffer->stream + paquete->buffer->offset, instruccion->p1, instruccion->tamp1);
    paquete->buffer->offset += instruccion->tamp1;

    memcpy(paquete->buffer->stream + paquete->buffer->offset, &(instruccion->tamp2), sizeof(int));
    paquete->buffer->offset += sizeof(int);

    memcpy(paquete->buffer->stream + paquete->buffer->offset, instruccion->p2, instruccion->tamp2);
    paquete->buffer->offset += instruccion->tamp2;

    int bytes = paquete->buffer->size + 2 * sizeof(int); // ESTE *2 NO SE PUEDE TOCAR, ANDA ASÍ, PUNTO(.).

    void *a_enviar = serializar_paquete(paquete, bytes);

    send(io->socket, a_enviar, bytes, 0);
    free(a_enviar);
    eliminar_paquete(paquete);
}

void desbloquear_pcb(int pid_a_desbloquear, char *nombre_io)
{

    bool is_pid(void *pcb)
    {
        return ((pcb_t *)pcb)->pid == pid_a_desbloquear; // no hay colores pq vscode no se la banca, no es bug
    };
    t_list *lista_blocked_del_io = dictionary_get(listas_pcbs_bloqueado, nombre_io);
    pthread_mutex_lock(&mutex_lista_bloqueado); // este mutex sería de la lista capaz(no se si hace falta)
    pcb_t *pcb_a_desbloquear = list_remove_by_condition(lista_blocked_del_io, is_pid);
    pthread_mutex_unlock(&mutex_lista_bloqueado);

    pthread_mutex_lock(&mutex_lista_ready);
    list_add(lista_pcbs_ready, pcb_a_desbloquear);
    sem_post(&elementos_ready);
    pthread_mutex_unlock(&mutex_lista_ready);
    pcb_a_desbloquear->state = READY_S;
}
int planificar_fifo(int socket_cliente, t_strings_instruccion *instruccion_de_desalojo, char* algoritmo)
{
    pthread_mutex_lock(&mutex_lista_ready);
    pcb_t *pcb_a_ejecutar = list_remove(lista_pcbs_ready, 0);
    pthread_mutex_unlock(&mutex_lista_ready);
    list_add(lista_pcbs_exec, pcb_a_ejecutar);
    pcb_a_ejecutar->state = EXEC_S;
    log_debug(logger, "Enviando PID %i a ejecutar", pcb_a_ejecutar->pid);

    return enviar_proceso_a_ejecutar(DISPATCH, pcb_a_ejecutar, socket_cliente, instruccion_de_desalojo,algoritmo); // se encarga de enviar y recibir el nuevo contexto actualizando lo que haga falta y el motivo de desalojo
    // esto hay q separarlo en dos funciones:enviar proceso Y recibir proceso/pcb NO.
}
int planificar_rr(int socket_cliente, t_strings_instruccion *instruccion_de_desalojo, char* algoritmo)
{
    // Fifo pero con quantum
    pcb_t *pcb = list_remove(lista_pcbs_ready, 0);
    list_add(lista_pcbs_exec, pcb);
    pcb->state = EXEC_S;
    return enviar_proceso_a_ejecutar(DISPATCH, pcb, socket_cliente, instruccion_de_desalojo,algoritmo);
}

void enviar_PCB(int cod_op, pcb_t pcb, int socket_cliente)
{
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
    int socket_io = *(int *)arg;
    int modulo = handshake_Server(socket_io);
    log_info(logger, "se conecto alguna io");

    bool conexion_terminada = false;
    while (!conexion_terminada)
    {
        int cod_op = recibir_operacion(socket_io); 
        switch (cod_op)
        {
        case CREACION_IO:
            t_interfaz *interfaz = recibir_IO(socket_io);
            interfaz->socket = socket_io;
            dictionary_put(dictionary_ios, interfaz->nombre, interfaz); // creoq ya esta medio al pedo esta
            t_list *list_blocked_io = list_create();
            dictionary_put(listas_pcbs_bloqueado, interfaz->nombre, list_blocked_io);
            log_info(logger, "la io conectada se llama '%s'", interfaz->nombre);

            break;
        case FIN_IO_TASK: /// falta manejar caso en que finaliza una io a un proceso en exit

            t_fin_io_task *estructura = recibir_fin_io_task(socket_io);
            desbloquear_pcb(estructura->pid, estructura->nombre);
            log_info(logger, "TERMINO LA IO del pid:%i que uso:%s", estructura->pid, estructura->nombre);
            //!!!mandar siguiente task
            break;
        case -1:
            log_info(logger, "SE DESCONECTO UNA IO");
            return -1;
        default:
            log_warning(logger, "Operacion desconocida. No quieras meter la pata");
            break;
        }
    }
    close(socket_io);
}
t_fin_io_task *recibir_fin_io_task(int socket_cliente)
{

    int tam_nombre;
    t_buffer *buffer = malloc(sizeof(t_buffer));
    recv(socket_cliente, &(buffer->size), sizeof(uint32_t), 0);
    buffer->stream = malloc(buffer->size);
    recv(socket_cliente, buffer->stream, buffer->size, 0);

    t_fin_io_task *estructura = malloc(sizeof(t_interfaz));
    void *stream = buffer->stream;
    memcpy(&(estructura->pid), stream, sizeof(int));
    stream += sizeof(int);
    memcpy(&tam_nombre, stream, sizeof(int));
    stream += sizeof(int);
    estructura->nombre = malloc(tam_nombre);
    memcpy((estructura->nombre), stream, tam_nombre);
    stream += tam_nombre;

    free(buffer->stream);
    free(buffer);
    return estructura;
}
t_interfaz *recibir_IO(int socket_cliente)
{
    int tam_nombre;
    t_buffer *buffer = malloc(sizeof(t_buffer));
    recv(socket_cliente, &(buffer->size), sizeof(uint32_t), 0);
    buffer->stream = malloc(buffer->size);
    recv(socket_cliente, buffer->stream, buffer->size, 0);

    t_interfaz *estructura = malloc(sizeof(t_interfaz));
    void *stream = buffer->stream;
    memcpy(&tam_nombre, stream, sizeof(int));
    stream += sizeof(int);
    estructura->nombre = malloc(tam_nombre);
    memcpy((estructura->nombre), stream, tam_nombre);
    stream += tam_nombre;
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