#include <stdlib.h>
#include <stdio.h>
#include <utils/hello.c>
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
    logger = log_create("kernel.log", "Servidor", 1, LOG_LEVEL_DEBUG);
    int server_fd = iniciar_servidor();
    log_info(logger, "Kernel listo para recibir");
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

    // while (1)
    // {
	// 	int cod_op = recibir_operacion(cliente_fd);
    //     switch (cod_op)
    //     {
    //     case MENSAJE:
	// 		recibir_mensaje(cliente_fd);
    //         break;
    //     case -1:
    //         log_error(logger, "el cliente se desconecto. Terminando servidor");
    //         return EXIT_FAILURE;
    //     default:
    //         log_warning(logger, "Operacion desconocida. No quieras meter la pata");
    //         break;
    //     }
    // }

    decir_hola("Kernel_Servidor");
    return 0;
}