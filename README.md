# 08. Depuración: cómo depurar programas multiproceso

![](https://codingornot.files.wordpress.com/2017/03/debug1.png)

Con lo visto hasta este punto de la serie, hemos  cubierto  herramientas  suficientes
para que explotes las capacidades que **GDB** te ofrece para  encontrar  defectos  en
tus programas. En esta ocasión te mostraré cómo utilizar el depurador de **GNU** para
examinar aplicaciones cliente-servidor.  Las  recetas  que  te  presento  pueden  ser
adaptadas para otros escenarios (como aplicaciones multihilo o que  se  comuniquen  a
través de *sockets*).

Utilizaré como ejemplo una versión innecesariamente complicada  de  nuestro  programa
`maximo` con el que trabajamos en la [nota sobre *watchpoints*][1]. Para refrescar la
memoria te diré que en ese programa la idea es, dada una lista con números, encontrar
cuál de ellos es el mayor.

	int maximo(int *numeros)
	{
		int maxv = -1;
		int tam;
		int i;

		/* El primer elemento indica el número de argumentos. */
		tam = *numeros++;

		for (i = 0; i < tam; i++)
			if (maxv < numeros[i])
				maxv = numeros[i];

		return maxv;
	}

En esta ocasión modifiqué el programa para que se ejecute por medio de dos  procesos,
un cliente que le pregunte al usuario los números que enviará a través de un  mensaje
al servidor que es el proceso que realmente ejecutará  la  función  `maximo()`.   Por
ahora voy a obviar la implementación del canal de comunicación entre el cliente y  su
servidor, puesto que no lo necesitamos para nuestro  ejemplo.   De  todos  modos,  el
siguiente diagrama te dará un panorama general de cómo funciona este programa:

	       cliente
	          |     fork()
	          +-------------------> servidor
	          |                        |
	          |                        . atender()
	<entrada> |                        . <espera>
	          |     solicitar()        .
	          `----------------------> *
	<espera>  .                        | <procesa>
	          .     responder()        |
	          * <----------------------'
	          |                        |
	          |                        |
	          v                        v

## Funcionamiento del programa

Al compilar el programa, se generará un binario llamado `maximo`;  al  ejecutarlo  se
despliega un *prompt* muy simple (`?`), que le pregunta al usuario lo que el programa
debe hacer a continuación.

El cliente solo reconoce tres palabras: `maximo`,  `enviar`  y  `salir`.   Cuando  el
programa es invocado, inmediatamente lanza un segundo proceso en  segundo  plano  (el
servidor) y establece comunicación con este.  Una vez que el servidor está listo,  el
programa original jugará el papel de cliente, esperando las órdenes del usuario .  Al
ingresar la orden `maximo` el *prompt* cambia de forma  (`>`)  para  indicar  que  ya
puedes ingresar los números, el cliente acepta un número  por  línea.   Cuando  hayas
ingresado suficientes, escribe `enviar`  para  que  el  cliente  elabore  el  mensaje
correspondiente y lo mande al servidor.  Cuando  el  servidor  conteste,  el  cliente
desplegará su respuesta y el *prompt*  recuperará  su  forma  original  (`?`).   Para
terminar la sesión, escribe la orden `salir`.

	➜ maximo
	? maximo
	> 4
	> 5
	> 0
	> 1
	> enviar
	El valor máximo es 5
	? salir
	El servidor se ha apagado
	➜ 

## El código fuente

Dado que, como mencioné al inicio, el programa es innecesariamente largo y resultaría
molesto ponerlo todo aquí, por eso lo he  publicado  completo  en  [GitHub][2].   Las
partes que nos interesan para esta nota están en `main.c`  e  `include/debug.h`.   Si
quieres echarle un vistazo solamente, puedes consultarlo [aquí][3] y para obtener  el
código y jugar un rato con él puedes descargarlo directamente en tu terminal: 

	➜ git clone https://github.com/codingornot/depuracion-cliente-servidor.git
	Cloning into 'depuracion-cliente-servidor'...
	...
	Checking connectivity... done.
	➜ cd depuracion-cliente-servidor
	➜ make
	cc -c -Wall --pedantic-errors -Iinclude -O0 -g3 -DDEBUG main.c
	cc -c -Wall --pedantic-errors -Iinclude -O0 -g3 -DDEBUG canal.c
	cc main.o canal.o -o maximo
	➜

## Depurando el programa

A partir de este momento ya podemos depurar el programa.  Lo primero que viene  a  la
mente  para depurar el programa es utilizar **GDB**  igual  que  siempre,  pero  esto
presenta dos pequeños inconvenientes.  El primero  es  que  al  iniciar  el  programa
mediante la línea de comandos `gdb maximo` la terminal quedará asociada  tanto  a  la
sesión de **GDB** como a la del programa `maximo` (recuerda que este  pide  datos  al
usuario mediante  la  entrada  estándar),  sería  más  conveniente  tener  terminales
separadas para cada una de estas sesiones. El segundo inconveniente es que al iniciar
el programa de esta forma, el depurador solamente puede examinar el programa cliente,
el programa servidor sigue en segundo plano.

Aquí es donde viene la primera  diferencia  con  la  forma  en  que  hemos  usado  el
depurador hasta el momento: los puntos de quiebre están insertados directamente en el
código fuente mediante la macro `breakpoint` que está definida en  `include/debug.h`.
Podríamos definir los puntos de quiebre utilizando el comando `break` de  **GDB**  ya
sea escribiéndolos en la sesión interactiva, o haciendo  que  el  depurador  los  lea
desde un archivo de texto, pero a estas alturas resultaría aburrido. Además de que en
este caso es más práctico incluirlos en el programa.  El archivo `include\debug.h` se
ve así:

	#ifndef __DEBUG_H__
	#define __DEBUG_H__

	#include <stdio.h>
	#include <signal.h>
	#include <sys/types.h>
	#include <unistd.h>

	#define _breakpoint(file, line) \
	do { int pid = getpid(); \
	printf("El proceso %d alcanzó un breakpoint en %s:%d\n", \
				 pid, (file), (line));\
	raise(SIGINT);\
	} while(0)

	#define breakpoint _breakpoint(__FILE__, __LINE__)

	#endif/*__DEBUG_H__*/

Esta macro puede tener problemas de  portabilidad,  pero  funcionará  si  tu  sistema
operativo no es Windows.  Podemos hacer referencia a  `include/debug.h`  en  `main.c`
así: `#include <debug.h>`, gracias a  que  entre  las  banderas  que  le  pasamos  al
compilador incluimos `-Iinclude`.   El  siguiente  es  un  fragmento  de  la  función
`cliente()` en donde se utiliza esta macro:

	if (leidos > 0)
	{
	  msj->tipo     = SOLICITUD;
	  msj->cantidad = leidos;
	
	  breakpoint; /* <-- Este es el breakpoint */
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

Bueno, ahora vamos a depurar ambos procesos a la vez, para ello necesitas abrir  tres
terminales, en una vas a invocar el programa `maximo` que llamaremos "consola"  y  en
cada una de las otras dos vas a iniciar una sesión  de  **GDB**,  a  las  cuales  nos
referiremos a partir de ahora como "cliente" y "servidor" según el proceso  que  esté
examinando cada una. Manos a la obra:

En *consola* ejecuta:

	➜ maximo
	?    

Eso iniciará el programa que  tomará  el  papel  de  proceso  cliente  y  lanzará  el
proceso servidor en segundo plano. Luego ve a *cliente* y dentro de **GDB** ejecuta:

	(gdb) !pgrep maximo
	23927
	23928
	(gdb)

El  comando [`pgrep`][4] es  una  herramienta  externa  al  depurador,  por  ello  le
anteponemos un signo de admiración, lo que hace es buscar en la  lista  de  procesos,
aquellos cuyo nombre coincida con la palabra que le pasamos  como  argumento  y  como
salida imprime los identificadores de los  procesos  que  encuentre.   En  este  caso
encuentra dos: uno es el programa que lanzamos al escribir `maximo` en *consola* y el
segundo corresponde al proceso lanzado mediante la llamada a sistema [`fork()`][5].

Con estos datos es momento de  asociar  *cliente*  y  *servidor*  a  sus  respectivos
procesos; esa tarea la realizaremos utilizando otro comando de **GDB**: `attach`.

En *cliente*:

	(gdb) attach 23927
	...
	Loaded symbols for /lib64/ld-linux-x86-64.so.2
	0x00007f7c9d7c7ba0 in __read_nocancel () at ../sysdeps/unix/syscall-template.S:81
	81	../sysdeps/unix/syscall-template.S: No existe el fichero o el directorio.
	(gdb)

Y en *servidor*:

	(gdb) attach 23928
	...
	Loaded symbols for /lib64/ld-linux-x86-64.so.2
	0x00007f7c9d7c7ba0 in __read_nocancel () at ../sysdeps/unix/syscall-template.S:81
	81	../sysdeps/unix/syscall-template.S: No existe el fichero o el directorio.
	(gdb)

Luego de eso utiliza el comando `continue` tanto en *cliente* como en *servidor*:

	(gdb) continue
	Continuing.

Y listo, ya podemos iniciar la sesión.  Vuelve a *consola*  e  ingresa  unos  números
siguiendo las reglas que te describí al inicio.

En *consola*:

	? maximo
	> 5
	> 4
	> 10
	> 2
	> 8
	>

Puedes alimentar el programa con muchos números, si a estas alturas ya viste todo  el
código fuente sabes que el programa no  soporta  más  de  1022  números,  parece  una
cantidad arbitraria, pero no lo es tanto, el tamaño total de  cada  mensaje  contando
los primeros dos miembros (`tipo` y `cantidad`) es 1024 elementos de tipo `int`, 1024
es un número "redondo" en términos de potencias de dos.  Cuando  hayas  terminado  de
escribir los datos de entrada, escribe la palabra "enviar" y presiona `[enter]`, esto
hará que *cliente* alcance su punto de quiebre.  Verás este evento reflejado  también
en *consola*.

En *consola*:

	> enviar
	El proceso 23927 alcanzó un breakpoint en main.c:141

En *cliente*:

	Continuing.
			
	Program received signal SIGINT, Interrupt.
	0x00007f7c9d721067 in __GI_raise (sig=2) at ../nptl/sysdeps/unix/sysv/linux/raise.c:56
	56	../nptl/sysdeps/unix/sysv/linux/raise.c: No existe el fichero o el directorio.
	(gdb) next
	57	in ../nptl/sysdeps/unix/sysv/linux/raise.c
	(gdb) next
	cliente (conexion=0x17d7010) at main.c:142
	142	        if (!(error = solicitar(conexion, buf_mensaje)))
	(gdb) bt
	#0  cliente (conexion=0x17d7010) at main.c:142
	#1  0x0000000000400a26 in main (argc=1, argv=0x7ffda03f5f08) at main.c:78
	(gdb) 

Ya estamos examinando a *cliente* justo antes de enviar el mensaje que ha construido
a partir de la entrada del usuario. De hecho puedes visualizar el mensaje.

En *cliente*:

	(gdb) backtrace
	#0  cliente (conexion=0x17d7010) at main.c:142
	#1  0x0000000000400a26 in main (argc=1, argv=0x7ffda03f5f08) at main.c:78
	(gdb) print *msj
	$1 = {tipo = 1, cantidad = 5, numeros = {5, 4, 10, 2, 8, 0 <repeats 1017 times>}}
	(gdb) list
	137	      {
	138	        msj->tipo     = SOLICITUD;
	139	        msj->cantidad = leidos;
	140	
	141	        breakpoint;
	142	        if (!(error = solicitar(conexion, buf_mensaje)))
	143	        {
	144	          if ((RESPUESTA_OK == msj->tipo))
	145	          {
	146	            int max = msj->numeros[0];
	(gdb) 

Si avanzas  una  instrucción  desde  este  punto,  *cliente*  llamará  a  la  función
`solicitar()`, la cual envía el mensaje a *servidor*.

En *cliente*:

	(gdb) next

En *servidor*:

	Continuing.

	Program received signal SIGINT, Interrupt.
	0x00007f7c9d721067 in __GI_raise (sig=2) at ../nptl/sysdeps/unix/sysv/linux/raise.c:56
	56	../nptl/sysdeps/unix/sysv/linux/raise.c: No existe el fichero o el directorio.
	(gdb) next
	57	in ../nptl/sysdeps/unix/sysv/linux/raise.c
	(gdb) next
	servidor (conexion=0x17d7010) at main.c:221
	221	    switch(msj->tipo)
	(gdb) backtrace
	#0  servidor (conexion=0x17d7010) at main.c:221
	#1  0x0000000000400a15 in main (argc=1, argv=0x7ffda03f5f08) at main.c:75
	(gdb) list
	216	  {
	217	
	218	    buffer_limpiar(salida);
	219	    breakpoint;
	220	
	221	    switch(msj->tipo)
	222	    {
	223	    case SOLICITUD:
	224	      entrada = (int *)&msj->cantidad;
	225	      max = maximo(entrada);
	(gdb) print *msj
	$1 = {tipo = 1, cantidad = 5, numeros = {5, 4, 10, 2, 8, 0 <repeats 1017 times>}}
	(gdb) 

Como puedes ver, *servidor*  ha  recibido  correctamente	el	mensaje  que	le	envió
*cliente* y se dispone a procesarlo.	Mientras esto pasa, *cliente*  sigue	bloqueado
esperando la respuesta de *servidor*.		Si	dejamos  que	*servidor*	siga	adelante,
recuperaremos el control de *cliente* ya con la respuesta.

En *servidor*:

	(gdb) continue
	Continuing.

En *cliente*:

	144	          if ((RESPUESTA_OK == msj->tipo))
	(gdb) print *msj
	$2 = {tipo = 3, cantidad = 0, numeros = {10, 0 <repeats 1021 times>}}
	(gdb) continue
	Continuing.

Ahora el *cliente* ha desplegado el resultado en *consola* y ambos procesos están en
espera de que ingreses una nueva orden.

En *consola*:

	> enviar
	El proceso 23927 alcanzó un breakpoint en main.c:141
	El proceso 23928 alcanzó un breakpoint en main.c:219
	El valor máximo es 10
	? salir
	El proceso 23928 alcanzó un breakpoint en main.c:219
	El servidor se ha apagado
	➜

Y eso es todo, ya tienes todos los elementos para diagnosticar	incluso  aplicaciones
multiproceso o multihilo utilizando **GDB**.	Me parece que este es buen momento para
hacer una pausa en este tema.  Seguro tienes mucho que practicar y explorar.	Lo	que
hasta ahora te he presentado no son más que  las	bases  necesarias  para  empezar	a
utilizar esta herramienta, y por supuesto que las cosas que has aprendido a lo	largo
de estas notas son de utilidad para depuradores de otros lenguajes, así que cuando te
encuentres con otro depurador, ten la confianza de	aplicar  estos	conceptos  y	(en
muchos casos) los comandos que aquí has aprendido. ¡Hasta la próxima!

[1]: https://codingornot.com/2017/01/11/03-depuracion-watchpoints/
[2]: https://github.com
[3]: https://github.com/codingornot/depuracion-cliente-servidor
[4]: https://linux.die.net/man/1/pgrep
[5]: https://linux.die.net/man/2/fork

