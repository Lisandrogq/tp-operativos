#include "utils.h"
#include <errno.h>

t_log *logger;
t_dictionary *dictionary_codigos;
t_list *sems_espera_creacion_codigos;
int RETARDO_RESPUESTA;
t_list *lista_tablas_paginas;
int TAM_PAGINA;
int TAM_MEMORIA;
void *espacio_usuario;

char *leer_codigo(char *path)
{
	// leer el pseudocodigo
	FILE *file;

	char *codigo;
	file = fopen(path, "r");

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
void crear_estructuras_administrativas(solicitud_creacion_t *e_admin)
{
	char *codigo = leer_codigo(e_admin->path);
	char pid_str[5] = "";
	int_to_char(e_admin->pid, pid_str);
	dictionary_put(dictionary_codigos, pid_str, codigo);
	// signal
	sem_t *sem = malloc(sizeof(sem_t));
	sem_init(sem, 0, 1);
	list_add_in_index(sems_espera_creacion_codigos, e_admin->pid, sem); // los elementos nunca se
																		// borran, pq si hago remove muevo los demas(creo), solo se hace free del sem al eliminar_e_admin.
}
void eliminar_estrucuras_administrativas(int pid_a_eliminar)
{
	char pid_str[5] = "";
	int_to_char(pid_a_eliminar, pid_str);
	sem_t *sem_a_liberar = list_get(sems_espera_creacion_codigos, pid_a_eliminar);
	free(sem_a_liberar);
	char *codigo = dictionary_remove(dictionary_codigos, pid_str); // creo que no hace falta mutex para codigos
	free(codigo);
	log_debug(logger, "Se elimino el codigo del pid %i", pid_a_eliminar);
}
void handle_cpu_client(int socket_cliente)
{
	log_info(logger, "ESTOY EN EL HANDLER DEL CPU");

	bool conexion_terminada = false;
	while (!conexion_terminada)
	{
		int cod_op = recibir_operacion(socket_cliente);
		switch (cod_op)
		{

		case FETCH:
			fetch_t *p_info = recibir_process_info(socket_cliente);
			log_info(logger, "pid:%i", p_info->pid);
			log_info(logger, "pc:%i", p_info->pc);
			usleep(RETARDO_RESPUESTA); // MILISEGUNDOS PEDIDOS POR CONFIG(consigna);
			sem_t *sem = list_get(sems_espera_creacion_codigos, p_info->pid);
			sem_wait(sem);
			char **palabras = get_siguiente_instruction(p_info, socket_cliente);
			sem_post(sem);
			enviar_instruccion(palabras, socket_cliente);
			break;
		case GET_FRAME:
			// se recibe pid,npagina. se devuelve nframe.
			//(si se quisiera pedir varias paginas(por dato largo), se pide de auna)
			break;
		case READ_MEM:
			read_t *solicitud_r = recibir_pedido_lectura(socket_cliente);
			log_info(logger, "r_dir: %i|| tam: %i", solicitud_r->dir_fisica, solicitud_r->tam_lectura);
			void *datos_leidos = leer_memoria(solicitud_r->dir_fisica, solicitud_r->tam_lectura);
			log_info(logger, "datos_leidos: %d", *(u_int32_t *)datos_leidos); // aunq sea un int8 se muestra bien
			enviar_datos_leidos(datos_leidos, solicitud_r->tam_lectura, socket_cliente);
			// se recibe dirfisica,longitud, se devuelve lo leido.
			//! CREO Q SI SE TIENE Q LEER MAS DE UNA PAGINA, SE HACER POR SEPARADO!
			break; // no se recibe pid en read/write(creo)
		case WRITE_MEM:
			write_t *solicitud_w = recibir_pedido_escritura(socket_cliente);
			log_info(logger, "w_dir: %i, tam: %i, datos: %d", solicitud_w->dir_fisica, solicitud_w->tam_escritura, *(u_int32_t *)solicitud_w->datos);
			int status = escribir_memoria(solicitud_w->datos, solicitud_w->dir_fisica, solicitud_w->tam_escritura);
			enviar_status_escritura(status, socket_cliente);

			// se recibe dirfisica,longitud,dato a escribir. se devuelve OK(NO_OK TAMBIEN?).
			//! CREO Q SI SE TIENE Q ESCRIBIR MAS DE UNA PAGINA, SE HACER POR SEPARADO!
			break;
		case -1:
			return -1;
		default:
			break;
		}
	}
}

int escribir_memoria(void *datos, u_int32_t dir_fisica, int tam_lectura)
{
	int status = MEM_W_OK; // creo q hay q validar si escribe dentro del espacio de usario para devolver no_ok
	memcpy(espacio_usuario + dir_fisica, datos, tam_lectura);
	return status;
}

void *leer_memoria(u_int32_t dir_fisica, int tam_lectura)
{
	void *datos_leidos = malloc(tam_lectura);
	memcpy(datos_leidos, espacio_usuario + dir_fisica, tam_lectura);
	return datos_leidos;
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
			solicitud_creacion_t *e_admin = recibir_solicitud_de_creacion(socket);
			log_info(logger, "path:%s", e_admin->path);
			crear_estructuras_administrativas(e_admin);
			// crear tabla de paginas del pid
			elemento_lista_tablas *elemento = malloc(sizeof(elemento_lista_tablas));
			elemento->pid = e_admin->pid;
			elemento->tabla = list_create();
			list_add(lista_tablas_paginas, elemento);
			break;
		case ELIMINAR_ESTRUC_ADMIN:
			int pid_a_eliminar = recibir_solicitud_de_eliminacion(socket);
			eliminar_estrucuras_administrativas(pid_a_eliminar);
			// TODO: eliminar tabla
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

int handshake_Server(int socket_cliente)
{

	size_t bytes;

	int32_t handshake;
	int32_t resultOk = 0;
	int32_t resultError = -1;

	bytes = recv(socket_cliente, &handshake, sizeof(int32_t), MSG_WAITALL);

	if (handshake == HS_KERNEL || handshake == HS_IO)
	{
		bytes = send(socket_cliente, &resultOk, sizeof(int32_t), 0);
	}
	else if (handshake == HS_CPU) // si se conecta la cpu, se le informa el tamaño de pagina
	{
		bytes = send(socket_cliente, &TAM_PAGINA, sizeof(int32_t), 0);
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
	char *copy = strdup(input_string);
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
char **separar_linea_en_parametros(const char *input_string)
{
	char **palabras = malloc(sizeof(char *) * 6); // max 1 instruccion + 5 params
	memset(palabras, 0, sizeof(char *) * 6);
	int word_count = 0;
	char *copy = strdup(input_string);
	char *token = strtok(copy, " ");

	while (token != NULL && word_count <= 5)
	{
		palabras[word_count] = strdup(token);
		word_count++;
		token = strtok(NULL, " ");
	}
	free(copy); // Free the memory allocated by strdup

	/* for (int i = 0; i < 6; i++)
	{
		log_warning(logger, "linea %i: %s", i, palabras[i]);
	} */
	return palabras;
}
char **get_siguiente_instruction(fetch_t *p_info, int socket_cliente)
{
	char *linea; // esto debería ser dinamico y en un malloc, creo, mejro si no:p.
	char pid_str[5] = "";
	int_to_char(p_info->pid, pid_str);
	char *codigo = dictionary_get(dictionary_codigos, pid_str);
	linea = get_linea_buscada(codigo, p_info->pc); // HABRÍA QUE DIVIDIR EL CODIGO EN LINEAS AL CREAR ESTRUCTURAS ADMINISTRATIVAS,PERO NO HAY PLATA.
	// HAY QUE VALIDAR Y VER QUE PASA SI SE TRATA DE ACCEDER A UNA LINEA QUE NO CORRESPONDE ()
	log_info(logger, "LINEA LEIDA:%s", linea);
	char **palabras = separar_linea_en_parametros(linea);
	return palabras;
}
int enviar_instruccion(char **palabras, int socket_cliente)
{
	int t0 = strlen(palabras[0]) + 1;
	int t1 = 0;
	int t2 = 0;
	int t3 = 0;
	int t4 = 0;
	int t5 = 0;
	if (palabras[1] != 0) /// esto es feo, pero bueno es lo que hay.
	{
		t1 = strlen(palabras[1]) + 1;
		if (palabras[2] != 0)
		{
			t2 = strlen(palabras[2]) + 1;
			if (palabras[3] != 0)
			{
				t3 = strlen(palabras[3]) + 1;
				if (palabras[4] != 0)
				{
					t4 = strlen(palabras[4]) + 1;
					if (palabras[5] != 0)
						t5 = strlen(palabras[5]) + 1;
				}
			}
		}
	}
	t_paquete *paquete = malloc(sizeof(t_paquete));
	paquete->codigo_operacion = SIGUENTE_INSTRUCCION;
	paquete->buffer = malloc(sizeof(t_buffer));
	paquete->buffer->size = t0 + t1 + t2 + t3 + t4 + t5 + 6 * sizeof(int); //!!!#!"#capaz el 5*sizeof(int) debería ser calculado
	paquete->buffer->stream = malloc(paquete->buffer->size);
	memset(paquete->buffer->stream, 0, paquete->buffer->size);
	paquete->buffer->offset = 0;

	// si alguno de los tams es 0, no se escribe nada en ese parametro osea queda todo 0
	memcpy(paquete->buffer->stream + paquete->buffer->offset, &t0, sizeof(int));
	paquete->buffer->offset += sizeof(int);
	memcpy(paquete->buffer->stream + paquete->buffer->offset, palabras[0], t0);
	paquete->buffer->offset += t0;

	memcpy(paquete->buffer->stream + paquete->buffer->offset, &t1, sizeof(int));
	paquete->buffer->offset += sizeof(int);
	memcpy(paquete->buffer->stream + paquete->buffer->offset, palabras[1], t1);
	paquete->buffer->offset += t1;

	memcpy(paquete->buffer->stream + paquete->buffer->offset, &t2, sizeof(int));
	paquete->buffer->offset += sizeof(int);
	memcpy(paquete->buffer->stream + paquete->buffer->offset, palabras[2], t2);
	paquete->buffer->offset += t2;

	memcpy(paquete->buffer->stream + paquete->buffer->offset, &t3, sizeof(int));
	paquete->buffer->offset += sizeof(int);
	memcpy(paquete->buffer->stream + paquete->buffer->offset, palabras[3], t3);
	paquete->buffer->offset += t3;

	memcpy(paquete->buffer->stream + paquete->buffer->offset, &t4, sizeof(int));
	paquete->buffer->offset += sizeof(int);
	memcpy(paquete->buffer->stream + paquete->buffer->offset, palabras[4], t4);
	paquete->buffer->offset += t4;

	memcpy(paquete->buffer->stream + paquete->buffer->offset, &t5, sizeof(int));
	paquete->buffer->offset += sizeof(int);
	memcpy(paquete->buffer->stream + paquete->buffer->offset, palabras[5], t5);
	paquete->buffer->offset += t5;

	int bytes = paquete->buffer->size + 2 * sizeof(int);

	void *a_enviar = serializar_paquete(paquete, bytes);
	send(socket_cliente, a_enviar, bytes, 0);
	free(a_enviar);
	eliminar_paquete(paquete);
}

fetch_t *recibir_process_info(int socket_cliente)
{
	t_paquete *paquete = malloc(sizeof(t_paquete));
	paquete->buffer = malloc(sizeof(t_buffer));

	recv(socket_cliente, &(paquete->buffer->size), sizeof(int), 0);
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

write_t *recibir_pedido_escritura(int socket_cliente)
{
	t_paquete *paquete = malloc(sizeof(t_paquete));
	paquete->buffer = malloc(sizeof(t_buffer));

	recv(socket_cliente, &(paquete->buffer->size), sizeof(int), 0);
	paquete->buffer->stream = malloc(paquete->buffer->size);
	recv(socket_cliente, paquete->buffer->stream, paquete->buffer->size, 0);

	write_t *solicitud_w = malloc(sizeof(write_t));
	void *stream = paquete->buffer->stream;
	memcpy(&(solicitud_w->dir_fisica), stream, sizeof(int));
	stream += sizeof(int);
	memcpy(&(solicitud_w->tam_escritura), stream, sizeof(int));
	stream += sizeof(int);
	solicitud_w->datos = malloc(solicitud_w->tam_escritura);
	memcpy(solicitud_w->datos, stream, solicitud_w->tam_escritura);

	eliminar_paquete(paquete);

	return solicitud_w;
}
read_t *recibir_pedido_lectura(int socket_cliente)
{
	t_paquete *paquete = malloc(sizeof(t_paquete));
	paquete->buffer = malloc(sizeof(t_buffer));

	recv(socket_cliente, &(paquete->buffer->size), sizeof(int), 0);
	paquete->buffer->stream = malloc(paquete->buffer->size);
	recv(socket_cliente, paquete->buffer->stream, paquete->buffer->size, 0);

	read_t *sol_lectura = malloc(sizeof(read_t));
	void *stream = paquete->buffer->stream;
	memcpy(&(sol_lectura->dir_fisica), stream, sizeof(int));
	stream += sizeof(int);
	memcpy(&(sol_lectura->tam_lectura), stream, sizeof(int));

	eliminar_paquete(paquete);

	return sol_lectura;
}
void enviar_datos_leidos(void *datos_leidos, int tam_datos, int socket_cliente)
{
	t_paquete *paquete = malloc(sizeof(t_paquete));

	paquete->codigo_operacion = READ_MEM_RESPONSE;
	paquete->buffer = malloc(sizeof(t_buffer));
	paquete->buffer->size = sizeof(int) + tam_datos;
	paquete->buffer->stream = malloc(paquete->buffer->size);
	paquete->buffer->offset = 0;

	memcpy(paquete->buffer->stream + paquete->buffer->offset, &tam_datos, sizeof(u_int32_t)); // paso los datos pq si,desde cpu ya se sabe
	paquete->buffer->offset += sizeof(u_int32_t);

	memcpy(paquete->buffer->stream + paquete->buffer->offset, datos_leidos, tam_datos);
	paquete->buffer->offset += tam_datos;

	int bytes = paquete->buffer->size + 2 * sizeof(int);

	void *a_enviar = serializar_paquete(paquete, bytes);

	send(socket_cliente, a_enviar, bytes, 0);
	free(a_enviar);
	eliminar_paquete(paquete);
}
void enviar_status_escritura(int status, int socket_cliente)
{
	t_paquete *paquete = malloc(sizeof(t_paquete));

	paquete->codigo_operacion = WRITE_MEM_RESPONSE;
	paquete->buffer = malloc(sizeof(t_buffer));
	paquete->buffer->size = sizeof(int);
	paquete->buffer->stream = malloc(paquete->buffer->size);
	paquete->buffer->offset = 0;

	memcpy(paquete->buffer->stream + paquete->buffer->offset, &status, sizeof(int)); // paso los datos pq si,desde cpu ya se sabe
	int bytes = paquete->buffer->size + 2 * sizeof(int);

	void *a_enviar = serializar_paquete(paquete, bytes);

	send(socket_cliente, a_enviar, bytes, 0);
	free(a_enviar);
	eliminar_paquete(paquete);
}

solicitud_creacion_t *recibir_solicitud_de_creacion(int socket_cliente)
{
	t_buffer *buffer = malloc(sizeof(t_buffer));
	recv(socket_cliente, &(buffer->size), sizeof(int), 0);
	buffer->stream = malloc(buffer->size);
	recv(socket_cliente, buffer->stream, buffer->size, 0);

	solicitud_creacion_t *estructura = malloc(sizeof(pcb_t));
	void *stream = buffer->stream;
	memcpy(&(estructura->tam), stream, sizeof(int));
	stream += sizeof(int);
	estructura->path = malloc(estructura->tam);
	memcpy((estructura->path), stream, estructura->tam); // este sizeof(int) no debería ser estructura->tam???
	stream += estructura->tam;
	memcpy(&(estructura->pid), stream, sizeof(int)); // REGISTROS_T?????
	free(buffer->stream);
	free(buffer);
	return estructura;
}

int recibir_solicitud_de_eliminacion(int socket_cliente)
{
	int pid_a_eliminar;
	t_buffer *buffer = malloc(sizeof(t_buffer));
	recv(socket_cliente, &(buffer->size), sizeof(int), 0);
	buffer->stream = malloc(buffer->size);
	recv(socket_cliente, buffer->stream, buffer->size, 0);
	void *stream = buffer->stream;

	memcpy(&pid_a_eliminar, stream, sizeof(int));
	free(buffer->stream);
	free(buffer);
	return pid_a_eliminar;
}