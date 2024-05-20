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

typedef enum
{ // habría q ver como podemos unificar esto en un solo archivo
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
} instruction_code;

typedef enum
{ // habría q ver como podemos unificar esto en un solo archivo
	MENSAJE,
	PAQUETE,
	OPERACION_IO_1,
	OPERACION_KERNEL_1,
	CREAR_PCB,
	CREAR_ESTRUC_ADMIN,
	OPERACION_CPU_1,
	ELIMINAR_PCB,
} op_code;

typedef enum
{ // habría q ver como podemos unificar esto en un solo archivo
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
	void* stream;
} t_buffer;
typedef struct
{
	op_code codigo_operacion;
	t_buffer *buffer;
} t_paquete;

pcb_t *crear_pcb(int pid);
void eliminar_pcb(pcb_t* pcb);
void eliminar_paquete(t_paquete* paquete);

/**
 * @fn    decir_hola
 * @brief Imprime un saludo al nombre que se pase por parámetro por consola.
 */
void decir_hola(char *quien);
int crear_conexion(char *ip, char *puerto, t_log *logger);
int esperar_cliente(int socket_servidor, void *client_handler(void *));
int iniciar_servidor(char *PUERTO, t_log *logger);
#endif
