CFLAGS = -Wall
X = unixsocketclient unixsocketserver

all: $X

unixsocketclient: unixsocketclient.c

unixsocketserver: unixsocketserver.c

clean:
	rm -f $X

