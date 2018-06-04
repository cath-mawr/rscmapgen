CFLAGS+=	-std=c99
CFLAGS+=	-g -O0

all: mapdump

mapdump: mapdump.o
	$(CC) $(LDFLAGS) -o mapdump mapdump.o

mapdump.o: mapdump.c
	$(CC) $(CFLAGS) -c -o mapdump.o mapdump.c

clean:
	rm -f mapdump.o mapdump
