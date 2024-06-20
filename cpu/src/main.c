#include <stdlib.h>
#include <stdio.h>
#include <utils/utils_generales.h>
#include <string.h>
#include <commons/log.h>
#include <commons/collections/dictionary.h>
#include <readline/readline.h>
#include <pthread.h>
#include <semaphore.h>
#include "utils.h"

pthread_t tid[2];

bool terminar_modulo = false;

void ejecutar_cliclos()
{
	while (!terminar_modulo)
	{
		int status = STATUS_OK;
		sem_wait(&hay_proceso);
		while (status != STATUS_DESALOJADO)
		{
			t_strings_instruccion *instruccion = fetch(pcb_exec->registros->PC);

			status = decode(instruccion);
			// if temporal(las exec_van a estar adentro de decode) //!!to do despues del merge con recursos
			if (status != STATUS_DESALOJADO)
			{
				int result = execute(instruccion);
				status = result;
			}
			if (status == STATUS_OK) // si el proceso justo desalojo en execute, la interrupcion se leera en luego de
			{						 // que se ejecute la siguiente ejecucion
									 // revisar si esto es teoricamente correcto, creo qsi
				check_intr(&status); // asi puede marcar el proceso como desalojado
			}
		}
	}
}

void solicitar_siguiente_instruccion()
{

	t_paquete *paquete = malloc(sizeof(t_paquete));

	paquete->codigo_operacion = FETCH;
	paquete->buffer = malloc(sizeof(t_buffer));
	paquete->buffer->size = sizeof(int) * 2;
	paquete->buffer->stream = malloc(paquete->buffer->size);
	paquete->buffer->offset = 0;

	memcpy(paquete->buffer->stream + paquete->buffer->offset, &(pcb_exec->pid), sizeof(int));
	paquete->buffer->offset += sizeof(int);

	memcpy(paquete->buffer->stream + paquete->buffer->offset, &(pcb_exec->registros->PC), sizeof(int));
	paquete->buffer->offset += sizeof(int);

	int bytes = paquete->buffer->size + 2 * sizeof(int); // ESTE *2 NO SE PUEDE TOCAR, ANDA ASÍ, PUNTO(.).

	void *a_enviar = serializar_paquete(paquete, bytes);

	send(socket_memoria, a_enviar, bytes, 0);
	free(a_enviar);
	eliminar_paquete(paquete);
}

t_strings_instruccion *recibir_siguiente_instruccion()
{
	int codop = recibir_operacion(socket_memoria); // m lo saco de encima pq esta en el paquete xd

	t_buffer *buffer = malloc(sizeof(t_buffer));
	recv(socket_memoria, &(buffer->size), sizeof(int), 0);
	buffer->stream = malloc(buffer->size);
	recv(socket_memoria, buffer->stream, buffer->size, 0);

	t_strings_instruccion *palabras = malloc(sizeof(t_strings_instruccion));
	memset(palabras, 0, sizeof(t_strings_instruccion));

	void *stream = buffer->stream;
	memcpy(&(palabras->tamcod), stream, sizeof(int));
	stream += sizeof(int);
	palabras->cod_instruccion = malloc(palabras->tamcod);
	memcpy(palabras->cod_instruccion, stream, palabras->tamcod);
	stream += palabras->tamcod;

	memcpy(&(palabras->tamp1), stream, sizeof(int));
	stream += sizeof(int);

	palabras->p1 = malloc(palabras->tamp1);
	memset(palabras->p1, 0, 1); // se pone el unico byte alocado por malloc(0) en 0 para limpiar la basura(caso parametro vacio)
	memcpy((palabras->p1), stream, palabras->tamp1);
	stream += palabras->tamp1;

	memcpy(&(palabras->tamp2), stream, sizeof(int));
	stream += sizeof(int);

	palabras->p2 = malloc(palabras->tamp2);
	memset(palabras->p2, 0, 1); // se pone el unico byte alocado por malloc(0) en 0 para limpiar la basura(caso parametro vacio)

	memcpy((palabras->p2), stream, palabras->tamp2);
	stream += palabras->tamp2;

	memcpy(&(palabras->tamp3), stream, sizeof(int));
	stream += sizeof(int);

	palabras->p3 = malloc(palabras->tamp3);
	memset(palabras->p3, 0, 1); // se pone el unico byte alocado por malloc(0) en 0 para limpiar la basura(caso parametro vacio)

	memcpy((palabras->p3), stream, palabras->tamp3);
	stream += palabras->tamp3;

	memcpy(&(palabras->tamp4), stream, sizeof(int));
	stream += sizeof(int);

	palabras->p4 = malloc(palabras->tamp4);
	memset(palabras->p4, 0, 1); // se pone el unico byte alocado por malloc(0) en 0 para limpiar la basura(caso parametro vacio)

	memcpy((palabras->p4), stream, palabras->tamp4);
	stream += palabras->tamp4;

	memcpy(&(palabras->tamp5), stream, sizeof(int));
	stream += sizeof(int);

	palabras->p5 = malloc(palabras->tamp5);
	memset(palabras->p5, 0, 1); // se pone el unico byte alocado por malloc(0) en 0 para limpiar la basura(caso parametro vacio)

	memcpy((palabras->p5), stream, palabras->tamp5);
	stream += palabras->tamp5;

	// en algun lugar(afuera de esto) hay q hacerle malloc a las palabras.
	free(buffer->stream);
	free(buffer);
	return palabras;
}

t_strings_instruccion *fetch(int PC)
{
	// ACA IRÍA UN WAIT('HAY_PROCESO_PARA_EJECUTAR') QUE SE ACTUALIZA CUANDO SE RECIBE UN CONTEXTO EN DISPATCH;
	log_fetch_instruccion();
	solicitar_siguiente_instruccion();
	t_strings_instruccion *palabras = recibir_siguiente_instruccion();

	pcb_exec->registros->PC += 1; // EM LA CONSINGA DICE: HACER ESTO SI CORRESPONDE, NO SE Q SIGNIFICA

	return palabras;
}
int decode(t_strings_instruccion *instruccion)
{

	if (strcmp(instruccion->cod_instruccion, "IO_GEN_SLEEP") == 0)
	{
		// traduccion de direcciones
		buffer_instr_io_t *buffer_intruccion = malloc(sizeof(buffer_instr_io_t));
		buffer_intruccion->buffer = malloc(sizeof(int));
		int cant_sleep = atoi(instruccion->p2);
		memcpy(buffer_intruccion->buffer, &cant_sleep, sizeof(int));
		buffer_intruccion->size = sizeof(int);
		devolver_pcb(IO_TASK, *pcb_exec, socket_dispatch, instruccion, buffer_intruccion); // habria que ponerle mutex a dispatch
		return STATUS_DESALOJADO;
	}
	if (strcmp(instruccion->cod_instruccion, "MOV_IN") == 0) // MOV_IN (Registro Datos, Registro Dirección)
	{
		int dir_logica = 0;
		void *p_dir_logica = 0;
		int tam_r_dir = *((int *)dictionary_get(dic_tam_registros, instruccion->p2));
		p_dir_logica = dictionary_get(dic_p_registros, instruccion->p2);

		memcpy(&dir_logica, p_dir_logica, tam_r_dir);

		int tam_r_datos = *((int *)dictionary_get(dic_tam_registros, instruccion->p1));
		void *datos = dictionary_get(dic_p_registros, instruccion->p1);

		t_list *solicitudes = obtener_direcciones_fisicas_read(dir_logica, tam_r_datos);
		execute_mov_in(solicitudes, datos);
		liberar_y_eliminar_solicitudes(solicitudes);
	}

	if (strcmp(instruccion->cod_instruccion, "MOV_OUT") == 0) // MOV_OUT (Registro Dirección, Registro Datos)
	{
		int dir_logica = 0;
		void *p_dir_logica = 0;
		int tam_r_dir = *((int *)dictionary_get(dic_tam_registros, instruccion->p1));
		p_dir_logica = dictionary_get(dic_p_registros, instruccion->p1);

		memcpy(&dir_logica, p_dir_logica, tam_r_dir);

		int tam_r_datos = *((int *)dictionary_get(dic_tam_registros, instruccion->p2));
		void *datos = dictionary_get(dic_p_registros, instruccion->p2);

		t_list *solicitudes = obtener_direcciones_fisicas_write(datos, dir_logica, tam_r_datos);

		int status = execute_mov_out(solicitudes); // los datosa escribir no se pasan pq estan en solicitud
		liberar_y_eliminar_solicitudes(solicitudes);
		if (status == MEM_W_NO_OK)
		{
			// NOSE QUE DICE LA CONSIGNAXD
		}
	}

	if (strcmp(instruccion->cod_instruccion, "IO_STDIN_READ") == 0) // IO_STDIN_READ (Interfaz, Registro Dirección, Registro Tamaño)
	{
		// a la io se le manda una lista de solicitudes con el .datos vacio(sin malloc)
		// y alla se populan las solicitudes de la forma iterator para luego enviarlos a memoría de la forma mov_out
		void *p_max_tam = dictionary_get(dic_p_registros, instruccion->p3);
		int tam_r_max_tam = *(int *)dictionary_get(dic_tam_registros, instruccion->p3);
		int max_tam = 0;
		memcpy(&max_tam, p_max_tam, tam_r_max_tam);

		int dir_logica = 0;
		void *p_dir_logica = 0;
		int tam_r_dir = *((int *)dictionary_get(dic_tam_registros, instruccion->p2));
		p_dir_logica = dictionary_get(dic_p_registros, instruccion->p2);
		memcpy(&dir_logica, p_dir_logica, tam_r_dir);

		t_list *solicitudes = obtener_direcciones_fisicas_read(dir_logica, max_tam);

		// serializar solicitudes

		buffer_instr_io_t *buffer_instruccion = serializar_solicitudes(solicitudes, max_tam);
		devolver_pcb(IO_TASK, *pcb_exec, socket_dispatch, instruccion, buffer_instruccion);
		liberar_y_eliminar_solicitudes(solicitudes);
		return STATUS_DESALOJADO;
	}
	if (strcmp(instruccion->cod_instruccion, "IO_STDOUT_WRITE") == 0) // IO_STDOUT_WRITE (Interfaz, Registro Dirección, Registro Tamaño)
	{

		void *p_max_tam = dictionary_get(dic_p_registros, instruccion->p3);
		int tam_r_max_tam = *(int *)dictionary_get(dic_tam_registros, instruccion->p3);
		int max_tam = 0;
		memcpy(&max_tam, p_max_tam, tam_r_max_tam);

		int dir_logica = 0;
		void *p_dir_logica = 0;
		int tam_r_dir = *((int *)dictionary_get(dic_tam_registros, instruccion->p2));
		p_dir_logica = dictionary_get(dic_p_registros, instruccion->p2);
		memcpy(&dir_logica, p_dir_logica, tam_r_dir);

		t_list *solicitudes = obtener_direcciones_fisicas_read(dir_logica, max_tam);

		buffer_instr_io_t *buffer_instruccion = serializar_solicitudes(solicitudes, max_tam);
		devolver_pcb(IO_TASK, *pcb_exec, socket_dispatch, instruccion, buffer_instruccion);
		liberar_y_eliminar_solicitudes(solicitudes);
		return STATUS_DESALOJADO;
	}
	if (strcmp(instruccion->cod_instruccion, "IO_FS_CREATE") == 0) // IO_FS_CREATE (Interfaz, Nombre Archivo):
	{

		buffer_instr_io_t *buffer_instruccion = serializar_nombre(instruccion->p2, IO_FS_CREATE);
		devolver_pcb(IO_TASK, *pcb_exec, socket_dispatch, instruccion, buffer_instruccion);
		return STATUS_DESALOJADO;
	}
	if (strcmp(instruccion->cod_instruccion, "IO_FS_DELETE") == 0) //IO_FS_DELETE (Interfaz, Nombre Archivo):
	{

		buffer_instr_io_t *buffer_instruccion = serializar_nombre(instruccion->p2, IO_FS_DELETE);
		devolver_pcb(IO_TASK, *pcb_exec, socket_dispatch, instruccion, buffer_instruccion);
		return STATUS_DESALOJADO;
	}
	if (strcmp(instruccion->cod_instruccion, "IO_FS_TRUNCATE") == 0) //IO_FS_TRUNCATE (Interfaz, Nombre Archivo, Registro Tamaño):
	{
		void *p_r_bytes = dictionary_get(dic_p_registros, instruccion->p3);
		int tam_r_bytes = *(int *)dictionary_get(dic_tam_registros, instruccion->p3);
		int bytes = 0;
		memcpy(&bytes, p_r_bytes, tam_r_bytes);

		buffer_instr_io_t *buffer_instruccion = serializar_truncate_sol(instruccion->p2,bytes);
		devolver_pcb(IO_TASK, *pcb_exec, socket_dispatch, instruccion, buffer_instruccion);
		return STATUS_DESALOJADO;
	}
	if (strcmp(instruccion->cod_instruccion, "COPY_STRING") == 0) // COPY_STRING (Tamaño)
	{
		int tam_string = atoi(instruccion->p1);
		void *buffer_intermedio = malloc(tam_string);
		memset(buffer_intermedio, 0, tam_string + 1); // p agregar /0

		u_int32_t *dir_logica_read = dictionary_get(dic_p_registros, "SI"); // siempre se usa SI

		t_list *solicitudes_r = obtener_direcciones_fisicas_read(*dir_logica_read, tam_string);
		execute_mov_in(solicitudes_r, buffer_intermedio);
		log_info(logger, "EL STRING COPIADO ES:%s", buffer_intermedio);
		liberar_y_eliminar_solicitudes(solicitudes_r);

		// traduccion write
		u_int32_t *dir_logica_write = dictionary_get(dic_p_registros, "DI");
		t_list *solicitudes_w = obtener_direcciones_fisicas_write(buffer_intermedio, *dir_logica_write, tam_string);
		execute_mov_out(solicitudes_w);
		liberar_y_eliminar_solicitudes(solicitudes_w);

		/* //TEST: lo escrito por 2 proceso simultaneamente esta bien:
		t_list *solicitudes_T = obtener_direcciones_fisicas_read(*dir_logica_write, tam_string);
		execute_mov_in(solicitudes_T, buffer_intermedio);
		log_info(logger, "EL STRING ESCRITO, AL VOLVER A LEERSE ES:%s", buffer_intermedio);
		liberar_y_eliminar_solicitudes(solicitudes_T); */
	}

	if (strcmp(instruccion->cod_instruccion, "RESIZE") == 0) // RESIZE (bytes)
	{
		int bytes = atoi((instruccion->p1));

		enviar_solicitud_resize(pcb_exec->pid, bytes);
		int status = recibir_status_resize();
		if (status == -1)
		{
			devolver_pcb(OUT_OF_MEMORY, *pcb_exec, socket_dispatch, instruccion, 0); // habria que ponerle mutex a dispatch
			return STATUS_DESALOJADO;
		}
	}

	return STATUS_OK;
}

int recibir_status_resize()
{
	t_paquete *paquete = malloc(sizeof(t_paquete));
	paquete->buffer = malloc(sizeof(t_buffer));

	int cod_op = recibir_operacion(socket_memoria); // me saco el cdo
	recv(socket_memoria, &(paquete->buffer->size), sizeof(int), 0);
	paquete->buffer->stream = malloc(paquete->buffer->size);
	recv(socket_memoria, paquete->buffer->stream, paquete->buffer->size, 0);

	int status = 0;
	void *stream = paquete->buffer->stream;
	memcpy(&status, stream, sizeof(int));

	eliminar_paquete(paquete);

	return status;
}
void enviar_solicitud_resize(int pid, int bytes_solicitados)
{

	t_paquete *paquete = malloc(sizeof(t_paquete));

	paquete->codigo_operacion = RESIZE;
	paquete->buffer = malloc(sizeof(t_buffer));
	paquete->buffer->size = sizeof(int) * 2;
	paquete->buffer->stream = malloc(paquete->buffer->size);
	paquete->buffer->offset = 0;

	memcpy(paquete->buffer->stream + paquete->buffer->offset, &pid, sizeof(int)); // paso los datos pq si,desde cpu ya se sabe
	paquete->buffer->offset += sizeof(int);

	memcpy(paquete->buffer->stream + paquete->buffer->offset, &bytes_solicitados, sizeof(int)); // paso los datos pq si,desde cpu ya se sabe
	paquete->buffer->offset += sizeof(int);
	int bytes = paquete->buffer->size + 2 * sizeof(int);

	void *a_enviar = serializar_paquete(paquete, bytes);

	send(socket_memoria, a_enviar, bytes, 0);
	free(a_enviar);
	eliminar_paquete(paquete);
}
t_list *obtener_direcciones_fisicas_read(int dir_logica, int tam_r_datos) // return list sol_unitaria_t*
{
	t_list *solicitudes = list_create();
	int pagina = floor((dir_logica) / tam_pagina);
	int offset = dir_logica - pagina * tam_pagina;
	if (tam_r_datos > tam_pagina - offset)
	{
		int tam_siguiente = tam_pagina; // pag 4, offset 2, t_data = 3
		while (tam_siguiente > 0)
		{
			solicitud_unitaria_t *sol = malloc(sizeof(solicitud_unitaria_t)); // free de esto luego de pedir
			tam_siguiente = tam_siguiente - offset;
			sol->pid = pcb_exec->pid;
			sol->tam = tam_siguiente;
			sol->pagina = pagina;
			sol->offset = offset;
			sol->datos = malloc(sol->tam);

			pagina += 1;
			offset = 0;
			tam_r_datos = tam_r_datos - tam_siguiente;
			tam_siguiente = tam_r_datos;
			if (tam_siguiente > tam_pagina)
				tam_siguiente = tam_pagina;
			list_add(solicitudes, sol);
		}
		list_map(solicitudes, traducir_a_dir_fisica);
	}
	else
	{
		solicitud_unitaria_t *sol = malloc(sizeof(solicitud_unitaria_t));
		sol->pid = pcb_exec->pid;
		sol->pagina = pagina;
		sol->offset = offset;
		sol->tam = tam_r_datos;
		sol->datos = malloc(sol->tam);
		list_add(solicitudes, sol);
		list_map(solicitudes, traducir_a_dir_fisica); // xd
	}
	return solicitudes;
}

t_list *obtener_direcciones_fisicas_write(void *datos, int dir_logica, int tam_r_datos) // return list sol_unitaria_t*
{
	t_list *solicitudes = list_create();
	int pagina = floor((dir_logica) / tam_pagina);
	int offset = dir_logica - pagina * tam_pagina;
	int write_offset = 0;
	if (tam_r_datos > tam_pagina - offset)
	{
		int tam_siguiente = tam_pagina;
		while (tam_siguiente > 0)
		{
			solicitud_unitaria_t *sol = malloc(sizeof(solicitud_unitaria_t)); // free de esto luego de pedir
			tam_siguiente = tam_siguiente - offset;
			sol->pid = pcb_exec->pid;
			sol->tam = tam_siguiente;
			sol->pagina = pagina;
			sol->offset = offset;

			sol->datos = malloc(sol->tam);
			memcpy(sol->datos, datos + write_offset, sol->tam); // el pedido unitario tendra solo una parte de los datos
			write_offset += sol->tam;
			pagina += 1;
			offset = 0; // las siguientes paginas se escriben desde cero siempre

			tam_r_datos = tam_r_datos - tam_siguiente;
			tam_siguiente = tam_r_datos;
			if (tam_siguiente > tam_pagina) // limitar tamaño de la siguiente escritura
				tam_siguiente = tam_pagina;
			list_add(solicitudes, sol);
		}
		list_map(solicitudes, traducir_a_dir_fisica);
	}
	else
	{
		solicitud_unitaria_t *sol = malloc(sizeof(solicitud_unitaria_t));
		sol->pid = pcb_exec->pid;
		sol->pagina = pagina;
		sol->offset = offset;
		sol->tam = tam_r_datos;
		sol->datos = malloc(sol->tam);
		memcpy(sol->datos, datos, sol->tam);

		list_add(solicitudes, sol);
		list_map(solicitudes, traducir_a_dir_fisica); // xd
	}
	return solicitudes;
}

int recibir_frame()
{
	int cod_op = recibir_operacion(socket_memoria);

	int frame = -1;
	t_paquete *paquete = malloc(sizeof(t_paquete));
	paquete->buffer = malloc(sizeof(t_buffer));

	recv(socket_memoria, &(paquete->buffer->size), sizeof(int), 0);
	paquete->buffer->stream = malloc(paquete->buffer->size);
	recv(socket_memoria, paquete->buffer->stream, paquete->buffer->size, 0);

	void *stream = paquete->buffer->stream;
	memcpy(&frame, stream, sizeof(int));

	eliminar_paquete(paquete);

	return frame;
}
solicitud_unitaria_t *traducir_a_dir_fisica(solicitud_unitaria_t *sol)
{
	int n_frame = NULL;

	tlb_element *elemento = get_element_tlb(sol);

	if (elemento == NULL)
	{
		log_info(logger, "PID: %i - TLB MISS - Pagina: %i", pcb_exec->pid, sol->pagina);

		n_frame = solicitar_frame_a_memoria(sol);

		tlb_element *elemento = malloc(sizeof(elemento));
		elemento->pid = pcb_exec->pid;
		elemento->pagina = sol->pagina;
		elemento->frame = n_frame;

		if (strcmp(ALGORITMO_TLB, "LRU") == 0)
		{
			elemento->timestamp = temporal_gettime(cronometro_lru);
		}
		actualizar_tlb(elemento);
	}
	else
	{
		n_frame = elemento->frame;
		log_info(logger, "PID: %i - TLB HIT - Pagina: %i", pcb_exec->pid, sol->pagina);
		if (strcmp(ALGORITMO_TLB, "LRU") == 0)
		{
			elemento->timestamp = temporal_gettime(cronometro_lru);
		}
	}
	// esto se hace independientemente de la forma de obtencion
	sol->dir_fisica_base = n_frame * tam_pagina;
	void printear(tlb_element * elemento)
	{
		printf("(PID%i)=[%i-%i] |||| ", elemento->pid, elemento->pagina, elemento->frame);
	}
	printf("ESTADO TLB: ");
	list_iterate(tlb_list, printear);
	printf("\n");
	return sol;
}

int solicitar_frame_a_memoria(solicitud_unitaria_t *sol)
{
	t_paquete *paquete = malloc(sizeof(t_paquete));

	paquete->codigo_operacion = GET_FRAME;
	paquete->buffer = malloc(sizeof(t_buffer));
	paquete->buffer->size = sizeof(int) * 2;
	paquete->buffer->stream = malloc(paquete->buffer->size);
	paquete->buffer->offset = 0;

	memcpy(paquete->buffer->stream + paquete->buffer->offset, &(pcb_exec->pid), sizeof(u_int32_t));
	paquete->buffer->offset += sizeof(int);

	memcpy(paquete->buffer->stream + paquete->buffer->offset, &sol->pagina, sizeof(int));
	paquete->buffer->offset += sizeof(int);

	int bytes = paquete->buffer->size + 2 * sizeof(int);

	void *a_enviar = serializar_paquete(paquete, bytes);

	send(socket_memoria, a_enviar, bytes, 0);
	free(a_enviar);
	eliminar_paquete(paquete);
	int frame = recibir_frame();
	log_info(logger, "PID: %i - OBTENER MARCO - Página: %i - Marco: %i", pcb_exec->pid, sol->pagina, frame);
	return frame;
}

tlb_element *get_element_tlb(solicitud_unitaria_t *sol)
{
	bool find_page(void *arg)
	{
		tlb_element *elemento = (tlb_element *)arg;
		return elemento->pid == pcb_exec->pid && elemento->pagina == sol->pagina;
	};
	tlb_element *elemento = list_find(tlb_list, find_page);
	return elemento; // RETORNA NULL SI NO SE ENCONTRO
}
void actualizar_tlb(tlb_element *elemento) // al irse un proceso, se borra sus entradas de tlb?
{
	tlb_element *a_liberar;
	if (list_size(tlb_list) < CANTIDAD_ENTRADAS_TLB)
	{
		list_add_in_index(tlb_list, 0, elemento);
	}
	else
	{
		if (strcmp(ALGORITMO_TLB, "LRU") == 0)
		{
			a_liberar = remove_LRU_entry();
			list_add_in_index(tlb_list, 0, elemento);
		}
		if (strcmp(ALGORITMO_TLB, "FIFO") == 0)
		{
			a_liberar = list_remove(tlb_list, list_size(tlb_list) - 1);
			list_add_in_index(tlb_list, 0, elemento);
		}
		free(a_liberar);
	}
}

tlb_element *remove_LRU_entry()
{
	tlb_element *a_liberar = (tlb_element *)list_get_minimum(tlb_list, menor_timestamp);
	list_remove_element(tlb_list, a_liberar);
	return a_liberar;
}

void *menor_timestamp(tlb_element *elemento1, tlb_element *elemento2)
{
	return elemento1->timestamp <= elemento2->timestamp ? elemento1 : elemento2;
}
int execute(t_strings_instruccion *instruccion)
{
	log_instruccion_ejecutada(instruccion); // en teoría debería ir al final de cada if,xd
	if (strcmp(instruccion->cod_instruccion, "SET") == 0)
	{

		int num = atoi((instruccion->p2));
		execute_set(instruccion->p1, num);
		free((instruccion->p1));
		free(instruccion);
		return STATUS_OK;
	}
	if (strcmp(instruccion->cod_instruccion, "SUM") == 0)
	{
		// u_int32_t *p1 = dictionary_get(dictionary, instruccion->p1);
		// u_int32_t *p2 = dictionary_get(dictionary, instruccion->p2);
		execute_sum(instruccion->p1, instruccion->p2);
		free((instruccion->p1));
		free((instruccion->p2));
		free(instruccion);
		return STATUS_OK;
	}
	if (strcmp(instruccion->cod_instruccion, "SUB") == 0)
	{
		execute_sub(instruccion->p1, instruccion->p2);
		free((instruccion->p1));
		free((instruccion->p2));
		free(instruccion);
		return STATUS_OK;
	}
	if (strcmp(instruccion->cod_instruccion, "JNZ") == 0)
	{
		u_int32_t p2 = instruccion->p2;
		execute_jnz(instruccion->p1, p2, pcb_exec->registros);
		free((instruccion->p1));
		free(instruccion);
		return STATUS_OK;
	}

	if ((strstr(instruccion->cod_instruccion, "EXIT") != NULL)) // Fix termporal a aparicion random de '%'o'5'
	{

		devolver_pcb(SUCCESS, *pcb_exec, socket_dispatch, instruccion, 0); // habria que ponerle mutex a dispatch
		return STATUS_DESALOJADO;
	}
	if (strcmp(instruccion->cod_instruccion, "WAIT") == 0)
	{
		devolver_pcb(WAIT, *pcb_exec, socket_dispatch, instruccion, 0); // habria que ponerle mutex a dispatch
		sem_wait(&hay_proceso);
		return STATUS_OK;
	}
	if (strcmp(instruccion->cod_instruccion, "SIGNAL") == 0)
	{
		devolver_pcb(SIGNAL, *pcb_exec, socket_dispatch, instruccion, 0); // habria que ponerle mutex a dispatch
		sem_wait(&hay_proceso);
		return STATUS_OK; // Wait semaforo hay pcb o proceso
	}
	return STATUS_OK;
}
void check_intr(int *status)
{
	t_strings_instruccion *instruccion_vacia = malloc(sizeof(t_strings_instruccion));
	memset(instruccion_vacia, 0, sizeof(t_strings_instruccion));
	int cod_op = 0;
	recv(socket_interrupt, &cod_op, sizeof(int), MSG_DONTWAIT);
	if (cod_op == INTERRUPCION)
	{
		interrupcion_t *interrupcion = recibir_interrupcion(socket_interrupt);
		if (interrupcion->pid == pcb_exec->pid)
		{

			switch (interrupcion->motivo)
			{
			case INTERRUPTED_BY_USER:
				//	retornar por dispatch(mutex por las dudas)
				// marcar como desalojado
				devolver_pcb(INTERRUPTED_BY_USER, *pcb_exec, socket_dispatch, instruccion_vacia, 0);
				// liberar_pcb(pcb_exec);
				*status = STATUS_DESALOJADO;
				log_debug(logger, "SE DESALOJO UN PROCESO POR TERMINAR_PROCESO_INTR");
				break;
			case CLOCK:
				// retornar por dispatch(mutex por las dudas)
				//	marcar como desalojado
				devolver_pcb(CLOCK, *pcb_exec, socket_dispatch, instruccion_vacia, 0);
				// liberar_pcb(pcb_exec);
				*status = STATUS_DESALOJADO;
				log_debug(logger, "SE DESALOJO UN PROCESO POR CLOCK_INTR");
				break;
			}
		}
		else
			log_debug(logger, "Se recibio una intr para el pid %i, mientras ejecuta el pid%i motivo:%i", interrupcion->pid, pcb_exec->pid, interrupcion->motivo);
	}
	// else
	//  log_debug(logger, "No se recibio interrupciones");
}
void *servidor_interrupt()
{
	logger = log_create("cpu.log", "Servidor", 1, LOG_LEVEL_DEBUG);
	int server_fd = iniciar_servidor(PUERTO_CPU_INTERRUPT, logger);
	log_info(logger, "Cpu-interruptlisto para recibir");
	socket_interrupt = esperar_cliente_cpu(server_fd);
	// client_handler_interrupt(socket_interrupt);no hace falta,creo, las cosas se reciven en checkintr
}

void *servidor_dispatch()
{
	logger = log_create("cpu.log", "Servidor", 1, LOG_LEVEL_DEBUG);
	int server_fd = iniciar_servidor(PUERTO_CPU_DISPATCH, logger);
	log_info(logger, "Cpu-dipatch listo para recibir");
	socket_dispatch = esperar_cliente_cpu(server_fd);
	client_handler_dispatch(socket_dispatch);
}
t_log *iniciar_logger(void)
{
	t_log *nuevo_logger;

	return nuevo_logger;
}

t_config *iniciar_config(void)
{
	t_config *nuevo_config;

	return nuevo_config;
}
int terminarServidor(int server_fd, int client_fd)
{
	log_info(logger, "Terminando servidor");
	close(server_fd);
	close(client_fd);
	return EXIT_SUCCESS;
}
void terminar_programa(int conexion, t_log *logger, t_config *config)
{
	log_destroy(logger);
	config_destroy(config);
	liberar_conexion(conexion);
}

void iniciar_thread_dispatch()
{
	int err;
	err = pthread_create(&(tid[0]), NULL, servidor_dispatch, NULL);
	if (err != 0)
	{
		printf("\nHubo un problema al crear el thread servidor-cpu-dispatch:[%s]", strerror(err));
		return err;
	}
	printf("\nEl thread servidor-cpu-dispatch inició su ejecución\n");
}

void iniciar_thread_interrupt()
{
	int err = pthread_create(&(tid[1]), NULL, servidor_interrupt, NULL);
	if (err != 0)
	{
		printf("\nHubo un problema al crear el thread servidor-cpu-interrupt:[%s]", strerror(err));
		return err;
	}
	printf("\nEl thread servidor-cpu-interrupt inició su ejecución\n");
}
int iniciar_conexion_memoria(t_config *config, t_log *logger)
{
	int conexion_fd;
	char *ip;
	char *puerto;
	char *modulo;

	modulo = config_get_string_value(config, "MODULO");
	log_info(logger, modulo);
	ip = config_get_string_value(config, "IP_MEMORIA");
	puerto = config_get_string_value(config, "PUERTO_MEMORIA");

	conexion_fd = crear_conexion(ip, puerto, logger);
	tam_pagina = handshake(conexion_fd);

	return conexion_fd;
}

void inicializar_diccionario_tams()
{
	int *tam8 = malloc(sizeof(int));
	int *tam32 = malloc(sizeof(int));
	*tam8 = 1;
	*tam32 = 4;

	dictionary_put(dic_tam_registros, "AX", tam8);
	dictionary_put(dic_tam_registros, "BX", tam8);
	dictionary_put(dic_tam_registros, "CX", tam8);
	dictionary_put(dic_tam_registros, "DX", tam8);
	dictionary_put(dic_tam_registros, "EAX", tam32);
	dictionary_put(dic_tam_registros, "EBX", tam32);
	dictionary_put(dic_tam_registros, "ECX", tam32);
	dictionary_put(dic_tam_registros, "EDX", tam32);
	dictionary_put(dic_tam_registros, "SI", tam32);
	dictionary_put(dic_tam_registros, "DI", tam32);
}
int main(int argc, char const *argv[])
{
	cronometro_lru = temporal_create();
	sem_init(&hay_proceso, 0, 0);
	dic_p_registros = dictionary_create();
	dic_tam_registros = dictionary_create();
	inicializar_diccionario_tams();
	tlb_list = list_create();

	// dictionary = inicializar_diccionario(contexto); esto ya no hace falta, se inicializa al recibir pcb

	t_log *logger;
	t_config *config;
	logger = iniciar_logger();
	logger = log_create("cpu.log", "Cpu_MateLavado", 1, LOG_LEVEL_DEBUG);
	config = iniciar_config();
	config = config_create("cpu.config");
	CANTIDAD_ENTRADAS_TLB = config_get_int_value(config, "CANTIDAD_ENTRADAS_TLB");
	ALGORITMO_TLB = config_get_string_value(config, "ALGORITMO_TLB");
	iniciar_thread_dispatch();
	iniciar_thread_interrupt();
	socket_memoria = iniciar_conexion_memoria(config, logger);

	ejecutar_cliclos();

	pthread_join(tid[0], NULL);
	pthread_join(tid[1], NULL);
	return 0;
}