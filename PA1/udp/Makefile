INCLUDE_DIRS = 
LIB_DIRS = 

CDEFS= 
CFLAGS= -O0 -g $(INCLUDE_DIRS) $(CDEFS)
LIBS=

PRODUCT=client

HFILES=
CFILES= udp_client.c 

SRCS= ${HFILES} ${CFILES}
OBJS= ${CFILES:.c=.o}

all:	${PRODUCT}

clean:
	-rm -f *.o *.NEW *~ *.d
	-rm -f ${PRODUCT} ${GARBAGE}

client:	udp_client.o
	$(CC) $(LDFLAGS) $(CFLAGS) -o $@ udp_client.o $(LIBS)

depend:

.c.o:
	$(CC) $(CFLAGS) -c $<

