#ifndef UTILS_H_
#define UTILS_H_

#include<stdio.h>
#include<stdlib.h>
#include<signal.h>
#include<unistd.h>
#include<sys/socket.h>
#include<netdb.h>
#include<string.h>
#include<commons/log.h>
#include<commons/collections/list.h>
#include<commons/config.h>

#include<assert.h>
#include<readline/readline.h>

#include<commons/string.h>
#include <utils/utils_generales.h>




//Cliente
typedef struct
{
	int size;
	void* stream;
} t_buffer;

typedef struct
{
	op_code codigo_operacion;
	t_buffer* buffer;
} t_paquete;


//Cliente
void enviar_operacion(int cod_op,char *mensaje, int socket_cliente);
int handshake(int socket_cliente);
t_paquete* crear_paquete(void);
void agregar_a_paquete(t_paquete* paquete, void* valor, int tamanio);
void enviar_paquete(t_paquete* paquete, int socket_cliente);
void liberar_conexion(int socket_cliente);
void eliminar_paquete(t_paquete* paquete);
t_log* iniciar_logger(void);
t_config* iniciar_config(void);
void leer_consola(t_log*);
void paquete(int);
void terminar_programa(int, t_log*, t_config*);


//Server
extern t_log* logger;
int esperar_cliente_cpu(int socket_servidor);
void* recibir_buffer(int*, int);
void *client_handler_interrupt(int socket_cliente);
void *client_handler_dispatch(int socket_cliente);
t_list* recibir_paquete(int);
void recibir_mensaje(int);
int recibir_operacion(int);
int handshake_Server(int);
int terminarServidor(int,int);
void recibir_operacion1(int socket_cliente);

#endif /* UTILS_H_ */


