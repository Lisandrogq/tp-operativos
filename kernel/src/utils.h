#ifndef UTILS_H_
#define UTILS_H_

#include<stdio.h>
#include<stdlib.h>
#include<signal.h>
#include<unistd.h>
#include<sys/socket.h>
#include<netdb.h>
#include<string.h>
#include<commons/log.h>
#include<commons/collections/list.h>
#include<assert.h>

#include <utils/utils_generales.h>




typedef enum
{	//habría q ver como podemos unificar esto en un solo archivo
	MENSAJE,
	PAQUETE,
	OPERACION_IO_1, 
	OPERACION_KERNEL_1,
	OPERACION_CPU_1
}op_code;

//Cliente
typedef struct
{
	int size;
	void* stream;
} t_buffer;

typedef struct
{
	op_code codigo_operacion;
	t_buffer* buffer;
} t_paquete;


//Cliente
int crear_conexion(char* ip, char* puerto);
void enviar_operacion(int cod_op,char *mensaje, int socket_cliente);
int handshake(int socket_cliente);
t_paquete* crear_paquete(void);
void agregar_a_paquete(t_paquete* paquete, void* valor, int tamanio);
void enviar_paquete(t_paquete* paquete, int socket_cliente);
void liberar_conexion(int socket_cliente);
void eliminar_paquete(t_paquete* paquete);


//Server
extern t_log* logger;

void* recibir_buffer(int*, int);

int iniciar_servidor(void);
int esperar_cliente(int);
t_list* recibir_paquete(int);
void recibir_mensaje(int);
int recibir_operacion(int);
int handshake_Server(int);
int terminarServidor(int,int);
void recibir_operacion1(int socket_cliente);

#endif /* UTILS_H_ */


