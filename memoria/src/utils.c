#include "utils.h"
#include <errno.h>

t_log *logger;
t_dictionary *dictionary_codigos;
char *leer_codigo(char *path)
{
	// leer el pseudocodigo
	FILE *file;
	fopen(path, "r");
	char *codigo;
	file = fopen("file.txt", "r");

	fseek(file, 0, SEEK_END);
	int file_size = ftell(file);
	fseek(file, 0, SEEK_SET);
	codigo = malloc(file_size);
	memset(codigo, 0, file_size);

	char linea[100];
	while (fgets(linea, 100, file))
	{
		strcat(codigo, linea);
	}

	printf("%s\n", codigo);
	return codigo;
	fclose(file);
}
void int_to_char(int pid, char *pid_str)
{

	snprintf(pid_str, sizeof(pid_str), "%d", pid);
}
void crear_estructuras_administrativas(struct_administrativas *e_admin)
{
	char *codigo = leer_codigo(e_admin->path);
	char pid_str[5] = "";
	int_to_char(e_admin->pid, pid_str);
	dictionary_put(dictionary_codigos, pid_str, codigo);
}
void handle_cpu_client(int socket_cliente)
{
	log_info(logger, "ESTOY EN EL HANDLER DEL CPU");

	bool conexion_terminada = false;
	while (!conexion_terminada)
	{
		int cod_op = recibir_operacion(socket_cliente);
		log_info(logger, "codop:%i", cod_op);
		switch (cod_op)
		{

		case FETCH:
			log_info(logger, "recibi un fetch!!!!!");
			fetch_t *p_info = recibir_process_info(socket_cliente);
			log_info(logger, "after");
			log_info(logger, "pid:%i", p_info->pid);
			log_info(logger, "pc:%i", p_info->pc);
			devolver_siguiente_instruction(p_info, socket_cliente);
			break;
		case -1:
			return -1;
		default:
			break;
		}
	}
}
void handle_kerel_client(int socket)
{
	bool conexion_terminada = false;
	while (!conexion_terminada)
	{
		int cod_op = recibir_operacion(socket);
		switch (cod_op)
		{
		case CREAR_ESTRUC_ADMIN:
			struct_administrativas *e_admin = recibir_estructuras_administrativas(socket);
			crear_estructuras_administrativas(e_admin);
			break;

		default:
			break;
		}
	}
}
void *client_handler(void *arg)
{
	int socket_cliente = *(int *)arg;
	int modulo = handshake_Server(socket_cliente);
	switch (modulo) // se debería ejecutar un handler para cada modulo
	{
	case 0:
		log_info(logger, "se conecto el modulo kernel");
		handle_kerel_client(socket_cliente);
		break;
	case 1:
		log_info(logger, "se conecto el modulo cpu");
		handle_cpu_client(socket_cliente);
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

	// bool conexion_terminada = false;
	// while (!conexion_terminada)
	// {
	// 	int cod_op = recibir_operacion(socket_cliente);
	// 	log_warning(logger, "codop:%i", cod_op);
	// 	switch (cod_op)
	// 	{
	// 	case OPERACION_KERNEL_1:
	// 		// capaz habria q cambiar el nombre de recibir_(...) a manejar_(...)
	// 		recibir_operacion1(socket_cliente);
	// 		break;
	// 	case OPERACION_CPU_1:
	// 		// capaz habria q cambiar el nombre de recibir_(...) a manejar_(...)
	// 		recibir_operacion1(socket_cliente);
	// 		break;
	// 	case OPERACION_IO_1:
	// 		// capaz habria q cambiar el nombre de recibir_(...) a manejar_(...)
	// 		recibir_operacion1(socket_cliente);
	// 		break;
	// 	case MENSAJE:
	// 		// capaz habria q cambiar el nombre de recibir_(...) a manejar_(...)
	// 		recibir_mensaje(socket_cliente);
	// 		break;
	// 	case CREAR_PCB:
	// 		// capaz habria q cambiar el nombre de recibir_(...) a manejar_(...)
	// 		pcb_t *pcb = malloc(sizeof(pcb_t));
	// 		pcb = recibir_paquete(socket_cliente);
	// 		log_info(logger, "Pid: %i", pcb->pid);
	// 		log_info(logger, "Quantum: %i", pcb->quantum);
	// 		log_info(logger, "Registros: %i", pcb->registros->AX);
	// 		// FREEE??
	// 		break;
	// 	case ELIMINAR_PCB:
	// 		pcb = recibir_paquete(socket_cliente);
	// 		eliminar_pcb(pcb);
	// 		break;
	// 	case -1:
	// 		log_info(logger, "Se desconecto algun cliente");
	// 		conexion_terminada = true;
	// 		break;
	// 	default:
	// 		log_warning(logger, "Operacion desconocida. No quieras meter la pata");
	// 		break;
	// 	}
	// }
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

	if (handshake == HS_KERNEL || handshake == HS_CPU || handshake == HS_IO)
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

void handle_crear_pcb(int socket_cliente)
{
	int size;
	int *buffer = recibir_buffer(&size, socket_cliente);
	log_info(logger, "Me llego la operacion crear_pcb, el pid es: %s", buffer);
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
char *get_linea_buscada(const char *input_string, int linea_buscada)
{
	char *lines[100];
	int line_count = 0;
	char *copy = strdup(input_string); // Make a copy of input_string to avoid modifying the original
	char *token = strtok(copy, "\n");

	while (token != NULL && line_count <= linea_buscada)
	{
		lines[line_count] = strdup(token);
		line_count++;
		token = strtok(NULL, "\n");
	}

	free(copy); // Free the memory allocated by strdup

	return lines[linea_buscada];
}

void devolver_siguiente_instruction(fetch_t *p_info, int socket_cliente)
{
	char *linea; // esto debería ser dinamico y en un malloc, creo, mejro si no:p.
	char pid_str[5] = "";
	int_to_char(p_info->pid, pid_str);
	char *codigo = dictionary_get(dictionary_codigos, pid_str);
	linea = get_linea_buscada(codigo, p_info->pc); // HABRÍA QUE DIVIDIR EL CODIGO EN LINEAS AL CREAR ESTRUCTURAS ADMINISTRATIVAS,PERO NO HAY PLATA.
	log_info(logger, "LINEA:%s", linea);
}

fetch_t *recibir_process_info(int socket_cliente)
{
	t_paquete *paquete = malloc(sizeof(t_paquete));
	paquete->buffer = malloc(sizeof(t_buffer));

	recv(socket_cliente, &(paquete->buffer->size), sizeof(uint32_t), 0);
	paquete->buffer->stream = malloc(paquete->buffer->size);
	recv(socket_cliente, paquete->buffer->stream, paquete->buffer->size, 0);

	fetch_t *p_info = malloc(sizeof(fetch_t));
	void *stream = paquete->buffer->stream;
	memcpy(&(p_info->pid), stream, sizeof(int));
	stream += sizeof(int);
	memcpy(&(p_info->pc), stream, sizeof(int));

	eliminar_paquete(paquete);

	return p_info;
}

struct_administrativas *recibir_estructuras_administrativas(int socket_cliente)
{
	t_buffer *buffer = malloc(sizeof(t_buffer));
	recv(socket_cliente, &(buffer->size), sizeof(uint32_t), 0);
	buffer->stream = malloc(buffer->size);
	recv(socket_cliente, buffer->stream, buffer->size, 0);

	struct_administrativas *estructura = malloc(sizeof(pcb_t));
	void *stream = buffer->stream;
	memcpy(&(estructura->tam), stream, sizeof(int));
	stream += sizeof(int);
	estructura->path = malloc(estructura->tam);
	memcpy(&(estructura->path), stream, sizeof(int));
	stream += estructura->tam;
	memcpy(&(estructura->pid), stream, sizeof(registros_t));
	free(buffer->stream);
	free(buffer);
	return estructura;
}