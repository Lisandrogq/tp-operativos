#ifndef UTILS_H_
#define UTILS_H_

#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netdb.h>
#include <commons/log.h>
#include <commons/collections/list.h>
#include <string.h>
#include <assert.h>
#include <utils/utils_generales.h>
#include <pthread.h>
#include <semaphore.h>

extern t_log *logger;
void handle_crear_pcb(int socket_cliente);
void *recibir_buffer(int *, int);
void* client_handler(void *arg);
void recibir_mensaje(int);
int recibir_operacion(int);
void recibir_operacion1(int socket_cliente);
int handshake_Server(int);
int terminarServidor(int, int);
pcb_t *recibir_paquete(int socket_cliente);
#endif /* UTILS_H_ */
