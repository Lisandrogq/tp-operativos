#include <stdlib.h>
#include <stdio.h>
#include <utils/hello.h>

#include <string.h>
#include <commons/log.h>
#include "utils.h"



// typedef enum
// {
// 	KERNEL,
// 	CPU,
// 	MEMORIA,
// 	IO
// }  int32_t; // NUMEROS DE HANDSHAKE, pasar esto a un archivo global


int main(int argc, char *argv[])
{
    logger = log_create("log.log", "Servidor", 1, LOG_LEVEL_DEBUG);
    int server_fd = iniciar_servidor();
    log_info(logger, "CPU lista para recibir");
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
        case OPERACION_KERNEL_1:
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

    close(server_fd);
	return EXIT_FAILURE;
}
int terminarServidor(int server_fd, int client_fd)
{
	log_info(logger, "Terminando servidor");
	close(server_fd);
	close(client_fd);
	return EXIT_SUCCESS;
}
