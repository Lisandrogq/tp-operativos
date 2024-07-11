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
#include <dirent.h>
extern char *nombre;
extern t_log *logger;
extern t_config *config;
extern int socket_kernel;
extern int socket_memoria;
extern int tiempo_Unidad_Trabajo;
extern int RETRASO_COMPACTACION;

typedef struct
{
    char *file;
    int bytes;
} truncate_t;
typedef struct
{
    int max_tam;
    char *nombre;
    int puntero_archivo;
    t_list* solicitudes;
} fs_rw_sol_t;
typedef struct
{
    int gap_start;
    int next_busy_block;
} gap_t;

// Cliente
void inicializar_bloques();
int get_next_free_block(int inicio_busqueda);
void inicializar_bitmap();

t_dictionary *obtener_lista_files(char *a_excluir);
void ocupar_bloque(int index);
bool es_posible_agrandar(char *nombre, int bloques_actuales, int bloques_a_agregar);
void reducir_archivo(char *nombre, int bloques_actuales, int bloques_a_reducir, int nuevos_bytes);
fs_rw_sol_t *decode_buffer_rw_sol(buffer_instr_io_t *buffer_instruccion);
void *leer_archivo(char *nombre, int puntero, int tam);
void agrandar_archivo(char *nombre, int bloques_actuales, int bloques_a_agregar, int nuevos_bytes);
void truncar_archivo(truncate_t *sol, int pid_solicitante);

void compactar(char *nombre);
char *get_complete_path(char *nombre);
int get_puntero_base(char *nombre);
truncate_t *decode_buffer_truncate_sol(buffer_instr_io_t *buffer_instruccion);
int liberar_bloques(char *nombre);
int ocupar_bloques(int inicio, int cant_bloques);
void actualizar_tamanio(char *nombre, int nuevos_bytes);
void actualizar_metadata(char *nombre, int nuevo_inicio);
int get_next_busy_block(int inicio_busqueda);
void dezplazar_file(int nuevo_inicio, char *nombre);
char *get_complete_path(char *nombre);
void eliminar_archivo(char *nombre);
void desocupar_bloque(int index);
void crear_metadata(char *nombre, int bloque);
char *decode_buffer_file_name(buffer_instr_io_t *buffer_instruccion);
int decode_operation(buffer_instr_io_t *buffer_instruccion);
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