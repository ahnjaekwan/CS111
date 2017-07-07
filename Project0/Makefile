CC = gcc
CFLAGS = -g -O2
ARCHIVE = lab0-604057669.tar.gz
SOURCE = lab0.c Makefile backtrace.png breakpoint.png README input.txt

default:
	@$(CC) $(CFLAGS) -o lab0 lab0.c

check: clean default check-smoke dist

check-smoke:
	@./lab0 < input.txt > output.txt ; \
	if [ $$? -ne 0 ] ; \
	then \
		echo "Exit code is incorrect. It should be 0." ; \
		exit 1 ; \
	fi
	@rm -f output.txt

	@./lab0 --input=input.txt > output.txt ; \
	if [ $$? -ne 0 ] ; \
	then \
		echo "Exit code is incorrect. It should be 0." ; \
		exit 1 ; \
	fi
	@rm -f output.txt

	@./lab0 --output=output.txt < input.txt ; \
	if [ $$? -ne 0 ] ; \
	then \
		echo "Exit code is incorrect. It should be 0." ; \
		exit 1 ; \
	fi
	@rm -f output.txt

	@./lab0 --input=input.txt --output=output.txt ; \
	if [ $$? -ne 0 ] ; \
	then \
		echo "Exit code is incorrect. It should be 0." ; \
		exit 1 ; \
	fi
	@rm -f output.txt

	@./lab0 --exitcode1test 2> /dev/null ; \
	if [ $$? -ne 1 ] ; \
	then \
		echo "Exit code is incorrect. It should be 1." ; \
		exit 1 ; \
	fi

	@./lab0 --input=noinput.txt > output.txt 2> /dev/null ; \
	if [ $$? -ne 2 ] ; \
	then \
		echo "Exit code is incorrect. It should be 2." ; \
		exit 1 ; \
	fi

	@echo "Exit code 3 checking..." > output.txt
	@chmod -w output.txt
	@./lab0 --output=output.txt < input.txt 2> /dev/null ; \
	if [ $$? -ne 3 ] ; \
	then \
		echo "Exit code is incorrect. It should be 3." ; \
		exit 1 ; \
	fi
	@rm -f output.txt

	@./lab0 --segfault --catch 2> /dev/null ; \
	if [ $$? -ne 4 ] ; \
	then \
		echo "Exit code is incorrect. It should be 4." ; \
		exit 1 ; \
	fi

	@echo "Every test cases are succesfully passed"

clean:
	@rm -f lab0

dist:
	@tar -czf $(ARCHIVE) $(SOURCE)
