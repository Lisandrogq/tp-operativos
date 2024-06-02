# BUGS #
- [x] SI BAJAS UNA IO Y VOLVES A PEDIR IO, SEGFAULT!
- [x] si mantenes memoría y bajaste los otros modulos,cuando volves a iniciar_proceso, tira segmentation fault memoria pq no se recibe el path.
- [x] BREAKPOINTS PUEDEn ROMPER MEMORIA(de alguna forma el codigo esta vacio)
- [x] hacer issue: si se finaliza un proceso justo cuando sale por sysscall, se borran las estructuras administrativas y no se mando a exit(capaz la 'solucion' es un mutex para exec)
- [x] probar  bien todos los casos especiales de planificacion + finalizar_proceso
- [ ] ARREGLAR LEAKS
- [ ] ARREGLAR cuando se mueve el offset haciendo stream+=... , Cuando se quiera liberar stream, no se va a liberar nada, pq stream esta al final.
- [ ] cambiar los tipos de 'PID' de int a U_int_X (creo que no es muy necesario, pero capaz genera inconsistencias)
- [ ] Una vez paso que un proceso entro a io y volvio a ejecutar al toque(capaz es un tema de q faltan mutex :p)
- [ ] Revisar que la implementacion de cpu sea teoricamente correcta(q las cosas se hagan en la fase correspondiente).
- [ ] TERMINAR DE AGREGAR MUTEX
____
#### CHECKPOINT 2 ####
- [ ] CMD:Ejecutar Script de Operaciones.
- [ ] check interrupt:(hay que validar q la interrupcion que llega sea de ese PID(capaz de esa rafaja de cpu tamb),creo. otra alternativa es matar el hilo que cuenta el quantum al desalojar un proceso)

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
#### CHECKPOINT 3 #######
- [ ] CMD:Iniciar planificación,Detener planificación
- [ ] Agregar logs requeridos en consigna
- [ ] Refactorizar instrucciones: Escribir los registros con memcpy (el size de escritura dependería del registro mandado).


- [ ] Aparece un '%' al final de exit en codigos. lo solucion es medio sucia, para la entrega final habría q ver q onda eso
