#include "utils.h"
#include <errno.h>
#include <pthread.h>
int pid_sig_term; // sirve para cuando se mata un proceso y justo salio bloqueado

t_log *logger;
t_config *config;
int next_pid;
t_list *lista_pcbs_ready;
t_list *lista_ready_mas;
pthread_mutex_t mutex_lista_ready;
pthread_mutex_t mutex_pcb_desalojado;
// pthread_mutex_t mutex_lista_bloqueado;
pthread_mutex_t mutex_lista_exit;
pthread_mutex_t mutex_lista_exec;
sem_t elementos_ready; // contador de ready, si no hay, no podes planificar.
t_dictionary *dictionary_pcbs_bloqueado;
t_list *lista_pcbs_exec;
t_list *lista_pcbs_exit;
t_list *lista_pcbs_new;
t_dictionary *dictionary_ios;
pthread_mutex_t mutex_socket_memoria;
t_temporal *timer;
sem_t contador_multi;
sem_t hay_new;
int operacion;
int contador;
int socket_memoria;
pthread_mutex_t mutex_socket_interrupt;
int socket_interrupt;
void comando_iniciar_proceso(char *path, int tam)
{
    pcb_t *nuevo_pcb = crear_pcb(next_pid);
    elemento_cola_new *elemento = malloc(sizeof(elemento_cola_new));
    char *nuevo_path = malloc(sizeof(tam));
    strcpy(nuevo_path, path);
    elemento->pcb = nuevo_pcb;
    elemento->tam = tam;
    elemento->path = nuevo_path;
    int error = list_add(lista_pcbs_new, elemento);
    sem_post(&hay_new);
    next_pid++;
}
pcb_t *crear_pcb(int pid)
{
    pcb_t *nuevo_pcb = malloc(sizeof(pcb_t));
    memset(nuevo_pcb, 0, sizeof(pcb_t));
    nuevo_pcb->quantum = config_get_int_value(config, "QUANTUM");
    nuevo_pcb->pid = pid;
    printf("\nEl pid del nuevopcb es: %i\n", nuevo_pcb->pid);
    nuevo_pcb->state = NEW_S; // previamente estaba en ready
    registros_t *registros = malloc(sizeof(registros_t));
    memset(registros, 0, sizeof(registros_t));
    nuevo_pcb->registros = registros;
    return nuevo_pcb;
}
int get_pid_state(int pid_buscado)
{
    int esta_bloqueado = false;
    bool is_pid(void *pcb)
    {
        return ((pcb_t *)pcb)->pid == pid_buscado; // no hay colores pq vscode no se la banca, no es bug
    };
    bool is_pid_in_blocked(void *elemento)
    {
        return ((elemento_cola_io *)elemento)->pcb->pid == pid_buscado; // requeiere otra lista pq los elementos son structs
    };
    void *is_pid_in_list(char *nombre_io, t_cola_io *struct_cola)
    {
        t_list *lista = struct_cola->cola_de_io_pedido;
        if (!esta_bloqueado)
            esta_bloqueado = list_any_satisfy(lista, is_pid_in_blocked); // no hay colores pq vscode no se la banca, no es bug
        else
            return;
    };
    int esta_ready = (int)list_any_satisfy(lista_pcbs_ready, is_pid);
    dictionary_iterator(dictionary_pcbs_bloqueado, is_pid_in_list);
    int esta_new = (int)list_any_satisfy(lista_pcbs_new, is_pid);
    int esta_exec = (int)list_any_satisfy(lista_pcbs_exec, is_pid);

    // solo uno de los esta_ debería estar en 1
    int estado_encontrado = esta_ready * READY_S + esta_exec * EXEC_S + esta_bloqueado * BLOCK_S + esta_new * NEW_S;
    // log_debug("se encontro el %i en el estado %i", pid_buscado, estado_encontrado);
    return estado_encontrado;
}

void comando_finalizar_proceso(char *pid_str, int motivo)
{
    int pid_a_terminar = atoi(pid_str);
    int pid_state = get_pid_state(pid_a_terminar);
    pcb_t *pcb_a_terminar;

    bool is_pid(void *pcb) /// NO ENCONTRE FORMA DE NO DECLARAR ESTAS FUNCIONES ADENTRO
    {
        return ((pcb_t *)pcb)->pid == pid_a_terminar; // no hay colores pq vscode no se la banca, no es bug
    };
    bool is_pid_in_blocked(void *elemento) /// NO ENCONTRE FORMA DE NO DECLARAR ESTAS FUNCIONES ADENTRO
    {
        return ((elemento_cola_io *)elemento)->pcb->pid == pid_a_terminar; // no hay colores pq vscode no se la banca, no es bug
    };
    void *eliminar_de_bloqueado_si_corresponde(char *nombre_io, t_cola_io *struct_io) // NO ENCONTRE FORMA DE NO DECLARAR ESTAS FUNCIONES ADENTRO
    {
        t_list *lista = struct_io->cola_de_io_pedido;
        sem_t *sem = struct_io->elementos_cola_io;
        // wait mutex
        elemento_cola_io *a_borrar = list_find(lista, is_pid_in_blocked);
        if (a_borrar != NULL)
        {
            pcb_a_terminar = a_borrar->pcb;
            sem_wait(sem);
            list_remove_element(lista, a_borrar);
        }
        // wait del elementos_bloqued correspondiente
        return 0;
    };

    if (pid_state == NEW_S)
    {
        // mutex , remove y NO liberar eadmin
        // ACA SE DEBERIA agregar a la lista de exit SI EL PROCESO ESTA EN NEW
    }
    if (pid_state == EXEC_S)
    {
        // intr a cpu y llevar a exit(creo q no hay mutex)
        enviar_interrupcion(motivo, pid_a_terminar);
        pid_sig_term = pid_a_terminar;
        pcb_a_terminar = list_get(lista_pcbs_exec, 0); // se obtiene sin removerlo, pq que el remove se hace al desalojarse el pcb
        pthread_mutex_lock(&mutex_pcb_desalojado);
    }
    if (pid_state == BLOCK_S) // revisar situacion con IOs pendientes(issue)
    {
        dictionary_iterator(dictionary_pcbs_bloqueado, (void *)eliminar_de_bloqueado_si_corresponde); // habría que validar si realmente se borro
    }
    if (pid_state == READY_S)
    {
        sem_wait(&elementos_ready);
        pthread_mutex_lock(&mutex_lista_ready);
        pcb_a_terminar = list_remove_by_condition(lista_pcbs_ready, is_pid);
        pthread_mutex_unlock(&mutex_lista_ready);
    }

    if (pid_state != -1 && pid_state != NEW_S) // si el proceso existe y no esta en new.
    {
        pthread_mutex_lock(&mutex_socket_memoria);
        solicitar_eliminar_estructuras_administrativas(pid_a_terminar);
        pthread_mutex_unlock(&mutex_socket_memoria);
        pcb_a_terminar->state = EXIT_S;
        if (pid_state != EXEC_S)
        {
            list_add(lista_pcbs_exit, pcb_a_terminar); // Post(contador multiprogramacion)
            sem_post(&contador_multi);
        }
    }

    log_info(logger, "Finaliza el proceso %i - Motivo: %i", pid_a_terminar, motivo); // hacerlo string
    // mandar pcb a exit
}

void *imprimir_pcb(pcb_t *pcb)
{
    log_info(logger, "PID: %i", pcb->pid);
    log_info(logger, "Quantum: %i", pcb->quantum);
    printf("\n");
    return 0;
}

void *imprimir_pcb_bloqueado(elemento_cola_io *elemento_cola)
{
    pcb_t *pcb = elemento_cola->pcb;
    log_info(logger, "PID: %i, PC: %i, Q: %i", pcb->pid, pcb->registros->PC, pcb->quantum);
    printf("\n");
    return 0;
}

void *imprimir_lista_bloqueado(char *nombre_io, t_cola_io *struct_interfaz)
{
    t_list *lista = struct_interfaz->cola_de_io_pedido;
    printf("IO: %s\n", nombre_io);
    list_iterate(lista, (void *)imprimir_pcb_bloqueado);
    printf("\n");
    return 0;
}

void comando_listar_procesos_por_estado()
{
    log_info(logger, "Procesos en Ready:");
    pthread_mutex_lock(&mutex_lista_ready);
    list_iterate(lista_pcbs_ready, (void *)imprimir_pcb);
    pthread_mutex_unlock(&mutex_lista_ready);
    log_info(logger, "Procesos en Exec:");
    pthread_mutex_lock(&mutex_lista_exec);
    list_iterate(lista_pcbs_exec, (void *)imprimir_pcb);
    pthread_mutex_unlock(&mutex_lista_exec);
    log_info(logger, "Procesos en Blocked:");
    dictionary_iterator(dictionary_pcbs_bloqueado, (void *)imprimir_lista_bloqueado);
    pthread_mutex_lock(&mutex_lista_exit);
    log_info(logger, "Procesos en Exit:");
    pthread_mutex_unlock(&mutex_lista_exit);
    list_iterate(lista_pcbs_exit, (void *)imprimir_pcb);
}

void comando_ejecutar_script(char *path, FILE *archivo)
{
    // abro el archivo como lectura//

    char *linea = malloc(100);
    size_t len = 50;
    ssize_t read;

    // Mientras haya lineas en el archivo//
    while ((read = fgets(linea, len, archivo)) != NULL)
    {
        char *instruccion[2];
        instruccion[0] = malloc(strlen(linea));
        instruccion[1] = malloc(strlen(linea));
        instruccion[0] = strtok(linea, " \n");
        instruccion[1] = strtok(NULL, " \n");
        if (strcmp(linea, "\0") == 0)
        {
            free(linea);
            continue;
        }
        log_info(logger, "Instruccion: %s", linea);
        // HAY QUE EJECUTAR CADA COMANDO//
        // seguro hay que ver como hacerlo sin repetir logica//
        // ¿generar una funcion que se repita?//
        if (!strcmp(instruccion[0], "INICIAR_PROCESO"))
        {
            char *copia;
            strncpy(copia, instruccion[1], strlen(instruccion[1] - 1));
            int tam = 1 + strlen(instruccion[1]);
            comando_iniciar_proceso(instruccion[1], tam); 
        }
        if (!strcmp(instruccion[0], "FINALIZAR_PROCESO"))
        {
            int tam = 1 + sizeof(strlen(linea));
            comando_finalizar_proceso(instruccion[1], INTERRUPTED_BY_USER);
        }
        if (!strcmp(instruccion[0], "ESTADO_PROCESO"))
        {
            comando_listar_procesos_por_estado();
        }
        if (!strcmp(instruccion[0], "ddd"))
            return;
        free(linea);
    }
    // cierra el archivo//
    fclose(archivo);
}
void modificar_multiprogramacion(int grado, FILE *archivo){
    int encontrado = 0;
    char *linea = malloc(100);
    size_t len = 100;
    while(fgets(linea, len, archivo) != NULL){
          if (ferror(archivo)) {
        perror("Error de lectura");
        break;
    }
         if (!strncmp(linea, "GRADO_MULTIPROGRAMACION=", strlen("GRADO_MULTIPROGRAMACION="))) {
            fseek(archivo, -strlen(linea), SEEK_CUR);
            fprintf(archivo, "GRADO_MULTIPROGRAMACION=%i", grado);
            encontrado = 1;
            break;
        } 
    } 
      if(encontrado == 0){
        log_info(logger, "no encontre el parametro");
    }  
    fclose(archivo);
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

/* void ejecutar_script(const char *path)
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
} */

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
void *hilo_quantum(void *parametro)
{
    pcb_t *pcb = (pcb_t *)parametro;
    usleep(pcb->quantum);
    enviar_interrupcion(CLOCK, pcb->pid);
}
void *hilo_quantumVRR(void *parametro)
{
    timer = temporal_create();
    pcb_t *pcb = (pcb_t *)parametro;
    log_info(logger, "Quantum justo antes: %i", pcb->quantum);
    usleep(pcb->quantum * 1000);
    enviar_interrupcion(CLOCK, pcb->pid);
    log_info(logger, "MANDE INTERRUPCION");
}
int enviar_proceso_a_ejecutar(int cod_op, pcb_t *pcb, int socket_cliente, t_strings_instruccion *palabras, char *algoritmo)
{
    enviar_PCB(cod_op, *pcb, socket_cliente);
    int motivo_desalojo = -1;
    int restante_quantum = 0;

    if (strcmp(algoritmo, "RR") == 0)
    {
        pthread_t quantum;
        pthread_create(&quantum, NULL, hilo_quantum, pcb);
        pthread_detach(quantum);
        if (recibir_operacion(socket_cliente) != -1)
        {
            pthread_cancel(quantum);
        }
    }
    else if (strcmp(algoritmo, "FIFO") == 0)
    {
        int prueba = recibir_operacion(socket_cliente);
    }
    else
    { // VRR
        pthread_t quantumVRR;
        pthread_create(&quantumVRR, NULL, hilo_quantumVRR, pcb);
        pthread_detach(quantumVRR);
        if (recibir_operacion(socket_cliente) != CLOCK) // esta validacion va a ser mas larga cuando se usen recursos
        {
            temporal_stop(timer);
            pthread_cancel(quantumVRR);
            restante_quantum = pcb->quantum - temporal_gettime(timer);
            log_info(logger, "Restante: %i", restante_quantum);
        }
        temporal_destroy(timer);
    }
    t_buffer *buffer = malloc(sizeof(t_buffer));
    recv(socket_cliente, &(buffer->size), sizeof(int), MSG_WAITALL);
    buffer->stream = malloc(buffer->size);

    recv(socket_cliente, buffer->stream, buffer->size, 0);
    void *stream = buffer->stream;

    memcpy(&(pcb->pid), stream, sizeof(int));
    stream += sizeof(int);
    memcpy(&(pcb->quantum), stream, sizeof(int));
    stream += sizeof(int);
    if (restante_quantum > 0)
    { // hay veces que queda en negativo el quantum
        pcb->quantum = restante_quantum;
        log_info(logger, "Quantum: %i", pcb->quantum);
    }
    else
    {
        pcb->quantum = config_get_int_value(config, "QUANTUM");
    }
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

    bool is_pid(void *elemento)
    {
        return ((elemento_cola_io *)elemento)->pcb->pid == pid_a_desbloquear; // no hay colores pq vscode no se la banca, no es bug
    };
    t_cola_io *struct_cola = dictionary_get(dictionary_pcbs_bloqueado, nombre_io);
    t_list *lista_blocked_del_io = struct_cola->cola_de_io_pedido;
    sem_t *sem = struct_cola->elementos_cola_io;
    sem_wait(sem);
    elemento_cola_io *elemento_borrado = list_remove_by_condition(lista_blocked_del_io, is_pid);
    pcb_t *pcb_a_desbloquear = elemento_borrado->pcb;
    int quantum = config_get_int_value(config, "QUANTUM");
    if (pcb_a_desbloquear->quantum == quantum) // hay veces que queda en negativo el quantum TENER EN CUENTA
    {
        pthread_mutex_lock(&mutex_lista_ready);
        list_add(lista_pcbs_ready, pcb_a_desbloquear);
        sem_post(&elementos_ready);
        pthread_mutex_unlock(&mutex_lista_ready);
    }
    else
    {
        list_add(lista_ready_mas, pcb_a_desbloquear); // capaz ready plus no tienq existir y solo se agregan en ready[0
        sem_post(&elementos_ready);
    }
    pcb_a_desbloquear->state = READY_S;
}
int planificar(int socket_cliente, t_strings_instruccion *instruccion_de_desalojo, char *algoritmo)
{
    pcb_t *pcb_a_ejecutar;
    // if(hay alguno en exit)(caso de wait/signal)
    if (list_is_empty(lista_ready_mas))
    {
        log_debug(logger, "Enviando a ejecutar desde normal");
        pthread_mutex_lock(&mutex_lista_ready);
        pcb_a_ejecutar = list_remove(lista_pcbs_ready, 0);
        pthread_mutex_unlock(&mutex_lista_ready);
    }
    else
    {
        log_debug(logger, "Enviando a ejecutar desde ready+");
        pcb_a_ejecutar = list_remove(lista_ready_mas, 0);
    }

    list_add(lista_pcbs_exec, pcb_a_ejecutar);
    pcb_a_ejecutar->state = EXEC_S;
    log_debug(logger, "Enviando PID %i a ejecutar", pcb_a_ejecutar->pid);

    return enviar_proceso_a_ejecutar(DISPATCH, pcb_a_ejecutar, socket_cliente, instruccion_de_desalojo, algoritmo); // se encarga de enviar y recibir el nuevo contexto actualizando lo que haga falta y el motivo de desalojo
    // esto hay q separarlo en dos funciones:enviar proceso Y recibir proceso/pcb NO.
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

bool pcb_esta_en_exit(int pid_buscado)
{
    bool is_pid(void *pcb)
    {
        return ((pcb_t *)pcb)->pid == pid_buscado; // no hay colores pq vscode no se la banca, no es bug
    };
    bool result = list_any_satisfy(lista_pcbs_exit, is_pid);
    return result;
}

// Server
void *client_handler(void *arg)
{
    int socket_io = *(int *)arg;
    int modulo = handshake_Server(socket_io);
    log_info(logger, "se conecto alguna io");
    bool conexion_terminada = false;
    t_interfaz *interfaz;
    while (!conexion_terminada)
    {
        int cod_op = recibir_operacion(socket_io);
        switch (cod_op)
        {
        case CREACION_IO:
            interfaz = recibir_IO(socket_io);
            interfaz->socket = socket_io;
            dictionary_put(dictionary_ios, interfaz->nombre, interfaz); // creoq ya esta medio al pedo esta
            t_cola_io *struct_io = malloc(sizeof(t_cola_io));
            struct_io->cola_de_io_pedido = list_create();
            struct_io->elementos_cola_io = malloc(sizeof(sem_t));
            sem_init(struct_io->elementos_cola_io, 0, 0);
            dictionary_put(dictionary_pcbs_bloqueado, interfaz->nombre, struct_io);
            log_info(logger, "la io conectada se llama '%s'", interfaz->nombre);

            break;
        case FIN_IO_TASK:

            t_fin_io_task *estructura = recibir_fin_io_task(socket_io);
            if (!pcb_esta_en_exit(estructura->pid))
            {
                desbloquear_pcb(estructura->pid, estructura->nombre); // el wait elementos se hace adentro
                log_info(logger, "TERMINO LA IO del pid:%i que uso:%s", estructura->pid, estructura->nombre);
            }
            else
            { // si el proceso que la pidio esta en exit, no se intenta desbloquearlo
                log_warning(logger, "TERMINO LA IO del pid:%i que uso:%s, pero el pcb esta en exit", estructura->pid, estructura->nombre);
            }
            t_cola_io *struct_cola = dictionary_get(dictionary_pcbs_bloqueado, estructura->nombre);
            if (list_size(struct_cola->cola_de_io_pedido) != 0) // esto no va en desbloquear pcb ppq despues va a haber recursos
            {
                log_error(logger, "ENTRE AL FIN IO TASK-MANDAR OTRO TASK");
                elemento_cola_io *elemento = list_get(struct_cola->cola_de_io_pedido, 0);
                pedir_io_task(elemento->pcb->pid, interfaz, elemento->instruccion_de_bloqueo);
            }
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