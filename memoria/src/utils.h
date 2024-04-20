#ifndef UTILS_H_
#define UTILS_H_

#include<stdio.h>
#include<stdlib.h>
#include<sys/socket.h>
#include<unistd.h>
#include<netdb.h>
#include<commons/log.h>
#include<commons/collections/list.h>
#include<string.h>
#include<assert.h>
#include<utils/utils_generales.h>



typedef enum
{	//habría q ver como podemos unificar esto en un solo archivo
	MENSAJE,
	PAQUETE,
	OPERACION_IO_1, 
	OPERACION_KERNEL_1,
	OPERACION_CPU_1
}op_code;

extern t_log* logger;

void* recibir_buffer(int*, int);

int iniciar_servidor(void);
int esperar_cliente(int);
t_list* recibir_paquete(int);
void recibir_mensaje(int);
int recibir_operacion(int);
void recibir_operacion1(int socket_cliente);
int handshake_Server(int);
int terminarServidor(int,int);
#endif /* UTILS_H_ */
