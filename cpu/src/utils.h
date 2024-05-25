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


extern t_dictionary *dictionary;
extern sem_t hay_proceso;
extern sem_t desalojar;
extern registros_t *contexto;
extern int pid_exec;

// Cliente
t_strings_instruccion *fetch(int PC);
void decode();
int execute(t_strings_instruccion *instruccion);
void check_intr();
t_dictionary *inicializar_diccionario(registros_t *contexto);

void execute_set(char *nombre_r_destino, int valor);
void execute_sum(char *nombre_r_destino, char *nombre_r_origen);
void execute_sub(char *nombre_r_destino, char *nombre_r_origen);
void execute_jnz(char *nombre_r,uint32_t nuevo_pc, registros_t *contexto);
// Cliente
void enviar_operacion(int cod_op, char *mensaje, int socket_cliente);
int handshake(int socket_cliente);
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
int handshake_Server(int);
int terminarServidor(int, int);
void recibir_operacion1(int socket_cliente);


#endif /* UTILS_H_ */
