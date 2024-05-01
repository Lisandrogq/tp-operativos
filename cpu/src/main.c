#include <stdlib.h>
#include <stdio.h>
#include <utils/utils_generales.h>
#include <string.h>
#include <commons/log.h>
#include <readline/readline.h>
#include <pthread.h>
#include <semaphore.h>
#include "utils.h"

pthread_t tid[2];
void * cliente(){

    int conexion_fd;
	char* ip;
	char* puerto;
	char* modulo;

	t_log* logger;
	t_config* config;

	logger = iniciar_logger();
	
	logger = log_create("cpu.log","Cpu_MateLavado",1,LOG_LEVEL_INFO);
	log_info(logger,"Soy un log!");


	config = iniciar_config();
	config = config_create("cpu.config");
	modulo = config_get_string_value(config, "MODULO");
	log_info(logger,modulo);
	ip = config_get_string_value(config, "IP_MEMORIA");
	puerto = config_get_string_value(config, "PUERTO_MEMORIA");
	
	conexion_fd = crear_conexion(ip, puerto,logger);
	int resultado = handshake(conexion_fd);

	enviar_operacion(OPERACION_CPU_1,modulo, conexion_fd);
	enviar_operacion(MENSAJE,"segundo mensaje", conexion_fd);
	terminar_programa(conexion_fd, logger, config);
	return 0;
}

void * servidor_interrupt(){
    logger = log_create("cpu.log", "Servidor", 1, LOG_LEVEL_DEBUG);
    int server_fd = iniciar_servidor(PUERTO_CPU_INTERRUPT,logger);
    log_info(logger, "Cpu-interruptlisto para recibir");
    int socket_cliente = esperar_cliente_cpu(server_fd);
	client_handler_interrupt(socket_cliente);
}

void * servidor_dispatch(){
    logger = log_create("cpu.log", "Servidor", 1, LOG_LEVEL_DEBUG);
    int server_fd = iniciar_servidor(PUERTO_CPU_DISPATCH,logger);
    log_info(logger, "Cpu-dipatch listo para recibir");
    int socket_cliente = esperar_cliente_cpu(server_fd);
	client_handler_dispatch(socket_cliente);
	
}
t_log* iniciar_logger(void)
{
	t_log* nuevo_logger;

	return nuevo_logger;
}

t_config* iniciar_config(void)
{
	t_config* nuevo_config;

	return nuevo_config;
}
int terminarServidor(int server_fd, int client_fd)
{
	log_info(logger, "Terminando servidor");
	close(server_fd);
	close(client_fd);
	return EXIT_SUCCESS;
}
void terminar_programa(int conexion, t_log* logger, t_config* config)
{
	log_destroy(logger);
	config_destroy(config);
	liberar_conexion(conexion);
}
int main(int argc, char const *argv[])
{   
    int err;
    err = pthread_create(&(tid[0]), NULL, servidor_dispatch, NULL);
    if (err != 0){
    	printf("\nHubo un problema al crear el thread servidor-cpu-dispatch:[%s]", strerror(err));
    	return err;
    }
    printf("\nEl thread servidor-cpu-dispatch inició su ejecución\n");

 err = pthread_create(&(tid[1]), NULL, servidor_interrupt, NULL);
    if (err != 0){
    	printf("\nHubo un problema al crear el thread servidor-cpu-interrupt:[%s]", strerror(err));
    	return err;
    }
    printf("\nEl thread servidor-cpu-interrupt inició su ejecución\n");


    err = pthread_create(&(tid[2]), NULL, cliente, NULL);
    if (err != 0){
    	printf("\nHubo un problema al crear el thread cliente:[%s]", strerror(err));
    	return err;
    }
    printf("\nEl thread cliente inició su ejecución\n");
    pthread_join(tid[0], NULL);
    pthread_join(tid[1], NULL);
    pthread_join(tid[2], NULL);
    return 0;
}