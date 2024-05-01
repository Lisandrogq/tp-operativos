#include "utils.h"
#include <errno.h>
t_log *logger;

void *client_handler(void *arg)
{
	int socket_cliente = *(int *)arg;
	int modulo = handshake_Server(socket_cliente);
	switch (modulo) // se debería ejecutar un handler para cada modulo
	{
	case 0:
		log_info(logger, "se conecto el modulo kernel");
		break;
	case 1:
		log_info(logger, "se conecto el modulo cpu");
		break;
	case 2:
		log_info(logger, "se conecto el modulo memoria");
		break;
	case 3:
		log_info(logger, "se conecto el modulo io");
		break;
	default:
		log_warning(logger, "Cliente desconocido por memoria server.");
		return;
	}

	bool conexion_terminada = false;
	while (!conexion_terminada)
	{
		int cod_op = recibir_operacion(socket_cliente);

		switch (cod_op)
		{
		case OPERACION_KERNEL_1:
			// capaz habria q cambiar el nombre de recibir_(...) a manejar_(...)
			recibir_operacion1(socket_cliente);
			break;
		case OPERACION_CPU_1:
			// capaz habria q cambiar el nombre de recibir_(...) a manejar_(...)
			recibir_operacion1(socket_cliente);
			break;
		case OPERACION_IO_1:
			// capaz habria q cambiar el nombre de recibir_(...) a manejar_(...)
			recibir_operacion1(socket_cliente);
			break;
		case MENSAJE:
			// capaz habria q cambiar el nombre de recibir_(...) a manejar_(...)
			recibir_mensaje(socket_cliente);
			break;
		case -1:
			log_info(logger, "Se desconecto algun cliente");
			conexion_terminada = true;
			break;
		default:
			log_warning(logger, "Operacion desconocida. No quieras meter la pata");
			break;
		}
	}
	close(socket_cliente);
}

int recibir_operacion(int socket_cliente)
{
	int cod_op;
	if (recv(socket_cliente, &cod_op, sizeof(int), MSG_WAITALL) > 0)
	{
		return cod_op;
	}
	else
	{
		close(socket_cliente);
		return -1;
	}
}

int handshake_Server(int socket_cliente)
{

	size_t bytes;

	int32_t handshake;
	int32_t resultOk = 0;
	int32_t resultError = -1;

	bytes = recv(socket_cliente, &handshake, sizeof(int32_t), MSG_WAITALL);

	if (handshake == HS_KERNEL  || handshake == HS_CPU || handshake == HS_IO)
	{
		bytes = send(socket_cliente, &resultOk, sizeof(int32_t), 0);
	}
	else
	{
		bytes = send(socket_cliente, &resultError, sizeof(int32_t), 0);
	}
	return handshake;
}

void recibir_mensaje(int socket_cliente)
{
	int size;
	char *buffer = recibir_buffer(&size, socket_cliente);
	log_info(logger, "Me llego el mensaje: %s", buffer);
	free(buffer);
}

void recibir_operacion1(int socket_cliente)
{
	int size;
	char *buffer = recibir_buffer(&size, socket_cliente);
	log_info(logger, "Me llego la operacion uno, la informacion enviada fue: %s", buffer);
	free(buffer);
}

void *recibir_buffer(int *size, int socket_cliente)
{
	void *buffer;

	recv(socket_cliente, size, sizeof(int), MSG_WAITALL);
	buffer = malloc(*size);
	recv(socket_cliente, buffer, *size, MSG_WAITALL);

	return buffer;
}
