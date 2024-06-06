# BUGS #
- [x] SI BAJAS UNA IO Y VOLVES A PEDIR IO, SEGFAULT!
- [x] si mantenes memoría y bajaste los otros modulos,cuando volves a iniciar_proceso, tira segmentation fault memoria pq no se recibe el path.
- [x] BREAKPOINTS PUEDEn ROMPER MEMORIA(de alguna forma el codigo esta vacio)
- [x] hacer issue: si se finaliza un proceso justo cuando sale por sysscall, se borran las estructuras administrativas y no se mando a exit(capaz la 'solucion' es un mutex para exec)
- [x] probar  bien todos los casos especiales de planificacion + finalizar_proceso
- [ ] ARREGLAR LEAKS
- [ ] ARREGLAR cuando se mueve el offset haciendo stream+=... , Cuando se quiera liberar stream, no se va a liberar nada, pq stream esta al final.
- [ ] unificar los tipos de  de int a U_int_X ***en vm int 32 == int, capaz en el labo la pasamos mal***

- [ ] Una vez paso que un proceso entro a io y volvio a ejecutar al toque(capaz es un tema de q faltan mutex :p)
- [ ] TERMINAR DE AGREGAR MUTEX
____
#### CHECKPOINT 3 #######
- [ ] CMD:Iniciar planificación,Detener planificación,
- [ ] Manejo de recursos
- [ ] Planificador largo plazo
- [x] Memoría e instrucciones minimas
- [ ] Integrar con paginacion
- [ ] MMU
- [ ] TLB
- [ ] Instrucciones adicionales
- [ ] IOs
- [ ] El issue #3807 dice que hay retardo ante cualquier peticion, creo q hay q ponerla justo despues del codop o ago así
- [ ] Revisar uso de 'exit(-1);' para cerrar sockets

#### ENTREGA FINAL #######
- [ ] Agregar logs requeridos en consigna
- [ ] Revisar que la implementacion de cpu sea teoricamente correcta:q el saber q instruccion es se haga en decode(por traduccion)
- [ ] Refactorizar instrucciones: Escribir los registros con memcpy (el size de escritura se saca con otro diccionario)).
- [ ] Aparece un '%' al final de ALGUNOS'exit' en codigos(aparentemente cuandos son muy largos).la solucion es medio sucia, para la entrega final habría q ver q onda eso

#### CHECKPOINT 2 ####
- [x] CMD:Ejecutar Script de Operaciones.
- [x] falta manejar caso en que finaliza una io a un proceso en exit(creo q con un if está)
- [x] Terminar de hacer q los io task los maneje el kernel: el hilo con io manda al recibir un fintask y al pedir, si esta vacía la cola, se manda desde ahi.
- [x] Implementar RR
- [x] Integrar estructura de comunicacion IO->kernel->cpu
- [x] integrar blocked en planning(manejar syscallsIO)
- [X] Funcion crear_estructuras_administrativas
- [X] recibir estructuras_administrativas en memoria
- [X] Terminar de implementar consola(y conexion con memoria para ejecutar Comandos)
- [X] Implementar envio de instrucciones a cpu desde memoria.
- [X] Revisar si hace falta un hilo EN KERNEL que solo este en rcv para escuchar las solicitudes a IO que hace CPU. **RTA:VAN POR DISPATCH**
- [X] Revisar instrucciones con pocos parametros recibiendo cualquier cosa en los ultimosp parametros
- [x] check interrupt:(hay que validar q la interrupcion que llega sea de ese PID(capaz de esa rafaja de cpu tamb),creo. otra alternativa es matar el hilo que cuenta el quantum al desalojar un proceso)