all: cclient server

cclient:
	gcc -Wall -pedantic myclient.c testing.c -o cclient

server:
	gcc -Wall -pedantic myserver.c testing.c -o server

clean:
	rm server cclient
