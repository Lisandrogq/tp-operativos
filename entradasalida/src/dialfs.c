#include "client.h"
#include <readline/readline.h>
#include <semaphore.h>
int BLOCK_SIZE;
int BLOCK_COUNT;
int tam_blocks_file;
int tam_bitmap_file;
char *PATH_BASE_DIALFS;
void *bloques;
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

/* 	 crear_archivo("persistira1.t");
	 crear_archivo("aborrar.t");
	 crear_archivo("persistira2.t"); */
	eliminar_archivo("t1");
	eliminar_archivo("t2");
	eliminar_archivo("t3");
	return;
	inicializar_cliente_kernel();
	inicializar_cliente_memoria();
	t_interfaz *nueva_interfaz = crear_estrcutura_io(DIALFS);
	enviar_interfaz(CREACION_IO, *nueva_interfaz, socket_kernel);
	while (1) // xd
	{
		io_task *pedido = recibir_peticion();
		// decode operation
		// switch(operacion)
		// decode_create_file...
		log_info(logger, "PID: %i - Operacion: xxxxxx", pedido->pid_solicitante);

		informar_fin_de_tarea(socket_kernel, IO_OK, pedido->pid_solicitante);
	}
}
void crear_archivo(char *nombre)
{
	int bloque = get_next_free_block();
	ocupar_bloque(bloque);
	crear_metadata(nombre, bloque);
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
	config_set_value(metadata, "BLOQUE_INICIAL", string_itoa(bloque));
	config_set_value(metadata, "TAMANIO_ARCHIVO", string_itoa(0));
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
	bloques_a_liberar = bloques_a_liberar?bloques_a_liberar:1;//estoespq siempre se toma un bloque inicialmente
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
	// memset(mmbitmap, 7, tam_bitmap_file);
	// msync(mmbitmap, tam_bitmap_file, MS_SYNC);
	bitmap = bitarray_create_with_mode(mmbitmap, tam_bitmap_file, LSB_FIRST);

	////NO SE DONDE HACER munmap y fclose pq el proceso se cierra con ctrl c
	fclose(file);
	free(path);
}
void inicializar_bloques()
{
	char *nombre = "bloques.dat";
	char *path = get_complete_path(nombre);

	FILE *file;
	int file_fd;

	file = fopen(path, "r+");
	if (file == NULL) // sino existe, se crea
	{
		file = fopen(path, "w+");
		file_fd = fileno(file);
		if (ftruncate(file_fd, tam_blocks_file) != 0)
		{
			perror("Error truncating file");
			return EXIT_FAILURE;
		}
	}
	file_fd = fileno(file);
	bloques = mmap(NULL, tam_blocks_file, PROT_READ | PROT_WRITE, MAP_SHARED, file_fd, 0);
	// memset(bloques, 6, tam_blocks_file);
	// msync(bloques, tam_blocks_file, MS_SYNC);
	////NO SE DONDE HACER munmap y fclose pq el proceso se cierra con ctrl c
	fclose(file);
	free(path);
}