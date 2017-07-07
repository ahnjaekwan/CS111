CC = gcc
CFLAGS = -g -lmraa -lm -lssl -lcrypto
ARCHIVE = lab4c-604057669.tar.gz
SOURCE = lab4c_tcp_tls.c Makefile README

default: build

build: lab4c_tcp_tls.c

ifeq ($(shell uname -r | grep 'edison'),)
	 $(CC) -o lab4c_tcp lab4c_tcp_tls.c $(CFLAGS) -DDUMMY
	 $(CC) -o lab4c_tls lab4c_tcp_tls.c $(CFLAGS) -DDUMMY
else
	 $(CC) -o lab4c_tcp lab4c_tcp_tls.c $(CFLAGS)
	 $(CC) -o lab4c_tls lab4c_tcp_tls.c $(CFLAGS)
endif

dist:
	tar -czvf $(ARCHIVE) $(SOURCE)

clean:
	rm lab4c_tcp lab4c_tls $(ARCHIVE)