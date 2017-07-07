CC = gcc
ARCHIVE = lab3a-604057669.tar.gz
SOURCE = lab3a.c Makefile README ext2_fs.h

default: build

build:
	@$(CC) -g lab3a.c -lm -o lab3a

dist:
	tar -czvf $(ARCHIVE) $(SOURCE)

clean:
	rm lab3a
	rm $(ARCHIVE)