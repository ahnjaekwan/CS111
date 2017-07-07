CC = gcc
CFLAGS = -g -O2
ARCHIVE = lab1a-604057669.tar.gz
SOURCE = lab1a.c Makefile README

default:
	@$(CC) $(CFLAGS) -o lab1a lab1a.c

check: clean default dist

clean:
	@rm -f lab1a

dist:
	@tar -czf $(ARCHIVE) $(SOURCE)
