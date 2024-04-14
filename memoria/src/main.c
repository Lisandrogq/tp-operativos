#include <stdlib.h>
#include <stdio.h>
#include <utils/hello.c>
#include <string.h>
#include <commons/log.h>
#include "utils.h"

int main(int argc, char *argv[])
{
    logger = log_create("log.log", "Servidor", 1, LOG_LEVEL_DEBUG);
    int server_fd = iniciar_servidor();
    log_info(logger, "Memoria lista para recibir");
    int cliente_fd = esperar_cliente(server_fd);

    recibir_operacion(cliente_fd);
    handshake(cliente_fd);

    while (1)
    {

		int cod_op = recibir_operacion(cliente_fd);
        switch (cod_op)
        {
        case MENSAJE:
			recibir_mensaje(cliente_fd);
            break;
        case -1:
            log_error(logger, "el cliente se desconecto. Terminando servidor");
            return EXIT_FAILURE;
        default:
            log_warning(logger, "Operacion desconocida. No quieras meter la pata");
            break;
        }
    }

    decir_hola("Memoriaa");
    return 0;
}
