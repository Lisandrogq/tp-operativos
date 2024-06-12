#ifndef UTILS_H_
#define UTILS_H_

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
#include <commons/log.h>
#include <commons/collections/list.h>
#include <commons/config.h>
#include <assert.h>
#include <utils/utils_generales.h>

extern char *nombre;
extern t_log *logger;
extern t_config *config;
extern int socket_kernel;
extern int socket_memoria;
extern int *tiempo_Unidad_Trabajo;
// Cliente
// void enviar_mensaje(char* mensaje, int socket_cliente);
io_task *gen_recibir_peticion();
void gen_resolver_peticion(int cant_sleep);
void iniciar_interfaz_generica();
void iniciar_interfaz_generica();
void enviar_operacion(int cod_op, char *mensaje, int socket_cliente);
int handshake(int socket_cliente);
t_interfaz *crear_estrcutura_io(int tipo);
void enviar_interfaz(int cod_op, t_interfaz interfaz, int socket_cliente);
io_task *recibir_pedido_io(int socket_kernel);
#endif /* UTILS_H_ */