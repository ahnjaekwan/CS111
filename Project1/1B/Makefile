CC = gcc
ARCHIVE = lab1b-604057669.tar.gz
SOURCE = lab1b-client.c lab1b-server.c my.key Makefile README

default:
	@$(CC) -lmcrypt lab1b-client.c -o client
	@$(CC) -lmcrypt lab1b-server.c -o server

check: clean default dist

clean:
	@rm -f server client

dist:
	@tar -czf $(ARCHIVE) $(SOURCE)
