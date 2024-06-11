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

extern t_dictionary *dic_p_registros;
extern t_dictionary *dic_tam_registros;
extern int socket_memoria;

extern sem_t hay_proceso;
extern sem_t desalojar;
extern pcb_t *pcb_exec;
extern int socket_dispatch;
extern int socket_interrupt;
extern int tam_pagina;
// Cliente
interrupcion_t *recibir_interrupcion(int socket_interrupt);
t_strings_instruccion *fetch(int PC);
int decode(t_strings_instruccion *instruccion);
int execute(t_strings_instruccion *instruccion);
void check_intr();
t_dictionary *inicializar_diccionario(registros_t *registros);
void devolver_pcb(int motivo_desalojo, pcb_t pcb, int socket_cliente, t_strings_instruccion *instruccion);
solicitud_unitaria_t* execute_mov_in(solicitud_unitaria_t*sol);
int execute_mov_out(solicitud_unitaria_t*sol);
void execute_set(char *nombre_r_destino, int valor);
void execute_sum(char *nombre_r_destino, char *nombre_r_origen);
void execute_sub(char *nombre_r_destino, char *nombre_r_origen);
void execute_jnz(char *nombre_r, uint32_t nuevo_pc, registros_t *contexto);
void solicitar_leer_memoria(u_int32_t dir_fisica, int tam_r_datos);
void solicitar_escribir_memoria(void *datos, u_int32_t dir_fisica, int tam_r_datos);
void *recibir_datos_leidos();
int recibir_status_escritura();
int obtener_direccion_fisica(int dir_logica);
t_list *obtener_direcciones_fisicas(int dir_logica, int tam_r_datos);
t_list *obtener_direcciones_fisicas_write(void *datos, int dir_logica, int tam_r_datos);
int recibir_frame();
solicitud_unitaria_t *solicitar_frame_iterable(solicitud_unitaria_t *sol);
int solicitar_frame(int pagina);
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
void log_instruccion_ejecutada(t_strings_instruccion *palabras);
void log_fetch_instruccion();
#endif /* UTILS_H_ */
