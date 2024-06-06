#ifndef UTILS_HELLO_H_
#define UTILS_HELLO_H_

#include <stdlib.h>
#include <stdio.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
#include <pthread.h>
#include <semaphore.h>
#include <commons/log.h>

#define PUERTO_KERNEL "4444"
#define PUERTO_CPU_DISPATCH "4445"
#define PUERTO_MEMORIA "4446"
#define PUERTO_CPU_INTERRUPT "4447"
typedef struct
{
	int32_t dir_fisica;
	int tam_lectura;
} read_t;
typedef struct
{
	int32_t dir_fisica;
	int tam_escritura;
	void *datos;
} write_t;
typedef enum
{
	MEM_W_OK,
	MEM_W_NO_OK,//NI IDEA SI HAY MAS
} status_write_mem;
typedef enum
{
	GENERICA,
	STDIN,
	STDOUT,
	FS,
} tipo_interfaz;
typedef enum
{
	DISPONIBLE,
	BLOQUEADO,
	EXIT,
} estado_interfaz;
typedef struct
{
	int tamcod;
	char *cod_instruccion;
	int tamp1;
	char *p1;
	int tamp2;
	char *p2;
	int tamp3;
	char *p3;
	int tamp4;
	char *p4;
	int tamp5;
	char *p5;
} t_strings_instruccion;
typedef struct
{
	int pid;
	int pc;
} fetch_t;
typedef struct
{
	int tam;
	char *path;
	int pid;
} solicitud_creacion_t;

typedef struct
{
	int pid;
	int motivo;
} interrupcion_t;
/* typedef enum   // AL FINAL  ESTO SE HACE CON STRINGS
{
	SET,
	MOV_IN,
	MOV_OUT,
	SUM,
	SUB,
	JNZ,
	RESIZE,
	COPY_STRING,
	WAIT,
	SIGNAL,
	IO_GEN_SLEEP,
	IO_STDIN_READ,
	IO_STDOUT_WRITE,
	IO_FS_CREATE,
	IO_FS_DELETE,
	IO_FS_TRUNCATE,
	IO_FS_WRITE,
	IO_FS_READ,
	EXIT_P,
} instruction_code; */
typedef struct
{
	int socket; // no estoy seguro si hace falta,mepaqsi
	char *nombre;
	int estado;
	int tipo;
} t_interfaz;

typedef struct
{
	char *nombre;
	int pid;
	// capaz status o algo asi
} t_fin_io_task;

typedef struct
{
	t_strings_instruccion *instruccion;
	int pid_solicitante;
} io_task;
typedef enum
{
	MENSAJE,
	PAQUETE,
	OPERACION_IO_1,
	OPERACION_KERNEL_1,
	CREAR_PCB,
	CREAR_ESTRUC_ADMIN,
	OPERACION_CPU_1,
	ELIMINAR_PCB,
	DISPATCH,
	DISPATCH_RESPONSE,
	FETCH,
	SIGUENTE_INSTRUCCION,
	ELIMINAR_ESTRUC_ADMIN,
	CREACION_IO,
	IO_GEN_SLEEP,
	FIN_IO_TASK,
	INTERRUPCION,
	GET_FRAME,
	READ_MEM,
	READ_MEM_RESPONSE,
	WRITE_MEM,
	WRITE_MEM_RESPONSE,
} op_code;

typedef enum
{
	IO_OK,
	IO_ERROR,
} status_fin_io_task;
typedef enum
{
	PRUEBA,
	CLOCK,
	ENTRADA_SALIDA,
	SUCCESS,
	INVALID_RESOURCE,
	INVALID_INTERFACE,
	OUT_OF_MEMORY,
	INTERRUPTED_BY_USER,
	IO_TASK,
	// ELIMINAR_ESTRUC_ADMIN,
} motivos_desalojo;

typedef enum
{
	STATUS_OK,
	STATUS_DESALOJADO,
} status_ciclo_ejecucion;
typedef enum
{
	HS_KERNEL,
	HS_CPU,
	HS_MEMORIA,
	HS_IO,
} hs_code;

// Estados de pcb//
typedef enum
{
	NEW_S,
	READY_S,
	EXEC_S,
	BLOCK_S,
	EXIT_S
} state_t;

typedef struct
{
	uint32_t PC;
	uint8_t AX;
	uint8_t BX;
	uint8_t CX;
	uint8_t DX;
	uint32_t EAX;
	uint32_t EBX;
	uint32_t ECX;
	uint32_t EDX;
	uint32_t SI;
	uint32_t DI;
} registros_t;

typedef struct
{
	int pid;
	int quantum;
	registros_t *registros;
	state_t state;
} pcb_t;

typedef struct
{
	int size;
	int offset;
	void *stream;
} t_buffer;
typedef struct
{
	op_code codigo_operacion;
	t_buffer *buffer;
} t_paquete;

pcb_t *recibir_paquete(int socket_cliente);
void eliminar_pcb(pcb_t *pcb);
void eliminar_paquete(t_paquete *paquete);
void crear_buffer(t_paquete *paquete);

/**
 * @fn    decir_hola
 * @brief Imprime un saludo al nombre que se pase por parámetro por consola.
 */
void decir_hola(char *quien);
int crear_conexion(char *ip, char *puerto, t_log *logger);
int esperar_cliente(int socket_servidor, void *client_handler(void *));
int iniciar_servidor(char *PUERTO, t_log *logger);
t_paquete *crear_paquete(void);
void agregar_a_paquete(t_paquete *paquete, void *valor, int tamanio);
void *serializar_paquete(t_paquete *paquete, int bytes);
void enviar_paquete(t_paquete *paquete, int socket_cliente);
void liberar_conexion(int socket_cliente);
int recibir_operacion(int);
#endif
