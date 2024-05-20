## CHECKPOINT 2 ##
- [ ] Funcion crear_estructuras_administrativas
- [ ] recibir estructuras_administrativas en memoria


- [ ] Pensar check interrupt: tiene acceso al socket de dispatch e interrupt? hace un recv no bloqueante para chequear intrs?
- [ ] Implementar envio de instrucciones a cpu desde memoria.
- [ ] Pensar planning
- [ ] Terminar de implementar consola(y conexion con memoria para ejecutar Comandos)
- [ ] Revisar si hace falta un hilo EN KERNEL que solo este en rcv para escuchar las solicitudes a IO que hace CPU.
- [ ] Crear sub carpetas en modulos
## CHECKPOINT 3 ##
- [ ] Refactorizar instrucciones: Escribir los registros con memcpy (el size de escritura dependería del registro mandado).