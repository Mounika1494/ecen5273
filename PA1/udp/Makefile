INCLUDE_DIRS = 
LIB_DIRS = 

CDEFS= 
CFLAGS= -O0 -g $(INCLUDE_DIRS) $(CDEFS)
LIBS=

PRODUCT=client server

HFILES=
CFILES= udp_client.c udp_server.c

SRCS= ${HFILES} ${CFILES}
OBJS= ${CFILES:.c=.o}

all:	${PRODUCT}

clean:
	-rm -f *.o *.NEW *~ *.d
	-rm -f ${PRODUCT} ${GARBAGE}

client:	udp_client.o
	$(CC) $(LDFLAGS) $(CFLAGS) -o $@ udp_client.o $(LIBS)

server:	udp_server.o
	$(CC) $(LDFLAGS) $(CFLAGS) -o $@ udp_server.o $(LIBS)

depend:

.c.o:
	$(CC) $(CFLAGS) -c $<

