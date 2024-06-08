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
#include <commons/string.h>
#include <commons/temporal.h>

#include <assert.h>
#include <readline/readline.h>

#include <utils/utils_generales.h>
extern t_log *logger;
extern t_config *config;
extern pcb_t lista_pcbs[100];
extern int next_pid;
extern int socket_memoria;
extern int socket_interrupt;
extern pthread_mutex_t mutex_socket_interrupt;
extern pthread_mutex_t mutex_socket_memoria;
extern pthread_mutex_t mutex_pcb_desalojado;
extern pthread_mutex_t mutex_lista_exit;
extern t_list *lista_ready_mas;
extern pthread_mutex_t mutex_lista_exec;
extern int pid_sig_term;
extern t_list *lista_pcbs_exit;
extern t_temporal *timer;
extern int operacion;
extern t_list *lista_pcbs_ready;
extern t_list *lista_pcbs_new;
extern pthread_mutex_t mutex_lista_ready;
extern sem_t hay_new;
extern sem_t elementos_ready;
extern sem_t contador_multi;				// contador de ready, si no hay, no podes planificar.
extern t_dictionary *dictionary_pcbs_bloqueado; // "Int1","int3","recurso1"...cada elemento
											/// es un struct con lista y contador y mutex de acceso
typedef struct
{

	sem_t *elementos_cola_io;
	t_list *cola_de_io_pedido;//!CADA ELEMENTO ES UN''elemento_cola_io''
} t_cola_io;

typedef struct
{

	t_strings_instruccion* instruccion_de_bloqueo;
	pcb_t*pcb;//cada elemento:(pcb+instruccion)
} elemento_cola_io;

typedef struct
{
	int tam;
	char *path;
	pcb_t *pcb;
} elemento_cola_new;


//extern pthread_mutex_t mutex_lista_bloqueado;
extern int socket_interrupt;
extern t_list *lista_pcbs_exec;
extern t_dictionary *dictionary_ios;
typedef enum
{ // habría q ver como podemos unificar esto en un solo archivo
	INICIAR_PROCESO,

} kernel_opcode;
bool pcb_esta_en_exit(int pid);
void solicitar_eliminar_estructuras_administrativas(int pid);
int es_una_io_valida(int pid, t_strings_instruccion *instruccion);
t_fin_io_task *recibir_fin_io_task(int socket_cliente);
void solicitar_crear_estructuras_administrativas(int tam, char *path, int pid, int socket_memoria);
// Cliente
void comando_iniciar_proceso(char *path, int tam);
void comando_finalizar_proceso(char *pid_str, int motivo);
void comando_listar_procesos_por_estado();
void modificar_multiprogramacion(int grado);
void ejecutar_script(char *path);
void enviar_operacion(int cod_op, char *mensaje, int socket_cliente);
void enviar_PCB(int cod_op, pcb_t pcb, int socket_cliente);
int handshake(int socket_cliente);
t_log *iniciar_logger(void);
t_config *iniciar_config(void);
void leer_consola(t_log *);
void paquete(int);
void terminar_programa(int, t_log*, t_config*);
void desbloquear_pcb(int pid_a_desbloquear, char *nombre_io);
int enviar_proceso_a_ejecutar(int cod_op, pcb_t *pcb, int socket_cliente,t_strings_instruccion*instruccion_de_desalojo, char* algoritmo);
int planificar(int socket_cliente,t_strings_instruccion*instruccion_de_desalojo , char* algoritmo);
void *hilo_quantum(void* parametro);
//Server
extern t_log* logger;
void enviar_interrupcion(int motivo, int pid);

void *recibir_buffer(int *, int);

void *client_handler(void *arg);
void recibir_mensaje(int);
int handshake_Server(int);
int terminarServidor(int, int);
void recibir_operacion1(int socket_cliente);
pcb_t *crear_pcb(int pid);
t_interfaz *recibir_IO(int socket_cliente);
pedir_io_task(int pid, t_interfaz *io, t_strings_instruccion *instruccion);

#endif /* UTILS_H_ */
