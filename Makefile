CC=cc

BANDERAS_C=-c -Wall --pedantic-errors -Iinclude
BANDERAS_GDB=-O0 -g3 -DDEBUG
FUENTES=main.c canal.c debug.c
OBJETOS=$(FUENTES:.c=.o)
EJECUTABLE=maximo

$(EJECUTABLE): $(OBJETOS)
	$(CC) $(OBJETOS) -o $@

.c.o:
	$(CC) $(BANDERAS_C) $(BANDERAS_GDB) $<

clean:
	$(RM) $(EJECUTABLE) $(OBJETOS)


