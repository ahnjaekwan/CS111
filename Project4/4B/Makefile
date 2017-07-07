CC = gcc
CFLAGS = -g -lmraa -lm
ARCHIVE = lab4b-604057669.tar.gz
SOURCE = lab4b.c Makefile README testscript.sh

default: build

build:
	@$(CC) $(CFLAGS) lab4b.c -o lab4b

check:
	bash testscript.sh

dist:
	tar -czvf $(ARCHIVE) $(SOURCE)

clean:
	rm lab4b $(ARCHIVE)