all: bin/server bin/client

bin/server: src/server.c
	gcc src/server.c -o bin/server -lpthread

bin/client: src/client.c
	gcc src/client.c -o bin/client

clean:
	rm -fr bin/*

runsrv:
	./bin/server

runclt:
	./bin/client
