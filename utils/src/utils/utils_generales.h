#ifndef UTILS_HELLO_H_
#define UTILS_HELLO_H_

#include <stdlib.h>
#include <stdio.h>
#include<stdio.h>
#include<stdlib.h>
#include<signal.h>
#include<unistd.h>
#include<sys/socket.h>
#include	<netdb.h>
#include	<string.h>
#include	<pthread.h>
#include <semaphore.h>
#include<commons/log.h>



#define PUERTO_KERNEL "4444"
#define PUERTO_CPU_DISPATCH "4445"
#define PUERTO_MEMORIA "4446"
#define PUERTO_CPU_INTERRUPT "4447"


typedef enum
{	//habría q ver como podemos unificar esto en un solo archivo
	MENSAJE,
	PAQUETE,
	OPERACION_IO_1, 
	OPERACION_KERNEL_1,
	OPERACION_CPU_1
}op_code;

typedef enum
{	//habría q ver como podemos unificar esto en un solo archivo
	HS_KERNEL,
	HS_CPU,
	HS_MEMORIA, 
	HS_IO,
}hs_code;

//estructura de pcb//
typedef struct 
{
	int pid;
	int program_counter;
	int quantum;
	registrosCPU* registrosCPU,
}pcb;	

//funcion de crear pcb//
pcb* CrearPCB(int pid, int program_counter, int quantum, registrosCPU* registrosCPU);
//void DestruirPCB(pcb* pcb);

//creo regsitrosCPU//
typedef struct 
{
	uint32_t PC[4];
	uint8_t AX[1];
	uint8_t BX[1];
	uint8_t CX[1];
	uint8_t DX[1];
	uint32_t EAX[4];
	uint32_t EBX[4];
	uint32_t ECX[4];
	uint32_t EDX[4];
	uint32_t SI[4];
	uint32_t DI[4];
}registrosCPU;

typedef struct registros_generales
{
	int pid;
	int program_counter;
	int quantum;
};


/**
* @fn    decir_hola
* @brief Imprime un saludo al nombre que se pase por parámetro por consola.
*/
void decir_hola(char* quien);
int crear_conexion(char *ip, char *puerto,t_log *logger);
int esperar_cliente(int socket_servidor, void* client_handler (void*));
int iniciar_servidor(char* PUERTO,t_log *logger);
#endif
