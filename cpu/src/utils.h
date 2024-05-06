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
#include <readline/readline.h>

#include <commons/string.h>
#include <utils/utils_generales.h>

// Cliente
typedef struct
{
	int cod_instruccion;
	void *p1;
	void *p2;
	void *p3;
	void *p4;
	void *p5;
} t_instruccion;

t_instruccion *fetch(int conexion_fd, int PC);
void decode();
void execute(t_instruccion *instruccion);
void check_intr();

void execute_set(uint32_t *r_destino, int valor);
void execute_sum(uint32_t *r_destino, uint32_t *r_origen);
void execute_sub(uint32_t *r_destino, uint32_t *r_origen);
void execute_jnz(uint32_t *registro, uint32_t *nuevo_pc, registros_t *contexto);
// Cliente
void enviar_operacion(int cod_op, char *mensaje, int socket_cliente);
int handshake(int socket_cliente);
t_paquete *crear_paquete(void);
void agregar_a_paquete(t_paquete *paquete, void *valor, int tamanio);
void enviar_paquete(t_paquete *paquete, int socket_cliente);
void liberar_conexion(int socket_cliente);
t_log *iniciar_logger(void);
t_config *iniciar_config(void);
void leer_consola(t_log *);
void paquete(int);
void terminar_programa(int, t_log *, t_config *);

// Server
extern t_log *logger;
int esperar_cliente_cpu(int socket_servidor);
void *recibir_buffer(int *, int);
void *client_handler_interrupt(int socket_cliente);
void *client_handler_dispatch(int socket_cliente);
void recibir_mensaje(int);
int recibir_operacion(int);
int handshake_Server(int);
int terminarServidor(int, int);
void recibir_operacion1(int socket_cliente);

#endif /* UTILS_H_ */
