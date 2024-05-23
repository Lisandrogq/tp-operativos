#include "client.h"
#include <readline/readline.h>

t_log *logger;
t_config *config;
pthread_t tid[2];

int inicializar_cliente_memoria() // todavia no se usa
{
	int conexion_fd;
	char *ip;
	char *puerto;
	char *modulo;

	modulo = config_get_string_value(config, "MODULO");
	log_info(logger, "Este es el modulo: %s", modulo);
	ip = config_get_string_value(config, "IP_MEMORIA");
	puerto = config_get_string_value(config, "PUERTO_MEMORIA");

	conexion_fd = crear_conexion(ip, puerto, logger);
	int resultado = handshake(conexion_fd);
	if (resultado == -1)
		return;
	
	return conexion_fd;
}
int inicializar_cliente_kernel()
{
	int conexion_fd;
	char *ip;
	char *puerto;
	char *modulo;

	modulo = config_get_string_value(config, "MODULO");
	log_info(logger, "Este es el modulo: %s", modulo);
	ip = config_get_string_value(config, "IP_KERNEL");
	puerto = config_get_string_value(config, "PUERTO_KERNEL");

	conexion_fd = crear_conexion(ip, puerto, logger);
	int resultado = handshake(conexion_fd);
	if (resultado == -1)
		return;
	
	return conexion_fd;
}

int main(void)
{
	
	logger = iniciar_logger();
	logger = log_create("IO.log","IO_MateLavado",1,LOG_LEVEL_INFO);
	config = iniciar_config();
	config = config_create("IO.config");
	int err;
	err = pthread_create(&(tid[0]), NULL, inicializar_cliente_memoria, NULL); // todavia no se usa
	if (err != 0)
	{
		log_error(logger, "Hubo un problema al crear el thread cliente-memoria:[%s]", strerror(err));
		return err;
	}
	log_info(logger, "El thread cliente-memoria inició su ejecución");
	err = pthread_create(&(tid[1]), NULL, inicializar_cliente_kernel, NULL);
	if (err != 0)
	{
		log_error(logger, "Hubo un problema al crear el thread cliente-kernel:[%s]", strerror(err));
		return err;
	}
	log_info(logger, "El thread cliente-kernel inició su ejecución");

	pthread_join(tid[0], NULL); //join para que no termine el main creo que puede llegar a terminar igual
	pthread_join(tid[1], NULL); 
	

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


void terminar_programa(int conexion, t_log* logger, t_config* config)
{
	log_destroy(logger);
	config_destroy(config);
	liberar_conexion(conexion);
}
