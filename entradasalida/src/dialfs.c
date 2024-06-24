#include "client.h"
#include <readline/readline.h>
#include <semaphore.h>
int BLOCK_SIZE;
int BLOCK_COUNT;
int tam_blocks_file;
int tam_bitmap_file;
char *PATH_BASE_DIALFS;
char *bloques_path;
void *mmbitmap;
t_bitarray *bitmap;

void iniciar_interfaz_dialfs()
{

	PATH_BASE_DIALFS = config_get_string_value(config, "PATH_BASE_DIALFS");
	BLOCK_SIZE = config_get_int_value(config, "BLOCK_SIZE");
	BLOCK_COUNT = config_get_int_value(config, "BLOCK_COUNT");
	tam_blocks_file = BLOCK_SIZE * BLOCK_COUNT;
	tam_bitmap_file = BLOCK_COUNT / 8;
	if (BLOCK_COUNT % 8 != 0)
		tam_bitmap_file++;

	inicializar_bloques(); // abre/crea el file y lo mmapea para poder usarlo con memcpy
	inicializar_bitmap();  // abre/crea el file y lo mmapea para poder usarlo con memcpy
	inicializar_cliente_kernel();
	inicializar_cliente_memoria();

	t_interfaz *nueva_interfaz = crear_estrcutura_io(DIALFS);
	enviar_interfaz(CREACION_IO, *nueva_interfaz, socket_kernel);
	while (1) // xd
	{
		io_task *pedido = recibir_peticion();
		int operacion = decode_operation(pedido->buffer_instruccion);//no es un 'recibir_operacion'. Esto no lo saca del buffer
		handle_operations(operacion, pedido);

		informar_fin_de_tarea(socket_kernel, IO_OK, pedido->pid_solicitante);
	}
}
handle_operations(int operacion, io_task *pedido)
{
	buffer_instr_io_t *buffer_instruccion = pedido->buffer_instruccion;
	switch (operacion)
	{
	case IO_FS_CREATE:
		char *nombre_c = decode_buffer_file_name(buffer_instruccion);
		crear_archivo(nombre_c);
		log_info(logger, "PID: %i - Operacion: IO_FS_CREATE", pedido->pid_solicitante);
		break;
	case IO_FS_DELETE:
		char *nombre_d = decode_buffer_file_name(buffer_instruccion);
		eliminar_archivo(nombre_d);
		log_info(logger, "PID: %i - Operacion: IO_FS_DELETE", pedido->pid_solicitante);
		break;
	case IO_FS_TRUNCATE:
		truncate_t *sol_truncate = decode_buffer_truncate_sol(buffer_instruccion);
		truncar_archivo(sol_truncate);
		log_info(logger, "PID: %i - Operacion: IO_FS_TRUNCATE", pedido->pid_solicitante);
		break;
	case IO_FS_WRITE:
		fs_rw_sol_t *sol_write = decode_buffer_rw_sol(buffer_instruccion);
		void *datos_w = malloc(sol_write->max_tam);
		memset(datos_w, 0, sol_write->max_tam);
		leer_memoria(sol_write->solicitudes, datos_w);
		log_debug(logger, "datos_w leidos desde memoria: %s", (char *)datos_w);
		escribir_archivo(datos_w, sol_write->nombre, sol_write->puntero_archivo, sol_write->max_tam);
		log_info(logger, "PID: %i - Operacion: IO_FS_WRITE", pedido->pid_solicitante);
		free(datos_w);
		free(sol_write->nombre);
		free(sol_write);
		break;
	case IO_FS_READ:
		fs_rw_sol_t *sol_read = decode_buffer_rw_sol(buffer_instruccion);
		void *datos_r = leer_archivo(sol_read->nombre,sol_read->puntero_archivo,sol_read->max_tam);
		log_debug(logger, "datos_r leidos desde fs: %s", (char *)datos_r);
		populate_solicitudes(sol_read->solicitudes, datos_r);
		escribir_memoria(sol_read->solicitudes);
		log_info(logger, "PID: %i - Operacion: IO_FS_READ", pedido->pid_solicitante);
		free(datos_r);
		free(sol_read->nombre);
		free(sol_read);
		break;
	}
	free(pedido->buffer_instruccion);
	free(pedido);
}

void *leer_archivo(char *nombre, int puntero, int tam)
{
	void *datos = malloc(tam);
	int puntero_base = get_puntero_base(nombre);
	FILE *bloques = fopen(bloques_path, "r");
	fseek(bloques, puntero_base + puntero, SEEK_SET);
	fread(datos,tam,1,bloques);
	fclose(bloques);
	return datos;
}

fs_rw_sol_t *decode_buffer_rw_sol(buffer_instr_io_t *buffer_instruccion) // max tam = suma de tam de cada sol
{
	void *buffer = buffer_instruccion->buffer;
	fs_rw_sol_t *sol_write = malloc(sizeof(fs_rw_sol_t));
	memset(sol_write, 0, sizeof(fs_rw_sol_t));
	sol_write->solicitudes = list_create(); // solicitud_unitaria_t *
	int offset = sizeof(u_int32_t);			// pq sigue estando el op

	memcpy(&(sol_write->puntero_archivo), buffer + offset, sizeof(u_int32_t));
	offset += sizeof(u_int32_t);
	int tam_nombre = 0;
	memcpy(&tam_nombre, buffer + offset, sizeof(u_int32_t));
	offset += sizeof(u_int32_t);
	sol_write->nombre = malloc(tam_nombre + 1); //+1 /0
	memset(sol_write->nombre, 0, tam_nombre + 1);
	memcpy(sol_write->nombre, buffer + offset, tam_nombre);
	offset += tam_nombre;

	while (offset < buffer_instruccion->size)
	{
		solicitud_unitaria_t *sol = malloc(sizeof(solicitud_unitaria_t));
		memset(sol, 0, sizeof(solicitud_unitaria_t));
		memcpy(&(sol->dir_fisica_base), buffer + offset, sizeof(u_int32_t));
		offset += sizeof(u_int32_t);
		memcpy(&(sol->offset), buffer + offset, sizeof(u_int32_t));
		offset += sizeof(u_int32_t);
		memcpy(&(sol->tam), buffer + offset, sizeof(u_int32_t));
		offset += sizeof(u_int32_t);
		memcpy(&(sol->pid), buffer + offset, sizeof(u_int32_t));
		offset += sizeof(u_int32_t);
		sol_write->max_tam += sol->tam;
		list_add(sol_write->solicitudes, sol);
	}
	return sol_write;
}
void escribir_archivo(void *datos, char *nombre, int puntero, int tam)
{
	int puntero_base = get_puntero_base(nombre);
	FILE *bloques = fopen(bloques_path, "r+");
	fseek(bloques, puntero_base + puntero, SEEK_SET);
	fwrite(datos, tam, 1, bloques);
	fclose(bloques);
	// free datos;
}
int get_puntero_base(char *nombre)
{
	char *path = get_complete_path(nombre);
	t_config *metadata = config_create(path);
	int bloque_inicial = config_get_int_value(metadata, "BLOQUE_INICIAL");
	config_destroy(metadata);
	return bloque_inicial * BLOCK_SIZE;
}
void truncar_archivo(truncate_t *sol)
{
	int bytes_actuales = get_actual_bytes(sol->file);
	int nuevos_bytes = sol->bytes;
	int bloques_actuales = ceil(((double)bytes_actuales) / BLOCK_SIZE);
	int nuevos_bloques = ceil(((double)nuevos_bytes) / BLOCK_SIZE);
	bloques_actuales = bloques_actuales ? bloques_actuales : 1;
	nuevos_bloques = nuevos_bloques ? nuevos_bloques : 1; // un resize 0 no debe eliminar el bloque inicial
	if (nuevos_bloques - bloques_actuales > 0)			  // ampliar
	{
		int bloques_a_agregar = nuevos_bloques - bloques_actuales;
		if (!es_posible_agrandar(sol->file, bloques_actuales, bloques_a_agregar))
		{
			// todo COMPACTAR
		}
		agrandar_archivo(sol->file, bloques_actuales, bloques_a_agregar, nuevos_bytes);
	}
	if (nuevos_bloques - bloques_actuales < 0) // reducir
	{
		int bloques_a_reducir = bloques_actuales - nuevos_bloques;
		reducir_archivo(sol->file, bloques_actuales, bloques_a_reducir, nuevos_bytes);
	}
	// ante cualquiera de los 3 casos:
	actualizar_tamanio(sol->file, nuevos_bytes);
}

void actualizar_tamanio(char *nombre, int nuevos_bytes)
{
	char *path = get_complete_path(nombre);
	t_config *metadata = config_create(path);
	char *s = string_itoa(nuevos_bytes);
	config_set_value(metadata, "TAMANIO_ARCHIVO", s);
	free(s); // una paja tener q hacer esto
	config_save(metadata);
	config_destroy(metadata);
}

void agrandar_archivo(char *nombre, int bloques_actuales, int bloques_a_agregar, int nuevos_bytes)
{
	char *path = get_complete_path(nombre);
	t_config *metadata = config_create(path);
	int bloque_inicial = config_get_int_value(metadata, "BLOQUE_INICIAL");
	int inicio_ampliacion = bloque_inicial + bloques_actuales;
	for (int i = inicio_ampliacion; i < inicio_ampliacion + bloques_a_agregar; i++)
	{ // no hace falta validar que se vaya del bitmap pq eso ya se hizo en 'es_posible_agrandar'
		ocupar_bloque(i);
	}
	config_destroy(metadata);
}

void reducir_archivo(char *nombre, int bloques_actuales, int bloques_a_reducir, int nuevos_bytes)
{
	char *path = get_complete_path(nombre);
	t_config *metadata = config_create(path);
	int bloque_inicial = config_get_int_value(metadata, "BLOQUE_INICIAL");		  // 1100000
	int inicio_reduccion = bloque_inicial + bloques_actuales - 1;				  // i_r=3
	for (int i = inicio_reduccion; i > inicio_reduccion - bloques_a_reducir; i--) // i_r-b_r=3-2=1
	{
		desocupar_bloque(i);
	}
	config_destroy(metadata);
}
bool es_posible_agrandar(char *nombre, int bloques_actuales, int bloques_a_agregar)
{
	char *path = get_complete_path(nombre);
	t_config *metadata = config_create(path);
	int bloque_inicial = config_get_int_value(metadata, "BLOQUE_INICIAL");
	int inicio_ampliacion = bloque_inicial + bloques_actuales;
	for (int i = inicio_ampliacion; i < inicio_ampliacion + bloques_a_agregar; i++)
	{
		if (i >= BLOCK_COUNT) // fin pracito de bitmap
			return false;
		if (bitarray_test_bit(bitmap, i) == 1) // si no hay espacio continuo para agrandar, no se puede.
			return false;
	}
	config_destroy(metadata);
	return true;
}

int get_actual_bytes(char *nombre)
{
	char *path = get_complete_path(nombre);
	t_config *metadata = config_create(path);
	int tam = config_get_int_value(metadata, "TAMANIO_ARCHIVO");
	config_destroy(metadata);
	return tam;
}
truncate_t *decode_buffer_truncate_sol(buffer_instr_io_t *buffer_instruccion)
{
	truncate_t *sol = malloc(sizeof(truncate_t));
	void *buffer = buffer_instruccion->buffer;
	int offset = sizeof(u_int32_t); // pq sigue teniendo el op
	int tam_nombre = 0;
	memcpy(&tam_nombre, buffer + offset, sizeof(u_int32_t));
	offset += sizeof(u_int32_t);
	sol->file = malloc(tam_nombre + 1);
	memset(sol->file, 0, tam_nombre + 1);
	memcpy(sol->file, buffer + offset, tam_nombre);
	offset += tam_nombre;
	memcpy(&(sol->bytes), buffer + offset, sizeof(u_int32_t));

	return sol;
}
char *decode_buffer_file_name(buffer_instr_io_t *buffer_instruccion)
{
	void *buffer = buffer_instruccion->buffer;
	int offset = sizeof(u_int32_t); // pq sigue teniendo el op
	int tam_nombre = 0;
	memcpy(&tam_nombre, buffer + offset, sizeof(u_int32_t));
	offset += sizeof(u_int32_t);
	char *nombre = malloc(tam_nombre + 1);
	memset(nombre, 0, tam_nombre + 1);
	memcpy(nombre, buffer + offset, tam_nombre);
	return nombre;
}
int decode_operation(buffer_instr_io_t *buffer_instruccion)
{
	int op = 0;
	memcpy(&op, buffer_instruccion->buffer, sizeof(int));
	return op;
}
void crear_archivo(char *nombre)
{
	int bloque = get_next_free_block();
	ocupar_bloque(bloque);
	crear_metadata(nombre, bloque);
}
void desocupar_bloque(int index)
{
	bitarray_clean_bit(bitmap, index);
	msync(mmbitmap, tam_bitmap_file, MS_SYNC);
}
void ocupar_bloque(int index)
{
	bitarray_set_bit(bitmap, index);
	msync(mmbitmap, tam_bitmap_file, MS_SYNC);
}
void liberar_bloque(int index)
{
	bitarray_clean_bit(bitmap, index);
	msync(mmbitmap, tam_bitmap_file, MS_SYNC);
}
int get_next_free_block()
{

	for (int i = 0; i < BLOCK_COUNT; i++)
	{
		if (!bitarray_test_bit(bitmap, i))
		{
			return i;
		}
	}
	return -1;
}
void crear_metadata(char *nombre, int bloque)
{
	char *path = get_complete_path(nombre);

	FILE *file = fopen(path, "w+"); // crear file
	fclose(file);
	t_config *metadata = config_create(path);
	char *s = string_itoa(bloque);
	config_set_value(metadata, "BLOQUE_INICIAL", s);
	free(s);
	config_set_value(metadata, "TAMANIO_ARCHIVO", "0");
	config_save(metadata);
	config_destroy(metadata);
	free(path); // este free incluye a nombre
}

void eliminar_archivo(char *nombre)
{
	char *path = get_complete_path(nombre);
	t_config *metadata = config_create(path);
	if (metadata == 0)
		return;
	int bloque_inicial = config_get_int_value(metadata, "BLOQUE_INICIAL");
	int tam = config_get_int_value(metadata, "TAMANIO_ARCHIVO");
	int bloques_a_liberar = ceil(((double)tam) / BLOCK_SIZE);
	bloques_a_liberar = bloques_a_liberar ? bloques_a_liberar : 1; // estoespq siempre se toma un bloque inicialmente
	liberar_bloques_desde(bloque_inicial, bloques_a_liberar);
	config_destroy(metadata);
	remove(path);
	free(path);
}
void liberar_bloques_desde(int bloque_inicial, int bloques_a_liberar)
{
	for (int i = bloque_inicial; i < bloque_inicial + bloques_a_liberar; i++)
	{
		liberar_bloque(i);
	}
}
char *get_complete_path(char *nombre)
{
	char *path = malloc(strlen(PATH_BASE_DIALFS));
	strcpy(path, PATH_BASE_DIALFS);
	string_append(&path, nombre);
	return path;
}
void inicializar_bitmap()
{
	char *nombre = "bitmap.dat";
	char *path = get_complete_path(nombre);

	FILE *file;
	int file_fd;

	file = fopen(path, "r+");
	if (file == NULL) // sino existe, se crea
	{
		file = fopen(path, "w+");
		file_fd = fileno(file);
		if (ftruncate(file_fd, tam_bitmap_file) != 0)
		{
			perror("Error truncating file");
			return EXIT_FAILURE;
		}
	}
	file_fd = fileno(file);
	mmbitmap = mmap(NULL, tam_bitmap_file, PROT_READ | PROT_WRITE, MAP_SHARED, file_fd, 0);
	bitmap = bitarray_create_with_mode(mmbitmap, tam_bitmap_file, MSB_FIRST); // SI ES MSB, ES MAS FACIL LEERLO EN EL HEX EDITOR. EN MEMORIA NO VI QUE ESTO IMPORTARA

	////NO SE DONDE HACER munmap y fclose pq el proceso se cierra con ctrl c
	fclose(file);
	free(path);
}
void inicializar_bloques()
{
	char *nombre = "bloques.dat";
	bloques_path = get_complete_path(nombre);
	FILE *file;
	int file_fd;

	file = fopen(bloques_path, "r+");
	if (file == NULL) // sino existe, se crea
	{
		file = fopen(bloques_path, "w+");
		file_fd = fileno(file);
		if (ftruncate(file_fd, tam_blocks_file) != 0)
		{
			perror("Error truncating file");
			return EXIT_FAILURE;
		}
	}
	fclose(file);
}