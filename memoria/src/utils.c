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
int marcos;
t_bitarray *bitarray;
char *PATH_INSTRUCCIONES;
char *leer_codigo(char *path_relativo) // REVISAR POSIBLES LEAKS DE ESTO
{
	char *path_absoluto = malloc(strlen(PATH_INSTRUCCIONES) + 1); // tecnicamente no es absoluto pero podría serlo
	strcpy(path_absoluto, PATH_INSTRUCCIONES);
	string_append(&path_absoluto, path_relativo);

	// leer el pseudocodigo
	FILE *file;
	char *codigo;
	file = fopen(path_absoluto, "r");
	if (file)
	{
		fseek(file, 0, SEEK_END); // Segmentation fault
		int file_size = ftell(file);
		fseek(file, 0, SEEK_SET);
		codigo = malloc(file_size + 1); // un +1 aca arregla el 💀💀💀, no se porque(capaz tiene q ver con el /0)
		memset(codigo, 0, file_size);

		char linea[100];
		while (fgets(linea, 100, file))
		{
			strcat(codigo, linea);
		}
		printf("%s\n", codigo);
		fclose(file);
		free(path_absoluto); // esto hace free al relativo
		return codigo;
	}
	else
	{
		log_error(logger, "No se pudo encontrar el archivo");
		return "error";
	}
}

int crear_estructuras_administrativas(solicitud_creacion_t *e_admin)
{
	char *codigo = leer_codigo(e_admin->path);
	if (!strcmp(codigo, "error"))
	{
		return -1;
	}
	char *pid_str = string_itoa(e_admin->pid);
	dictionary_put(dictionary_codigos, pid_str, codigo);
	free(pid_str);//se le puede hacer free ahora, pq no se vuelve a usar, el codigo ya quedo en esa key
	return 1;
}
void eliminar_tabla_paginas(int pid_a_eliminar)
{
	bool is_pid(void *elemento)
	{
		return ((elemento_lista_tablas *)elemento)->pid == pid_a_eliminar; // no hay colores pq vscode no se la banca, no es bug
	};
	elemento_lista_tablas *elemento = list_remove_by_condition(lista_tablas_paginas, is_pid);
	t_list_iterator *iterator = list_iterator_create(elemento->tabla);
	log_info(logger, "Eliminacion de Tabla de Páginas ");
	log_info(logger, "Eliminacion de Tabla de Páginas - PID: %i - Tamaño: %i", pid_a_eliminar, list_size(elemento->tabla));

	while (list_iterator_has_next(iterator))
	{
		int *frame = list_iterator_next(iterator);
		bitarray_clean_bit(bitarray, *frame);
		free(frame);
	}
	list_destroy(elemento->tabla); // no hace falta eliminar los elementos de la lista pq son ints
	free(elemento);
}
void crear_tabla_paginas(int pid)
{
	// crear tabla de paginas del pid
	elemento_lista_tablas *elemento = malloc(sizeof(elemento_lista_tablas));
	elemento->pid = pid;
	elemento->tabla = list_create();
	list_add(lista_tablas_paginas, elemento);
	log_info(logger, "Creación de Tabla de Páginas - PID: %i - Tamaño: 0", pid);

	/* 	// prueba:
		list_add(elemento->tabla, 0); // indice = pagina||elem = frame
		list_add(elemento->tabla, 2);
		list_add(elemento->tabla, 1);
		// pags:[yyyx,xxx0,0000]->marcos:[yyyx,0000,xxx0] */
}
void eliminar_estrucuras_administrativas(int pid_a_eliminar)
{
	char *pid_str = string_itoa(pid_a_eliminar);
	sem_t *sem_a_liberar = list_get(sems_espera_creacion_codigos, pid_a_eliminar);
	free(sem_a_liberar);
	char *codigo = dictionary_remove(dictionary_codigos, pid_str); // creo que no hace falta mutex para codigos
	free(codigo);
	free(pid_str);
	log_debug(logger, "Se elimino el codigo del pid %i", pid_a_eliminar);
}
bool hay_paginas_disponibles(int paginas_a_agregar)
{
	int accum = 0;
	for (int i = 0; i < marcos; i++)
	{
		if (!bitarray_test_bit(bitarray, i))
			accum++;
	}
	return accum >= paginas_a_agregar;
}
int take_next_free_frame()
{

	for (int i = 0; i < marcos; i++)
	{
		if (!bitarray_test_bit(bitarray, i))
		{
			bitarray_set_bit(bitarray, i);
			return i;
		}
	}
	return -1;
}
int ampliar_proceso(t_list *tabla, int bytes_a_agregar)
{
	int paginas_a_agregar = bytes_a_agregar / TAM_PAGINA;
	if (bytes_a_agregar % TAM_PAGINA != 0)
	{
		paginas_a_agregar++;
	}
	if (!hay_paginas_disponibles(paginas_a_agregar))
		return -1; // solo un proceso puede estar pidiendo frames a la vez, asi que con la validacion inicial, ya esta
	for (int i = 0; paginas_a_agregar > i; i++)
	{
		int *frame = malloc(sizeof(int));
		*frame = take_next_free_frame(); // marca el frame como en uso.
		list_add(tabla, frame);
	}
	return 0;
}

int reducir_proceso(t_list *tabla, int bytes)
{
	int nuevas_paginas = bytes / TAM_PAGINA;
	if (bytes % TAM_PAGINA != 0)
	{
		nuevas_paginas++;
	}
	log_debug(logger, "Se pidio reducir %i bytes. Nueva cantidad de paginas = %i", bytes, nuevas_paginas);

	for (int i = list_size(tabla) - 1; i >= nuevas_paginas; i--)
	{
		int *frame_a_liberar = list_remove(tabla, i);
		bitarray_clean_bit(bitarray, *frame_a_liberar);
		free(frame_a_liberar);
	}
	return 0;
}

void handle_cpu_client(int socket_cliente)
{
	log_info(logger, "ESTOY EN EL HANDLER DEL CPU");

	bool conexion_terminada = false;
	while (!conexion_terminada)
	{
		int cod_op = recibir_operacion(socket_cliente);
		usleep(RETARDO_RESPUESTA * 1000); // MILISEGUNDOS PEDIDOS POR CONFIG(consigna);
		switch (cod_op)
		{

		case FETCH:
			fetch_t *p_info = recibir_process_info(socket_cliente);
			log_info(logger, "pid:%i", p_info->pid);
			log_info(logger, "pc:%i", p_info->pc);
			sem_t *sem = list_get(sems_espera_creacion_codigos, p_info->pid);
			sem_wait(sem);
			char **palabras = get_siguiente_instruction(p_info, socket_cliente);
			sem_post(sem);
			enviar_instruccion(palabras, socket_cliente);
			break;
		case RESIZE:
			resize_t *solicitud_resize = recibir_pedido_resize(socket_cliente);
			bool is_pid(void *elem)
			{
				return ((elemento_lista_tablas *)elem)->pid == solicitud_resize->pid; // no hay colores pq vscode no se la banca, no es bug
			};
			elemento_lista_tablas *elemento = list_find(lista_tablas_paginas, is_pid);
			int bytes_actuales = list_size(elemento->tabla) * TAM_PAGINA;
			int resize_status = 0;
			if (bytes_actuales > solicitud_resize->bytes)
			{
				reducir_proceso(elemento->tabla, solicitud_resize->bytes);
				log_info(logger, "PID: %i - Tamaño Actual: %i - Tamaño a Reducir: %i", solicitud_resize->pid, bytes_actuales, solicitud_resize->bytes);

			} //!!!!TODO: HACER CALCULO DEL TAMAÑO A...
			if (bytes_actuales < solicitud_resize->bytes)
			{
				// eliminar ultimas paginas
				resize_status = ampliar_proceso(elemento->tabla, solicitud_resize->bytes - bytes_actuales);
				log_info(logger, "PID: %i - Tamaño Actual: %i - Tamaño a Ampliar: %i", solicitud_resize->pid, bytes_actuales, solicitud_resize->bytes);
			}
			devolver_status_resize(resize_status, socket_cliente);
			free(solicitud_resize);
			break;
		case GET_FRAME:
			get_frame_t *solicitud_f = recibir_pedido_frame(socket_cliente);
			int *frame = calcular_frame(solicitud_f);
			enviar_frame(*frame, socket_cliente);

			// número_página = floor(dirección_lógica / tamaño_página)
			// desplazamiento = dirección_lógica - número_página * tamaño_página

			// se recibe pid,npagina. se devuelve nframe.
			//(si se quisiera pedir varias paginas(por dato largo), se pide de auna)
			break;
		case READ_MEM:
			read_t *solicitud_r = recibir_pedido_lectura(socket_cliente);
			void *datos_leidos = leer_memoria(solicitud_r->dir_fisica, solicitud_r->tam_lectura);
			int a_loggear = 0;
			enviar_datos_leidos(datos_leidos, solicitud_r->tam_lectura, socket_cliente);
			log_info(logger, "PID: %i - Accion: LEER - Direccion fisica: %i - Tamaño %i", solicitud_r->pid, solicitud_r->dir_fisica, solicitud_r->tam_lectura);
			break; // no se recibe pid en read/write(creo)
		case WRITE_MEM:
			write_t *solicitud_w = recibir_pedido_escritura(socket_cliente);
			int write_status = escribir_memoria(solicitud_w->datos, solicitud_w->dir_fisica, solicitud_w->tam_escritura);
			enviar_status_escritura(write_status, socket_cliente);
			log_info(logger, "PID: %i - Accion: ESCRIBIR - Direccion fisica: %i - Tamaño %i", solicitud_w->pid, solicitud_w->dir_fisica, solicitud_w->tam_escritura);

			break;
		case -1:
			return -1;
		default:
			break;
		}
	}
}

resize_t *recibir_pedido_resize(int socket_cliente)
{
	t_paquete *paquete = malloc(sizeof(t_paquete));
	paquete->buffer = malloc(sizeof(t_buffer));

	recv(socket_cliente, &(paquete->buffer->size), sizeof(int), 0);
	paquete->buffer->stream = malloc(paquete->buffer->size);
	recv(socket_cliente, paquete->buffer->stream, paquete->buffer->size, 0);

	resize_t *sol = malloc(sizeof(resize_t));
	void *stream = paquete->buffer->stream;
	memcpy(&(sol->pid), stream, sizeof(int));
	stream += sizeof(int);
	memcpy(&(sol->bytes), stream, sizeof(int));

	eliminar_paquete(paquete);

	return sol;
}
void devolver_status_resize(int status, int socket_cliente)
{
	t_paquete *paquete = malloc(sizeof(t_paquete));

	paquete->codigo_operacion = RESIZE_RESPONSE;
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
int escribir_memoria(void *datos, u_int32_t dir_fisica, int tam_escritura)
{
	int status = MEM_W_OK; // creo q hay q validar si escribe dentro del espacio de usario para devolver no_ok
	memcpy(espacio_usuario + dir_fisica, datos, tam_escritura);
	return status;
}

void *leer_memoria(u_int32_t dir_fisica, int tam_lectura)
{
	void *datos_leidos = malloc(tam_lectura);
	memcpy(datos_leidos, espacio_usuario + dir_fisica, tam_lectura);
	return datos_leidos;
}
void handler_kernel_client(int socket)
{
	bool conexion_terminada = false;
	while (!conexion_terminada)
	{
		int cod_op = recibir_operacion(socket);
		switch (cod_op)
		{
		case CREAR_ESTRUC_ADMIN:
			solicitud_creacion_t *e_admin = recibir_solicitud_de_creacion(socket);
			sem_t *sem = malloc(sizeof(sem_t));
			sem_init(sem, 0, 0);
			list_add_in_index(sems_espera_creacion_codigos, e_admin->pid, sem); // los elementos nunca se borran, pq si hago remove muevo los demas(creo), solo se hace free del sem al eliminar_e_admin.
			log_info(logger, "path:%s", e_admin->path); // Ultimo log antes del error del so-deploy
			int err = crear_estructuras_administrativas(e_admin);
			int status = 0;
			if (err == -1)
			{
				status = ERROR;
				send(socket, &status, sizeof(int), 0);
			}
			else
			{
				status = CREACION_EXITOSA;
				send(socket, &status, sizeof(int), 0);
				crear_tabla_paginas(e_admin->pid);
				sem_post(sem);
			}
			free(e_admin); // el free de .path se hace en leer codigo
			break;
		case ELIMINAR_ESTRUC_ADMIN:
			int pid_a_eliminar = recibir_solicitud_de_eliminacion(socket);
			eliminar_estrucuras_administrativas(pid_a_eliminar);
			eliminar_tabla_paginas(pid_a_eliminar);
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
		handler_kernel_client(socket_cliente);
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
		handle_cpu_client(socket_cliente); // debería ser uno separado q solo acepte write y read

		break;

	default:
		log_warning(logger, "Cliente desconocido por memoria server.");
		return;
	}
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
	log_info(logger, "Me llego la operacion crear_pcb, el pid es: %i", buffer);
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
	char *lines[1000];
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
	char *pid_str = string_itoa(p_info->pid);
	char *codigo = dictionary_get(dictionary_codigos, pid_str);
	linea = get_linea_buscada(codigo, p_info->pc); // HABRÍA QUE DIVIDIR EL CODIGO EN LINEAS AL CREAR ESTRUCTURAS ADMINISTRATIVAS,PERO NO HAY PLATA.
	// HAY QUE VALIDAR Y VER QUE PASA SI SE TRATA DE ACCEDER A UNA LINEA QUE NO CORRESPONDE ()
	log_info(logger, "LINEA LEIDA:%s", linea);
	char **palabras = separar_linea_en_parametros(linea);
	free(pid_str);
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

	fetch_t *p_info = malloc(sizeof(fetch_t)); //Free?
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
	memcpy(&(solicitud_w->pid), stream, sizeof(int));
	stream += sizeof(int);
	solicitud_w->datos = malloc(solicitud_w->tam_escritura);
	memcpy(solicitud_w->datos, stream, solicitud_w->tam_escritura);

	eliminar_paquete(paquete);

	return solicitud_w;
}
get_frame_t *recibir_pedido_frame(socket_cliente)
{
	t_paquete *paquete = malloc(sizeof(t_paquete));
	paquete->buffer = malloc(sizeof(t_buffer));

	recv(socket_cliente, &(paquete->buffer->size), sizeof(int), 0);
	paquete->buffer->stream = malloc(paquete->buffer->size);
	recv(socket_cliente, paquete->buffer->stream, paquete->buffer->size, 0);

	get_frame_t *sol_frame = malloc(sizeof(get_frame_t));
	void *stream = paquete->buffer->stream;
	memcpy(&(sol_frame->pid), stream, sizeof(int));
	stream += sizeof(int);
	memcpy(&(sol_frame->pagina), stream, sizeof(int));

	eliminar_paquete(paquete);

	return sol_frame;
}
int *calcular_frame(get_frame_t *solicitud)
{
	bool is_pid(void *elemento)
	{
		return ((elemento_lista_tablas *)elemento)->pid == solicitud->pid; // no hay colores pq vscode no se la banca, no es bug
	};
	t_list *tabla = ((elemento_lista_tablas *)list_find(lista_tablas_paginas, is_pid))->tabla;

	int *frame = list_get(tabla, solicitud->pagina);
	log_info(logger, "PID: %i - Pagina:%i - Marco: %i", solicitud->pid, solicitud->pagina, *frame);
	return frame;
}
void enviar_frame(int frame, int socket_cliente)
{
	// devuelve el numero de frame asignado a la pagina del pid
	//(frame== int, la dir fisica del frame es frame*tam_pag)
	t_paquete *paquete = malloc(sizeof(t_paquete));

	paquete->codigo_operacion = GET_FRAME_RESPONSE;
	paquete->buffer = malloc(sizeof(t_buffer));
	paquete->buffer->size = sizeof(int);
	paquete->buffer->stream = malloc(paquete->buffer->size);
	paquete->buffer->offset = 0;

	memcpy(paquete->buffer->stream + paquete->buffer->offset, &frame, sizeof(int)); // paso los datos pq si,desde cpu ya se sabe

	int bytes = paquete->buffer->size + 2 * sizeof(int);

	void *a_enviar = serializar_paquete(paquete, bytes);

	send(socket_cliente, a_enviar, bytes, 0);
	free(a_enviar);
	eliminar_paquete(paquete);
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
	stream += sizeof(int);
	memcpy(&(sol_lectura->pid), stream, sizeof(int));

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

	solicitud_creacion_t *estructura = malloc(sizeof(solicitud_creacion_t)); //puede ser este el error?
	void *stream = buffer->stream;
	memcpy(&(estructura->tam), stream, sizeof(int));
	stream += sizeof(int);
	estructura->path = malloc(estructura->tam); //Free
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
