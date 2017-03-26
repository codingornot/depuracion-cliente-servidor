#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <canal.h>
#include <debug.h>

struct _mensaje
{
  int tipo;
#define ERROR           0
#define SOLICITUD       1
#define SALIR           2
#define RESPUESTA_OK    3
#define RESPUESTA_EOF   4
  int cantidad;
  int numeros[1022];
};
typedef struct _mensaje *mensaje;

int cliente(canal conexion);
int servidor(canal c);
int maximo(int *);

int main(int argc, char *argv[])
{
  canal  conexion;
  int    pid;
  int    error = 0;
  
  if (canal_crear(&conexion) == -1)
  {
    perror("No se pudo crear el canal de comunicacion");
    exit(-1);
  }

  if ((pid = fork()) == -1)
  {
    perror("No se pudo iniciar el servidor");
    exit(-1);
  }
  else if (pid == 0)
    error = servidor(conexion);
  else
  {
    error = cliente(conexion);
  }

  error = canal_destruir(&conexion);

  return error;
}

int cliente(canal conexion)
{
	buffer  buf_mensaje;
	buffer  buf_entrada;
  mensaje msj;
  int     error;
  int     leidos;
  int     continuar;

  msj       = (mensaje)buf_mensaje;
  leidos    = 0;
  error     = 0;
  continuar = 0;

  do {
 
    char palabra[256];

    buffer_limpiar(buf_mensaje);
    buffer_limpiar(buf_entrada);

    memset(palabra, 0, 256);
    printf(" > ");
    scanf("%s", palabra);

    if (strcmp("maximo", palabra) == 0)
    {
      char *fin;
      int   numero;

      while (!error)
      {
        memset(palabra, 0, 256);
        printf(" > ");
        scanf("%s", palabra);

        numero = (int)strtol(palabra, &fin, 10);

        if (strlen(fin) == 0)
          msj->numeros[leidos++] = numero;
        else if (strcmp("enviar", fin) == 0)
          break;
        else
        {
          perror("Solo puedo comparar numeros");
          leidos  =  0;
          error   = -1;
        }
      } 

      if (leidos > 0)
      {
        msj->tipo     = SOLICITUD;
        msj->cantidad = leidos;

        if (!(error = solicitar(conexion, buf_mensaje)))
        {
          if ((RESPUESTA_OK == msj->tipo))
          {
            int max = msj->numeros[0];
            printf("El valor máximo es %d\n", max);
            continuar = 1;
          }
          else
          {
            perror("Error en el servidor al procesar el mensaje");
          }
        }
        else
        {
          perror("Ocurrió un error al enviar el mensaje");
        }
      }
    }
    else if (strcmp("salir", palabra) == 0)
    {
      msj->tipo = SALIR;
      if (!(error = solicitar(conexion, buf_mensaje)))
      {
        if (RESPUESTA_EOF == msj->tipo)
        {
          printf("El servidor se ha apagado\n");
          continuar = 0;
        }
        else if (ERROR == msj->tipo)
        {
          perror("Error en el servidor al procesar el mensaje");
        }
        else
        {
          perror("Error desconocido en el servidor");
        }
      }
      else
      {
        perror("Ocurrió un error al enviar el mensaje");
      }
    }
    else
    {
      perror("Comando no reconocido");
    }
  } while (!error && continuar);
  
  if (error)
    perror("Error al enviar solicitud al servidor");

	return error;
}

int servidor(canal conexion)
{
  buffer  buf;
  buffer  salida;
  mensaje msj;
  int    *entrada;
  int     error;
  int     max;
  int     salir = 0;

  if (!conexion)
  {
    perror("SERVIDOR: No hay conexion");
    return -1;
  }

  msj = (mensaje)buf;

  while (!salir && !(error = atender(conexion, buf)))
  {

    buffer_limpiar(salida);

    switch(msj->tipo)
    {
    case SOLICITUD:
      entrada = (int *)&msj->cantidad;
      max = maximo(entrada);

      buffer_limpiar(buf);
      msj->tipo = RESPUESTA_OK;
      msj->numeros[0] = max;
      break;

    case SALIR:
      buffer_limpiar(buf);
      msj->tipo = RESPUESTA_EOF;
      salir = 1;
      break;

    default:
      buffer_limpiar(buf);
      msj->tipo = ERROR;
      salir = 1;
      break;
    }
    
    error = responder(conexion, buf);
  }

  return error;
}

int maximo(int *numeros)
{
	int i;
	int maxv;
	int tam;

	/* El primer elemento indica el número de argumentos. */
	tam = *numeros++;

	for (i = 0; i < tam; i++)
		if (maxv < numeros[i])
			maxv = numeros[i];

	return maxv;
}

