#ifndef UTILS_H_
#define UTILS_H_

#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netdb.h>
#include <commons/log.h>
#include <commons/collections/dictionary.h>
#include <commons/string.h>
#include <string.h>
#include <assert.h>
#include <utils/utils_generales.h>
#include <pthread.h>
#include <commons/bitarray.h>
#include <semaphore.h>
extern RETARDO_RESPUESTA;
extern t_log *logger;
extern t_dictionary *dictionary_codigos;
extern t_list *sems_espera_creacion_codigos;
extern t_list *lista_tablas_paginas;
extern TAM_PAGINA;
extern int TAM_MEMORIA;
extern void *espacio_usuario;
extern int marcos;
extern t_bitarray *bitarray;
extern char* PATH_INSTRUCCIONES;
typedef struct
{
    int pid;
    t_list *tabla; // cada elem es de tipo int*
} elemento_lista_tablas;
resize_t * recibir_pedido_resize(int socket_cliente);
void devolver_status_resize(int status,int socket_cliente);
get_frame_t *recibir_pedido_frame(socket_cliente);
int reducir_proceso(t_list *tabla, int bytes);
int ampliar_proceso(t_list *tabla, int bytes_a_agregar);
int *calcular_frame(get_frame_t *solicitud);
void *leer_memoria(u_int32_t dir_fisica, int tam_lectura);
int escribir_memoria(void *datos, u_int32_t dir_fisica, int tam_lectura);
int enviar_instruccion(char **palabras, int socket_cliente);
char **get_siguiente_instruction(fetch_t *p_info, int socket_cliente);
fetch_t *recibir_process_info(int socket_cliente);
solicitud_creacion_t *recibir_solicitud_de_creacion(int socket_cliente);
int recibir_solicitud_de_eliminacion(int socket_cliente);
void eliminar_estrucuras_administrativas(int pid_a_eliminar);
read_t *recibir_pedido_lectura(socket_cliente);
write_t *recibir_pedido_escritura(int socket_cliente);
void enviar_status_escritura(int status, int socket_cliente);
int crear_estructuras_administrativas(solicitud_creacion_t *e_admin);
void crear_tabla_paginas(int pid);
void handler_kernel_client(int socket_memoria);
void *client_handler(void *arg);
void handle_crear_pcb(int socket_cliente);
void *recibir_buffer(int *, int);
void *client_handler(void *arg);
void recibir_mensaje(int);
void recibir_operacion1(int socket_cliente);
int handshake_Server(int);
int terminarServidor(int, int);
pcb_t *recibir_paquete(int socket_cliente);
#endif /* UTILS_H_ */
