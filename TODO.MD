# BUGS #
- [x] SI BAJAS UNA IO Y VOLVES A PEDIR IO, SEGFAULT!
- [x] si mantenes memoría y bajaste los otros modulos,cuando volves a iniciar_proceso, tira segmentation fault memoria pq no se recibe el path.
- [x] BREAKPOINTS PUEDEn ROMPER MEMORIA(de alguna forma el codigo esta vacio)
- [x] hacer issue: si se finaliza un proceso justo cuando sale por sysscall, se borran las estructuras administrativas y no se mando a exit(capaz la 'solucion' es un mutex para exec)
- [x] probar  bien todos los casos especiales de planificacion + finalizar_proceso
- [ ] ARREGLAR LEAKS
- [ ] TERMINAR DE AGREGAR MUTEX
- [ ] unificar los tipos de  de int a U_int_X ***en vm int 32 == int, capaz en el labo la pasamos mal***
- [x]**SI SE EJECUTA EL CODIGO DE'res'(dos resize seguidos) SE ROMPE MEMORIA**(Hay algun mal manejo de memoria previo)
____
#### CHECKPOINT 3 #######
- [ ] !!Agregar mutex cuando sea necesario(sockets,listas bloqueados,etc)

#### ENTREGA FINAL #######
- [ ] Agregar logs requeridos en consigna
- [ ] Agregar retardos donde corresponda
- [ ] Aparece un '%' al final de ALGUNOS'exit' en codigos(aparentemente cuandos son muy largos).la solucion es medio sucia, para la entrega final habría q ver q onda eso
- [ ] Invalidar entradas de tlb al hacer resize achicando #4112
- [ ] IMPLEMENTAR DESCONEXION DE ios
- [ ] cuando sacamos el thread interrupt(cpu) no nos dimos cuenta q el handshake no se esta haciendo creo(no genera problemas pero revisar)
- [x] TLB (LRU)
- [x] DETENER_PLANIFICACION, en principo un solo semaforo(en total) que bloquee las cosas.
- [X] FS CREATE
- [X] FS DELETE
- [x] FS WRITE
- [x] FS READ
- [x] FS TRUNCATE
- [x] FS COMPACTACION 💀💀💀
- [x] EL PLANIFICADOR de largo plazo debería esperar a que la memoria envié el okey de que se creo el proceso antes de poder pasar de New a Ready ( revisar respuesta del issue #4050 sobre bloqueo de planificador largo)


#### REVISAR #####
* revisar multiprog, probablemente no ande el caso:
mando 30 procesos, multiprog en 20, cambio a 17, cambio a 15, cambio a 12 ....
va  a suceder que algunos de los signals de exit caen en el wait del while hilo largo plazo haciendo que se manden a ready nuevos procesos cuando el grado de programacion no lo permite
Posible solucion: meter en hilo largo plazo un if (grado de multiprog cambiado){wait(reanudar)}
La solucion que decía el issue me hace ruido, es raro que proceso que terminen no hagan signal(deben marcar que terminaron)

* Revisar que la implementacion de cpu sea teoricamente correcta:q el saber q instruccion es se haga en decode(por traduccion)[ELIMINAR execute() y poner exec_()en cada if de decode]


* Revisar uso de 'exit(-1);' para cerrar sockets 
 


 
