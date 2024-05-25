#include "client.h"
#include <readline/readline.h>

t_log *logger;
t_config *config;
pthread_t tid[2];
pthread_t hilos_generica[10];
int socket_cliente;
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
	char *ip;
	char *puerto;
	char *modulo;
	char *tipo_Interfaz;
	int	*tiempo_Unidad_Trabajo;
	/*char* ip_Memoria;
	char* puerto_Memoria;
	char path_Base_Dialfs;
	int block_Size;
	int block_Count;
	int retraso_Compactacion;*/


	ip = config_get_string_value(config, "IP_KERNEL");
	puerto = config_get_string_value(config, "PUERTO_KERNEL");
	//ip_Memoria = config_get_string_value(config, "IP_MEMORIA");
	//puerto_Memoria = config_get_string_value(config, "PUERTO_MEMORIA");
	tipo_Interfaz = config_get_string_value(config, "TIPO_INTERFAZ");
	//path_Base_Dialfs = config_get_string_value(config, "PATH_BASE_DIALFS");
	//block_Size = config_get_int_value(config, "BLOCK_SIZE");
	//block_Count = config_get_int_value(config, "BLOCK_COUNT");
	//retraso_Compactacion = config_get_int_value(config, "RETRASO_COMPACTACION");


	socket_cliente = crear_conexion(ip, puerto, logger);
	int resultado = handshake(socket_cliente);
	if (resultado == -1)
		return;

	if(strcmp(tipo_Interfaz,"GENERICA") == 0){
		pthread_create(&(hilos_generica[0]), NULL, iniciar_interfaz_generica, NULL); //hay que ver como se crean
	}
/*
	if(strcmp(tipo_Interfaz,"STDIN") == 0){
		//interfaz_stdin(conexion_fd);
	}

	if(strcmp(tipo_Interfaz,"STDOUT") == 0{
		//interfaz_stdout(conexion_fd);
	}

	if(strcmp(tipo_Interfaz,"FS") == 0{
		//interfaz_fs(conexion_fd, path_Base_Dialfs, block_Size, block_Count, retraso_Compactacion);
	}
	*/
}
void interfaz_generica(int tiempo_Unidad_Trabajo){
    sleep(tiempo_Unidad_Trabajo*10);
	
    
}
void iniciar_interfaz_generica(){

	int tiempo_Unidad_Trabajo = config_get_int_value(config, "TIEMPO_UNIDAD_TRABAJO");
	t_interfaz *nueva_interfaz = crear_estrcutura_io(GENERICA);
	enviar_interfaz(CREACION_IO, *nueva_interfaz, crear_conexion);
	while (nueva_interfaz->estado == DISPONIBLE)
	{
		int cod_op = recibir_operacion(socket_cliente);
        switch (cod_op)
        {
        case SOLICITAR_IO:
            interfaz_generica(tiempo_Unidad_Trabajo);
            break;
        default:
            log_warning(logger, "Operacion desconocida. No quieras meter la pata");
            break;
        }
	}
    
}
int main(void)
{
	int nombre_ios = 0;
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
