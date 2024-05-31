#ifndef UTILS_H_
#define UTILS_H_

#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netdb.h>
#include <commons/log.h>
#include <commons/collections/dictionary.h>
#include <string.h>
#include <assert.h>
#include <utils/utils_generales.h>
#include <pthread.h>
#include <semaphore.h>
extern RETARDO_RESPUESTA;
extern t_log *logger;
extern t_dictionary *dictionary_codigos;
extern t_list *sems_espera_creacion_codigos;

int enviar_instruccion(char **palabras, int socket_cliente);
char** get_siguiente_instruction(fetch_t *p_info, int socket_cliente);
fetch_t *recibir_process_info(int socket_cliente);
void int_to_char(int pid,char*pid_str);
struct_administrativas * recibir_estructuras_administrativas(int socket_memoria);
void crear_estructuras_administrativas(struct_administrativas *e_admin);
void handle_kerel_client(int socket_memoria);
void *client_handler(void *arg);
void handle_crear_pcb(int socket_cliente);
void *recibir_buffer(int *, int);
void* client_handler(void *arg);
void recibir_mensaje(int);
void recibir_operacion1(int socket_cliente);
int handshake_Server(int);
int terminarServidor(int, int);
pcb_t *recibir_paquete(int socket_cliente);
#endif /* UTILS_H_ */
