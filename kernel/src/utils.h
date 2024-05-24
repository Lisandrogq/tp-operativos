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
#include<commons/string.h>

#include<assert.h>
#include<readline/readline.h>

#include <utils/utils_generales.h>
extern pcb_t lista_pcbs[100]; 
extern int next_pid;
extern int socket_memoria;
extern pthread_mutex_t mutex_socket_memoria;
extern int operacion;
typedef enum
{	//habría q ver como podemos unificar esto en un solo archivo
	INICIAR_PROCESO,

}kernel_opcode;
void solicitar_crear_estructuras_administrativas(int tam, char*path, int pid,int socket_memoria);
//Cliente
void iniciar_proceso(char *path, int tam);
void enviar_operacion(int cod_op ,char *mensaje, int socket_cliente);
void enviar_PCB(int cod_op, pcb_t pcb, int socket_cliente);
int handshake(int socket_cliente);
t_log* iniciar_logger(void);
t_config* iniciar_config(void);
void leer_consola(t_log*);
void paquete(int);
void terminar_programa(int, t_log*, t_config*);
void retirar_pcb_bloqueado(pcb_t pcb, int index);
int enviar_proceso_a_ejecutar(int cod_op, pcb_t *pcb, int socket_cliente);
int planificar_fifo(int socket_cliente);
int planificar_rr(int socket_cliente);
//Server
extern t_log* logger;

void* recibir_buffer(int*, int);

void* client_handler(void *arg);
void recibir_mensaje(int);
int recibir_operacion(int);
int handshake_Server(int);
int terminarServidor(int,int);
void recibir_operacion1(int socket_cliente);
pcb_t *crear_pcb(int pid);

#endif /* UTILS_H_ */


