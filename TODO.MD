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
- [x]**SI SE EJECUTA EL CODIGO DE'res'(dos resize seguidos) SE ROMPE MEMORIA**(Hay algun mal manejo de memoria previo)
____
#### CHECKPOINT 3 #######
- [ ] !!Agregar mutex cuando sea necesario(sockets,listas bloqueados,etc)

#### ENTREGA FINAL #######
- [ ] IMPLEMENTAR DESCONEXION DE ios
- [x] TLB (LRU)
- [x] DETENER_PLANIFICACION, en principo un solo semaforo(en total) que bloquee las cosas.
- [X] FS CREATE
- [X] FS DELETE
- [x] FS WRITE
- [x] FS READ
- [x] FS TRUNCATE
- [x] FS COMPACTACION 💀💀💀
- [ ] !!!REVISAR EL codigo 'esto_crashea_memoria', desaparece la tabla de paginas 
- [ ] Agregar logs requeridos en consigna
- [ ] Agregar retardos donde corresponda
- [ ] Aparece un '%' al final de ALGUNOS'exit' en codigos(aparentemente cuandos son muy largos).la solucion es medio sucia, para la entrega final habría q ver q onda eso
- [x] EL PLANIFICADOR de largo plazo debería esperar a que la memoria envié el okey de que se creo el proceso antes de poder pasar de New a Ready ( revisar respuesta del issue #4050 sobre bloqueo de planificador largo)


#### REVISAR #####
* revisar multiprog, probablemente no ande el caso:
mando 30 procesos, multiprog en 20, cambio a 17, cambio a 15, cambio a 12 ....
va  a suceder que algunos de los signals de exit caen en el wait del while hilo largo plazo haciendo que se manden a ready nuevos procesos cuando el grado de programacion no lo permite
Posible solucion: meter en hilo largo plazo un if (grado de multiprog cambiado){wait(reanudar)}
La solucion que decía el issue me hace ruido, es raro que proceso que terminen no hagan signal(deben marcar que terminaron)

* Revisar que la implementacion de cpu sea teoricamente correcta:q el saber q instruccion es se haga en decode(por traduccion)[ELIMINAR execute() y poner exec_()en cada if de decode]

* Revisar donde se suma el pc, antes de exec o dsp  de exec, si es despues, un set pc 3 terminaría siendo un set pc 4

* Hay que tener cuidado con nuestra planificacion, ya que no planificamos como tal(NUNCA ordenamos las listas) simplemente nos aseguramos de siempre agregar bien(Al final ya que todos usan fifo de alguna forma) esto podria estar mal desde el punto de vista teorico ya que solo se planifica e algunos casos y tambien porque no planificcamos como tal **PREGUNTARLE A CARRI**

* Revisar uso de 'exit(-1);' para cerrar sockets 
 


 
