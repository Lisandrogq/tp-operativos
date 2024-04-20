#include "client.h"
#include <readline/readline.h>

int main(void)
{
	int conexion_fd;
	char* ip;
	char* puerto;
	char* modulo;

	t_log* logger;
	t_config* config;

	logger = iniciar_logger();
	
	logger = log_create("IO.log","IO_MateLavado",1,LOG_LEVEL_INFO);
	log_info(logger,"Soy un log!");


	config = iniciar_config();
	config = config_create("IO.config");
	modulo = config_get_string_value(config, "MODULO");
	log_info(logger,modulo);
	ip = config_get_string_value(config, "IP");
	puerto = config_get_string_value(config, "PUERTO");
	
	conexion_fd = crear_conexion(ip, puerto);
	int resultado = handshake(conexion_fd);
	
	enviar_operacion(OPERACION_IO_1,modulo, conexion_fd); // modulo es la info que se mand
	enviar_operacion(MENSAJE,"segundo mensaje", conexion_fd);
	terminar_programa(conexion_fd, logger, config);

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
