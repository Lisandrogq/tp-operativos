## CHECKPOINT 2 ##
- [X] Funcion crear_estructuras_administrativas
- [X] recibir estructuras_administrativas en memoria
- [X] Terminar de implementar consola(y conexion con memoria para ejecutar Comandos)
- [ ] Implementar envio de instrucciones a cpu desde memoria.
- [ ] Pensar planning

- [ ] Revisar(cambiar los tipos de 'PID' de int a U_int_X)
- [ ] Pensar check interrupt: tiene acceso al socket de dispatch e interrupt? hace un recv no bloqueante para chequear intrs?
- [ ] Revisar si hace falta un hilo EN KERNEL que solo este en rcv para escuchar las solicitudes a IO que hace CPU.
- [ ] Revisar instrucciones con pocos parametros recibiendo cualquier cosa en los ultimosp parametros
## CHECKPOINT 3 ##
- [ ] Refactorizar instrucciones: Escribir los registros con memcpy (el size de escritura dependería del registro mandado).