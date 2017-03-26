#ifndef __CANAL_H__
#define __CANAL_H__
#include <string.h>

#define TAM_BLOQUE 4096
#define RECIBIR 0
#define ENVIAR  1

typedef struct _canal* canal;

typedef char buffer[TAM_BLOQUE];

#define buffer_limpiar(buf) memset((buf), 0, TAM_BLOQUE)

int canal_crear(canal *c);
int canal_destruir(canal *c);
int solicitar(canal conexion, buffer mensaje);
int responder(canal conexion, buffer mensaje);
int atender(canal conexion, buffer mensaje);

#endif/*__CANAL_H__*/
