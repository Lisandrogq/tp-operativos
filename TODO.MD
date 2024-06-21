# BUGS #
- [x] SI BAJAS UNA IO Y VOLVES A PEDIR IO, SEGFAULT!
- [x] si mantenes memoría y bajaste los otros modulos,cuando volves a iniciar_proceso, tira segmentation fault memoria pq no se recibe el path.
- [x] BREAKPOINTS PUEDEn ROMPER MEMORIA(de alguna forma el codigo esta vacio)
- [x] hacer issue: si se finaliza un proceso justo cuando sale por sysscall, se borran las estructuras administrativas y no se mando a exit(capaz la 'solucion' es un mutex para exec)
- [x] probar  bien todos los casos especiales de planificacion + finalizar_proceso
- [ ] ARREGLAR LEAKS
- [ ] ARREGLAR cuando se mueve el offset haciendo stream+=... , Cuando se quiera liberar stream, no se va a liberar nada, pq stream esta al final.
- [ ] unificar los tipos de  de int a U_int_X ***en vm int 32 == int, capaz en el labo la pasamos mal***
- [ ] TERMINAR DE AGREGAR MUTEX
- [ ]**SI SE EJECUTA EL CODIGO DE'res'(dos resize seguidos) SE ROMPE MEMORIA**(Hay algun mal manejo de memoria previo)
____
#### CHECKPOINT 3 #######
- [ ] CMD:Iniciar planificación,Detener planificación,
- [x] Manejo de recursos
- [x] Planificador largo plazo
- [x] Memoría e instrucciones minimas
- [x] Integrar con paginacion
- [x] MMU
- [x] TLB (FIFO)
- [x] Instrucciones adicionales
- [x] IOs
- [x] El issue #3807 dice que hay retardo ante cualquier peticion, creo q hay q ponerla justo despues del codop o ago así
- [ ] !!Agregar mutex cuando sea necesario(sockets,listas bloqueados,etc)

#### ENTREGA FINAL #######
- [ ] IMPLEMENTAR DESCONEXION DE ios
- [x] TLB (LRU)
- [ ] DETENER_PLANIFICACION, en principo un solo semaforo(en total) que bloquee las cosas.
- [X] FS CREATE
- [X] FS DELETE
- [ ] FS WRITE
- [ ] FS READ
- [x] FS TRUNCATE
- [ ] FS COMPACTACION 💀💀💀


- [ ] Agregar logs requeridos en consigna
- [ ] Revisar que la implementacion de cpu sea teoricamente correcta:q el saber q instruccion es se haga en decode(por traduccion)[ELIMINAR execute() y poner exec_()en cada if de decode]
- [ ] Refactorizar instrucciones: Escribir los registros con memcpy (el size de escritura se saca con otro diccionario)).
- [ ] Aparece un '%' al final de ALGUNOS'exit' en codigos(aparentemente cuandos son muy largos).la solucion es medio sucia, para la entrega final habría q ver q onda eso
- [ ] Hay que tener cuidado con nuestra planificacion, ya que no planificamos como tal(NUNCA ordenamos las listas) simplemente nos aseguramos de siempre agregar bien(Al final ya que todos usan fifo de alguna forma) esto podria estar mal desde el punto de vista teorico ya que solo se planifica e algunos casos y tambien porque no planificcamos como tal
- [ ] Revisar uso de 'exit(-1);' para cerrar sockets

#### CHECKPOINT 2 ####
- [] -unificar sigterm (un solo if antes del switch?)
- [] desconexion de ios
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