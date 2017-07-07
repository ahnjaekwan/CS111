CC = gcc
ARCHIVE = lab2a-604057669.tar.gz
SOURCE = lab2_add.c SortedList.c SortedList.h lab2_list.c Makefile lab2_add.csv lab2_list.csv lab2_add-1.png lab2_add-2.png lab2_add-3.png lab2_add-4.png lab2_add-5.png lab2_list-1.png lab2_list-2.png lab2_list-3.png lab2_list-4.png lab2_add.gp lab2_list.gp lab2a_add_test.sh lab2a_list_test.sh README

default: build

build: lab2a_add lab2a_list

lab2a_add:
	@$(CC) -pthread -o lab2_add lab2_add.c

lab2a_list:
	@$(CC) -pthread -o lab2_list lab2_list.c

tests:
	touch lab2_list-1.png lab2_list-2.png lab2_list-3.png lab2_list-4.png
	bash ./lab2a_add_test.sh
	bash ./lab2a_list_test.sh

graphs:
	gnuplot lab2_add.gp
	gnuplot lab2_list.gp

clean:
	rm $(ARCHIVE)
	rm lab2_add.csv lab2_list.csv
	rm lab2_add-1.png lab2_add-2.png lab2_add-3.png lab2_add-4.png lab2_add-5.png
	rm lab2_list-1.png lab2_list-2.png lab2_list-3.png lab2_list-4.png
	rm lab2_add lab2_list

dist:
	@tar -czf $(ARCHIVE) $(SOURCE)
