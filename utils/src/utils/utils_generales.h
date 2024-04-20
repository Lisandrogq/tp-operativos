#ifndef UTILS_HELLO_H_
#define UTILS_HELLO_H_

#include <stdlib.h>
#include <stdio.h>
#define PUERTO_KERNEL "4444"
#define PUERTO_CPU "4445"
#define PUERTO_MEMORIA "4446"
/**
* @fn    decir_hola
* @brief Imprime un saludo al nombre que se pase por parámetro por consola.
*/
void decir_hola(char* quien);

#endif
