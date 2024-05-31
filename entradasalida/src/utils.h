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
#include <assert.h>
#include <utils/utils_generales.h>

extern char*nombre;
typedef enum{
    GENERICA,
    STDIN,
    STDOUT,
    FS,
}tipo_interfaz;
typedef enum{
    DISPONIBLE,
    BLOQUEADO,
    EXIT,
}estado_interfaz;
// Cliente
// void enviar_mensaje(char* mensaje, int socket_cliente);
void iniciar_interfaz_generica();
void enviar_operacion(int cod_op, char *mensaje, int socket_cliente);
int handshake(int socket_cliente);
t_interfaz *crear_estrcutura_io(int tipo);
void enviar_interfaz(int cod_op, t_interfaz interfaz, int socket_cliente);
#endif /* UTILS_H_ */
