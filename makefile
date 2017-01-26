BIN=client server
LDFLAGS=
CFLAGS=-Wall -Werror -g
CC=gcc

all=$(BIN)

client: client.o client_code_etudiant.o
	$(CC) -o client client.o client_code_etudiant.o $(LDFLAGS)

client.o: client.c client_code_etudiant.h
	$(CC) -c client.c $(CFLAGS)

client_code_etudiant.o: client_code_etudiant.c client_code_etudiant.h
	$(CC) -c client_code_etudiant.c $(CFLAGS)

server: server.o server_code_etudiant.o
	$(CC) -o server server.o server_code_etudiant.o $(LDFLAGS)

server.o: server.c server_code_etudiant.h
	$(CC) -c server.c $(CFLAGS)

server_code_etudiant.o: server_code_etudiant.c server_code_etudiant.h
	$(CC) -c server_code_etudiant.c $(CFLAGS)

clean:
	rm -f *.o client server
