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
	
	conexion_fd = crear_conexion(ip, puerto);
	int resultado = handshake(conexion_fd);

	enviar_operacion(OPERACION_CPU_1,modulo, conexion_fd);
	enviar_operacion(MENSAJE,"segundo mensaje", conexion_fd);
	terminar_programa(conexion_fd, logger, config);
	return 0;
}

void * servidor(){
    logger = log_create("cpu.log", "Servidor", 1, LOG_LEVEL_DEBUG);
    int server_fd = iniciar_servidor();
    log_info(logger, "Cpu listo para recibir");
    int cliente_fd = esperar_cliente(server_fd);

    int modulo = handshake_Server(cliente_fd);

    switch (modulo)
	{
	case 0:
	log_info(logger,"se conecto el modulo kernel");
		break;
	case 1:
	log_info(logger,"se conecto el modulo cpu");
		break;
	case 2:
	log_info(logger,"se conecto el modulo memoria");
		break;
	case 3:
	log_info(logger,"se conecto el modulo io");
		break;

	}

    while (1)
    {
	 	int cod_op = recibir_operacion(cliente_fd);
        switch (cod_op)
        {
        case OPERACION_IO_1:
			//capaz habria q cambiar el nombre de recibir_(...) a manejar_(...)
			recibir_operacion1(cliente_fd);
			break;
        case MENSAJE:
	 		recibir_mensaje(cliente_fd);
             break;
        case -1:
            log_info(logger, "Se desconecto el cliente");
			return terminarServidor(server_fd, cliente_fd); //Por ahora esta bien que se cierra en un futuro no se si sea lo mejor
			break;
        default:
             log_warning(logger, "Operacion desconocida. No quieras meter la pata");
             break;
        }
    }
	terminarServidor(server_fd, cliente_fd);
	return EXIT_FAILURE;
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
    err = pthread_create(&(tid[0]), NULL, servidor, NULL);
    if (err != 0){
    	printf("\nHubo un problema al crear el thread servidor:[%s]", strerror(err));
    	return err;
    }
    printf("\nEl thread servidor inició su ejecución\n");

    err = pthread_create(&(tid[1]), NULL, cliente, NULL);
    if (err != 0){
    	printf("\nHubo un problema al crear el thread cliente:[%s]", strerror(err));
    	return err;
    }
    printf("\nEl thread cliente inició su ejecución\n");
    pthread_join(tid[0], NULL);
    pthread_join(tid[1], NULL);
    return 0;
}