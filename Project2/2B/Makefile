CC = gcc
ARCHIVE = lab2b-604057669.tar.gz
SOURCE = lab2_list.c SortedList.c SortedList.h Makefile lab2b_list.csv lab2b_1.png lab2b_2.png lab2b_3.png lab2b_4.png lab2b_5.png profile.out lab2b_test.sh profile.sh lab2b.gp README

default: build

build:
	@$(CC) -g -lpthread lab2_list.c SortedList.c -o lab2_list

tests:
	touch lab2b_list.csv
	bash ./lab2b_test.sh

profile:
	bash profile.sh

graphs:
	gnuplot lab2b.gp

dist:
	tar -czvf $(ARCHIVE) $(SOURCE)

clean:
	rm lab2_list
	rm lab2b_list.csv
	rm profile.out
	rm lab2b_1.png
	rm lab2b_2.png
	rm lab2b_3.png
	rm lab2b_4.png
	rm lab2b_5.png
	rm $(ARCHIVE)
