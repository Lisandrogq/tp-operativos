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
#include <readline/readline.h>
#include <commons/bitarray.h>
#include <sys/mman.h>  
#include <math.h>
extern char *nombre;
extern t_log *logger;
extern t_config *config;
extern int socket_kernel;
extern int socket_memoria;
extern int tiempo_Unidad_Trabajo;
// Cliente
// void enviar_mensaje(char* mensaje, int socket_cliente);
void inicializar_bloques();
int get_next_free_block();
void inicializar_bitmap();
void ocupar_bloque(int index);
void liberar_bloque(int index);
char* get_complete_path(char*nombre);
void liberar_bloques_desde(int bloque_inicial, int bloques_a_liberar);
char *get_complete_path(char *nombre);
void eliminar_archivo(char *nombre);
void crear_metadata(char *nombre,int bloque);
solicitud_unitaria_t *leer_memoria_unitario(solicitud_unitaria_t *sol);
void *recibir_datos_leidos();
void solicitar_leer_memoria(u_int32_t dir_fisica, int tam_r_datos, int pid);
void solicitar_escribir_memoria(void *datos, u_int32_t dir_fisica, int tam_r_datos, int pid);
void leer_memoria(t_list *solicitudes, void *datos);
void escribir_memoria(t_list *solicitudes);
void populate_solicitudes(t_list *solicitudes, char *input_string);
t_list *decode_addresses_buffer(buffer_instr_io_t *buffer_instruccion, int *max_tam);
void informar_fin_de_tarea(int socket_kernel, int status, int pid);
int inicializar_cliente_kernel();
void inicializar_cliente_memoria();
io_task *gen_recibir_peticion();
void gen_resolver_peticion(int cant_sleep);
void iniciar_interfaz_generica();
io_task *recibir_peticion();
void iniciar_interfaz_generica();
void enviar_operacion(int cod_op, char *mensaje, int socket_cliente);
int handshake(int socket_cliente);
t_interfaz *crear_estrcutura_io(int tipo);
void enviar_interfaz(int cod_op, t_interfaz interfaz, int socket_cliente);
io_task *recibir_pedido_io(int socket_kernel);
#endif /* UTILS_H_ */