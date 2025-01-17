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
	RETRASO_COMPACTACION = config_get_int_value(config, "RETRASO_COMPACTACION");
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
		int operacion = decode_operation(pedido->buffer_instruccion); // no es un 'recibir_operacion'. Esto no lo saca del buffer
		handle_operations(operacion, pedido);
		informar_fin_de_tarea(socket_kernel, IO_OK, pedido->pid_solicitante);
		free(pedido->buffer_instruccion->buffer);
		free(pedido->buffer_instruccion);
		free(pedido);
	}
}
handle_operations(int operacion, io_task *pedido)
{
	buffer_instr_io_t *buffer_instruccion = pedido->buffer_instruccion;
	switch (operacion)
	{
	case IO_FS_CREATE:
		log_info(logger, "PID: %i - Operacion: IO_FS_CREATE", pedido->pid_solicitante);
		char *nombre_c = decode_buffer_file_name(buffer_instruccion);
		crear_archivo(nombre_c);
		log_info(logger, "PID: %i - Crear Archivo: %s", pedido->pid_solicitante, nombre_c);
		free(nombre_c);
		break;
	case IO_FS_DELETE:
		log_info(logger, "PID: %i - Operacion: IO_FS_DELETE", pedido->pid_solicitante);
		char *nombre_d = decode_buffer_file_name(buffer_instruccion);
		eliminar_archivo(nombre_d);
		log_info(logger, "PID: %i - Eliminar Archivo: %s", pedido->pid_solicitante, nombre_d);
		free(nombre_d);
		break;
	case IO_FS_TRUNCATE:
		log_info(logger, "PID: %i - Operacion: IO_FS_TRUNCATE", pedido->pid_solicitante);
		truncate_t *sol_truncate = decode_buffer_truncate_sol(buffer_instruccion);
		truncar_archivo(sol_truncate, pedido->pid_solicitante);
		log_info(logger, "PID: %i - Truncar Archivo: %s - Tamaño: %i", pedido->pid_solicitante, sol_truncate->file, sol_truncate->bytes);
		free(sol_truncate->file);
		free(sol_truncate);
		break;
	case IO_FS_WRITE:
		log_info(logger, "PID: %i - Operacion: IO_FS_WRITE", pedido->pid_solicitante);
		fs_rw_sol_t *sol_write = decode_buffer_rw_sol(buffer_instruccion);
		void *datos_w = malloc(sol_write->max_tam);
		memset(datos_w, 0, sol_write->max_tam);
		leer_memoria(sol_write->solicitudes, datos_w);
		// log_debug(logger, "datos_w leidos desde memoria: %s", (char *)datos_w);
		escribir_archivo(datos_w, sol_write->nombre, sol_write->puntero_archivo, sol_write->max_tam);
		log_info(logger, "PID: %i - Escribir Archivo: %s - Tamaño a Escribir: %i - Puntero Archivo: %i", pedido->pid_solicitante, sol_write->nombre, sol_write->max_tam, sol_write->puntero_archivo);
		liberar_y_eliminar_solicitudes(sol_write->solicitudes);
		free(datos_w);
		free(sol_write->nombre);
		free(sol_write);
		break;
	case IO_FS_READ:
		log_info(logger, "PID: %i - Operacion: IO_FS_READ", pedido->pid_solicitante);
		fs_rw_sol_t *sol_read = decode_buffer_rw_sol(buffer_instruccion);
		void *datos_r = leer_archivo(sol_read->nombre, sol_read->puntero_archivo, sol_read->max_tam);
		log_debug(logger, "datos_r leidos desde fs: %s", (char *)datos_r);
		populate_solicitudes(sol_read->solicitudes, datos_r);
		escribir_memoria(sol_read->solicitudes);
		log_info(logger, "PID: %i - Leer Archivo: %s - Tamaño a Leer: %i - Puntero Archivo: %i", pedido->pid_solicitante, sol_read->nombre, sol_read->max_tam, sol_read->puntero_archivo);
		liberar_y_eliminar_solicitudes(sol_read->solicitudes);
		free(datos_r);
		free(sol_read->nombre);
		free(sol_read);
		break;
	}
}

void *leer_archivo(char *nombre, int puntero, int tam)
{
	void *datos = malloc(tam);
	int puntero_base = get_puntero_base(nombre);
	FILE *bloques = fopen(bloques_path, "r");
	fseek(bloques, puntero_base + puntero, SEEK_SET);
	fread(datos, tam, 1, bloques);
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
	free(path);
	return bloque_inicial * BLOCK_SIZE;
}
void truncar_archivo(truncate_t *sol, int pid_solicitante)
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
			log_info(logger, "PID: %i - Inicio Compactación.", pid_solicitante);
			compactar(sol->file);
			usleep(RETRASO_COMPACTACION * 1000);
			log_info(logger, "PID: %i - Fin Compactación.", pid_solicitante);
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
// 111233344050//se quiere agregar otro bloque al f2.
// 111033344050
// 111333044050
// 111333440050
// 111333445000
// 111333445220
void compactar(char *nombre)
{

	int bloques_file = liberar_bloques(nombre);
	t_dictionary *files_actuales = obtener_lista_files("AD"); // esto excluye a 'nombre'.El indice es el bloque inicial, el contenido es el nombre.
	void *buffer_file = leer_archivo(nombre, 0, bloques_file * BLOCK_SIZE);

	int gap_start = get_next_free_block(0);
	int next_busy_block = get_next_busy_block(gap_start);
	while (next_busy_block != -1) // si es -1, quiere decir que ya se llego al espacio continuo en disco
	{
		char *n_b_b_str = string_itoa(next_busy_block);
		dezplazar_file(gap_start, dictionary_get(files_actuales, n_b_b_str)); //  mueve hacia la izq al file que empiece en ese bloque
		gap_start = get_next_free_block(gap_start);
		next_busy_block = get_next_busy_block(gap_start);
		free(n_b_b_str);
	}
	actualizar_metadata(nombre, gap_start);
	escribir_archivo(buffer_file, nombre, 0, bloques_file * BLOCK_SIZE);
	ocupar_bloques(gap_start, bloques_file);

	free(buffer_file);
	dictionary_iterator(files_actuales, free_file_name);//REVISAR PORQUE ESTO NO HACE FREE DEL NOMBRE
	dictionary_destroy(files_actuales);
}
void *free_file_name(char *key, void *file_name)
{
	log_warning(logger,"filename:%s",file_name);
	free(file_name);
}
void dezplazar_file(int nuevo_inicio, char *nombre)
{
	int bloques_file = liberar_bloques(nombre);
	void *buffer_file = leer_archivo(nombre, 0, bloques_file * BLOCK_SIZE);
	actualizar_metadata(nombre, nuevo_inicio);
	escribir_archivo(buffer_file, nombre, 0, bloques_file * BLOCK_SIZE);
	ocupar_bloques(nuevo_inicio, bloques_file);
	free(buffer_file);
}
void actualizar_metadata(char *nombre, int nuevo_inicio)
{
	char *path = get_complete_path(nombre);
	t_config *metadata = config_create(path);
	char *s = string_itoa(nuevo_inicio);
	config_set_value(metadata, "BLOQUE_INICIAL", s);
	free(s);
	config_save(metadata);
	config_destroy(metadata);
	free(path); // este free incluye a nombre
}

t_dictionary *obtener_lista_files(char *a_excluir)
{
	t_dictionary *files = dictionary_create();
	DIR *dir = opendir("./estructuras_fs");
	struct dirent *entry;
	while ((entry = readdir(dir)) != NULL)
	{
		char *nombre = strdup(entry->d_name);

		if (strcmp(nombre, a_excluir) != 0 && strcmp(nombre, "bitmap.dat") != 0 && strcmp(nombre, "bloques.dat") != 0 && strcmp(nombre, ".") != 0 && strcmp(nombre, "..") != 0)
		{
			char *path = get_complete_path(nombre);
			t_config *metadata = config_create(path);
			int bloque_inicial = config_get_int_value(metadata, "BLOQUE_INICIAL");
			char *b_i_str = string_itoa(bloque_inicial);
			dictionary_put(files, b_i_str, nombre);
			free(b_i_str);
			free(path);
			config_destroy(metadata);
		}
	}
	closedir(dir);
	return files;
}
void *get_file_buffer(char *nombre)
{
}
void actualizar_tamanio(char *nombre, int nuevos_bytes)
{
	char *path = get_complete_path(nombre);
	t_config *metadata = config_create(path);
	char *s = string_itoa(nuevos_bytes);
	config_set_value(metadata, "TAMANIO_ARCHIVO", s);
	free(path);
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
	free(path);
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
		{
			config_destroy(metadata);
			free(path);
			return false;
		}
		if (bitarray_test_bit(bitmap, i) == 1) // si no hay espacio continuo para agrandar, no se puede.
		{
			config_destroy(metadata);
			free(path);
			return false;
		}
	}

	return true;
}

int get_actual_bytes(char *nombre)
{
	char *path = get_complete_path(nombre);
	t_config *metadata = config_create(path);
	int tam = config_get_int_value(metadata, "TAMANIO_ARCHIVO");
	config_destroy(metadata);
	free(path);
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
	int bloque = get_next_free_block(0);
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

int get_next_free_block(int inicio_busqueda)
{

	for (int i = inicio_busqueda; i < BLOCK_COUNT; i++)
	{
		if (!bitarray_test_bit(bitmap, i))
		{
			return i;
		}
	}
	return -1;
}

int get_next_busy_block(int inicio_busqueda)
{

	for (int i = inicio_busqueda; i < BLOCK_COUNT; i++)
	{
		if (bitarray_test_bit(bitmap, i))
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
	liberar_bloques(nombre);
	config_destroy(metadata);
	remove(path);
	free(path);
}
int liberar_bloques(char *nombre)
{
	char *path = get_complete_path(nombre);
	t_config *metadata = config_create(path);
	if (metadata == 0)
		return;
	int bloque_inicial = config_get_int_value(metadata, "BLOQUE_INICIAL");
	int tam = config_get_int_value(metadata, "TAMANIO_ARCHIVO");
	int bloques_a_liberar = ceil(((double)tam) / BLOCK_SIZE);
	bloques_a_liberar = bloques_a_liberar ? bloques_a_liberar : 1; // estoespq siempre se toma un bloque inicialmente

	for (int i = bloque_inicial; i < bloque_inicial + bloques_a_liberar; i++)
	{
		desocupar_bloque(i);
	}
	free(path);
	config_destroy(metadata);
	return bloques_a_liberar;
}

int ocupar_bloques(int inicio, int cant_bloques)
{

	for (int i = inicio; i < inicio + cant_bloques; i++)
	{
		ocupar_bloque(i);
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
