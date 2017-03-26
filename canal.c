#include <canal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

struct _canal
{
  int solicitud[2];
  int respuesta[2];
};

/* ------------------------------ canal_crear ------------------------------ */
int canal_crear(canal *c)
{
  int   error;
  canal cnx;

  *c  = (canal)0;
  cnx = (canal)malloc(sizeof(struct _canal));

  if ((error = pipe(cnx->solicitud)) == -1)
  {
    perror("No se puede crear el canal para enviar solicitudes al servidor");
    return error;
  }

  if ((error = pipe(cnx->respuesta)) == -1)
  {
    perror("No se puede crear el canal para recibir respuestas al cliente");
    return error;
  }

  *c = cnx;

  return 0;
}

int canal_destruir(canal *con)
{
  canal conexion = *con;

  if (conexion)
  {
    close(conexion->solicitud[RECIBIR]);
    close(conexion->solicitud[ENVIAR]);
    close(conexion->respuesta[RECIBIR]);
    close(conexion->respuesta[ENVIAR]);

    free(conexion);
    *con = (canal)0;
  }

  return 0;
}

int solicitar(canal conexion, buffer mensaje)
{
  int bytes;
  if (!conexion)
  {
    perror("CLIENTE: No hay conexion hacia el servidor");
    return -1;
  }

  bytes = write(conexion->solicitud[ENVIAR], mensaje, TAM_BLOQUE);

  if (bytes > 0)
  {
    printf("Se han enviado %d bytes al servidor\n", bytes);
    do {
      bytes = read(conexion->respuesta[RECIBIR], mensaje, TAM_BLOQUE);
    } while (bytes == 0);

    return 0;
  }

  return -1;
}

int responder(canal conexion, buffer mensaje)
{
  int exito;
  if (!conexion)
  {
    perror("SERVIDOR: No hay conexion hacia el servidor");
    return -1;
  }

  exito = !write(conexion->respuesta[ENVIAR], mensaje, TAM_BLOQUE);

  return exito;
}

int atender(canal conexion, buffer mensaje)
{
  int exito;
  if (!conexion)
  {
    perror("SERVIDOR: No hay conexion hacia el servidor");
    return -1;
  }

  exito = !read(conexion->solicitud[RECIBIR], mensaje, TAM_BLOQUE);

  return exito;
}
