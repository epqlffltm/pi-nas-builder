#makefile

CC=gcc
CFLAGS=
OBJS=setup.o
LIBS=
app:setup
setup : $(OBJS)
	$(CC) $(CFLAGS) -o setup $(OBJS) $(LIBS)

setup.o : setup.c
	$(CC) $(CFLAGS) -c setup.c

clean:
	rm -f $(OBJS) setup core