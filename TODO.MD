# BUGS #
- [ ] si mantenes memoría y bajaste los otros modulos,cuando volves a iniciar_proceso, tira segmentation fault memoria pq no se recibe el path.
- [ ] SI BAJAS UNA IO Y VOLVES A PEDIR IO, SEGFAULT!
- [x] BREAKPOINTS PUEDEn ROMPER MEMORIA(de alguna forma el codigo esta vacio)
____
#### CHECKPOINT 2 ####
- [ ] integrar blocked en planning(manejar syscallsIO)
- [ ] Pensar check interrupt:(hay que validar q la interrupcion que llega sea de ese PID(capaz de esa rafaja de cpu tamb),creo. otra alternativa es matar el hilo que cuenta el quantum al desalojar un proceso)
- [ ] Implementar RR
- [ ] Integrar estructura de comunicacion IO->kernel->cpu
- [ ] probar q ande TODO(todo).

- [ ] Revisar que la implementacion de cpu sea teoricamente correcta(q las cosas se hagan en la fase correspondiente).
- [ ] cambiar los tipos de 'PID' de int a U_int_X (creo que no es muy necesario, pero capaz genera inconsistencias)
- [X] Funcion crear_estructuras_administrativas
- [X] recibir estructuras_administrativas en memoria
- [X] Terminar de implementar consola(y conexion con memoria para ejecutar Comandos)
- [X] Implementar envio de instrucciones a cpu desde memoria.
- [X] Revisar si hace falta un hilo EN KERNEL que solo este en rcv para escuchar las solicitudes a IO que hace CPU. **RTA:VAN POR DISPATCH**
- [X] Revisar instrucciones con pocos parametros recibiendo cualquier cosa en los ultimosp parametros
#### CHECKPOINT 3 #######
- [ ] Como lograr que el contador que se encarga de los nombre de las IO se comparta entre los archivos
- [ ] Agregar logs requeridos en consigna
- [ ] Refactorizar instrucciones: Escribir los registros con memcpy (el size de escritura dependería del registro mandado).