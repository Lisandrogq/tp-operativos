#include <utils/utils_generales.h>
#include <errno.h>
void decir_hola(char* quien) {
    printf("Hola desde %s!!\n", quien);
}


int crear_conexion(char *ip, char *puerto,t_log *logger)
{
    struct addrinfo hints;
    struct addrinfo *server_info;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    getaddrinfo(ip, puerto, &hints, &server_info);

    // Ahora vamos a crear el socket.
    int socket_cliente = socket(server_info->ai_family, server_info->ai_socktype, server_info->ai_protocol);
    if (socket_cliente == -1)
    {
        log_error(logger, "Error at socket: %s\n", strerror(errno));
        return -1;
    }

    // Ahora que tenemos el socket, vamos a conectarlo

    freeaddrinfo(server_info); /// creo q esto deberia ir al final, antes del return
    int result = connect(socket_cliente, server_info->ai_addr, server_info->ai_addrlen);
    if (result == -1)
    {
        log_error(logger, "Error at connect: %s\n", strerror(errno));
        return -1;
    }
    return socket_cliente;
}


int esperar_cliente(int socket_servidor, void *client_handler (void*))
{
	while (1) // reemplazar por condicion de terminacion de sistema/modulo
	{
		pthread_t thread;
		int *socket_cliente = malloc(sizeof(int)); // cuando liberar esto, en el handler??
		// Aceptamos un nuevo cliente
		*socket_cliente = accept(socket_servidor, NULL, NULL);
		pthread_create(&thread,
					   NULL,
					   client_handler,
					   socket_cliente);
		pthread_detach(thread); // creo q debería ser detach pq la condicion de terminacion de sistema es externa
	}
}


int iniciar_servidor(char* PUERTO,t_log *logger)
{
    int socket_servidor;

    struct addrinfo hints, *server_info; //, *p;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    getaddrinfo(NULL, PUERTO, &hints, &server_info);

    // Creamos el socket de escucha del servidor
    socket_servidor = socket(server_info->ai_family, server_info->ai_socktype, server_info->ai_protocol);
    if (socket_servidor == -1)
    {
        log_error(logger,"Error at socket: %s\n", strerror(errno));
        return -1;
    }
    // Asociamos el socket a un puerto
    int resultado_bind = bind(socket_servidor, server_info->ai_addr, server_info->ai_addrlen);
    if (resultado_bind == -1)
    {
       log_error(logger,"Error at bind: %s\n", strerror(errno));
        return -1;
    }
    // Escuchamos las conexiones entrantes
    int resultado_listen = listen(socket_servidor, SOMAXCONN); // error en el listen
     if (resultado_listen == -1)
    {
        log_error(logger,"Error at listen: %s\n", strerror(errno));
        return -1;
    }
    freeaddrinfo(server_info);

    return socket_servidor;
}
pcb_t *crear_pcb(int pid)
{// capaz hay q setear el quantum cuando sea RR
	pcb_t *nuevo_pcb = malloc(sizeof(pcb_t));
	memset(nuevo_pcb, 0, sizeof(pcb_t));

    nuevo_pcb->pid = pid;
   // nuevo_pcb->state = malloc(sizeof(state_t)); esto da error, revisar
   // nuevo_pcb->state = NEW_S;
	registros_t *registros = malloc(sizeof(registros_t));//CHEQUEAR esto, creo q esta bien
	memset(registros, 0, sizeof(registros_t));
	nuevo_pcb->registros= registros;

	return nuevo_pcb;
}
