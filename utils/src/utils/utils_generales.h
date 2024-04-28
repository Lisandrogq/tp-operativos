#ifndef UTILS_HELLO_H_
#define UTILS_HELLO_H_

#include <stdlib.h>
#include <stdio.h>
#include<stdio.h>
#include<stdlib.h>
#include<signal.h>
#include<unistd.h>
#include<sys/socket.h>
#include<netdb.h>
#include<string.h>
#include <pthread.h>
#include <semaphore.h>
#include<commons/log.h>


#define PUERTO_KERNEL "4444"
#define PUERTO_CPU "4445"
#define PUERTO_MEMORIA "4446"

typedef enum
{	//habría q ver como podemos unificar esto en un solo archivo
	MENSAJE,
	PAQUETE,
	OPERACION_IO_1, 
	OPERACION_KERNEL_1,
	OPERACION_CPU_1
}op_code;

/**
* @fn    decir_hola
* @brief Imprime un saludo al nombre que se pase por parámetro por consola.
*/
void decir_hola(char* quien);
int esperar_cliente(int socket_servidor, void* client_handler (void*));
int iniciar_servidor(char* PUERTO,t_log *logger);
#endif
