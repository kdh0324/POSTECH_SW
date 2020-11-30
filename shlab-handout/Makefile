# Makefile for the CS:APP Shell Lab

SHELL := /bin/bash
TEAM = NOBODY
VERSION = 1
HANDINDIR = /afs/cs/academic/class/15213-f02/L5/handin
DRIVER = ./sdriver.pl
TSH = ./tsh
TSHREF = ./tshref
TSHARGS = "-p"
CC = gcc
CFLAGS = -Wall -O2
FILES = $(TSH) ./myspin ./mysplit ./mystop ./myint

all: $(FILES)

##################
# Handin your work
##################
handin:
	cp tsh.c $(HANDINDIR)/$(TEAM)-$(VERSION)-tsh.c


##################
# Regression tests
##################

# Run tests using the student's shell program
test01:
	$(DRIVER) -t trace01.txt -s $(TSH) -a $(TSHARGS)
test02:
	$(DRIVER) -t trace02.txt -s $(TSH) -a $(TSHARGS)
test03:
	$(DRIVER) -t trace03.txt -s $(TSH) -a $(TSHARGS)
test04:
	$(DRIVER) -t trace04.txt -s $(TSH) -a $(TSHARGS)
test05:
	$(DRIVER) -t trace05.txt -s $(TSH) -a $(TSHARGS)
test06:
	$(DRIVER) -t trace06.txt -s $(TSH) -a $(TSHARGS)
test07:
	$(DRIVER) -t trace07.txt -s $(TSH) -a $(TSHARGS)
test08:
	$(DRIVER) -t trace08.txt -s $(TSH) -a $(TSHARGS)
test09:
	$(DRIVER) -t trace09.txt -s $(TSH) -a $(TSHARGS)
test10:
	$(DRIVER) -t trace10.txt -s $(TSH) -a $(TSHARGS)
test11:
	$(DRIVER) -t trace11.txt -s $(TSH) -a $(TSHARGS)
test12:
	$(DRIVER) -t trace12.txt -s $(TSH) -a $(TSHARGS)
test13:
	$(DRIVER) -t trace13.txt -s $(TSH) -a $(TSHARGS)
test14:
	$(DRIVER) -t trace14.txt -s $(TSH) -a $(TSHARGS)
test15:
	$(DRIVER) -t trace15.txt -s $(TSH) -a $(TSHARGS)
test16:
	$(DRIVER) -t trace16.txt -s $(TSH) -a $(TSHARGS)

# Run the tests using the reference shell program
rtest01:
	$(DRIVER) -t trace01.txt -s $(TSHREF) -a $(TSHARGS)
rtest02:
	$(DRIVER) -t trace02.txt -s $(TSHREF) -a $(TSHARGS)
rtest03:
	$(DRIVER) -t trace03.txt -s $(TSHREF) -a $(TSHARGS)
rtest04:
	$(DRIVER) -t trace04.txt -s $(TSHREF) -a $(TSHARGS)
rtest05:
	$(DRIVER) -t trace05.txt -s $(TSHREF) -a $(TSHARGS)
rtest06:
	$(DRIVER) -t trace06.txt -s $(TSHREF) -a $(TSHARGS)
rtest07:
	$(DRIVER) -t trace07.txt -s $(TSHREF) -a $(TSHARGS)
rtest08:
	$(DRIVER) -t trace08.txt -s $(TSHREF) -a $(TSHARGS)
rtest09:
	$(DRIVER) -t trace09.txt -s $(TSHREF) -a $(TSHARGS)
rtest10:
	$(DRIVER) -t trace10.txt -s $(TSHREF) -a $(TSHARGS)
rtest11:
	$(DRIVER) -t trace11.txt -s $(TSHREF) -a $(TSHARGS)
rtest12:
	$(DRIVER) -t trace12.txt -s $(TSHREF) -a $(TSHARGS)
rtest13:
	$(DRIVER) -t trace13.txt -s $(TSHREF) -a $(TSHARGS)
rtest14:
	$(DRIVER) -t trace14.txt -s $(TSHREF) -a $(TSHARGS)
rtest15:
	$(DRIVER) -t trace15.txt -s $(TSHREF) -a $(TSHARGS)
rtest16:
	$(DRIVER) -t trace16.txt -s $(TSHREF) -a $(TSHARGS)

# Compare the tests between student's & reference shell program
PRINT = $(shell printf 'trace%02d is wrong.' $(number))
NUMBER = 01 02 03 04 05 06 07 08 09 10 11 12 13 14 15 16
test:
	@wrong = 0; \
	for i in $(NUMBER); do \
		chmod +x sdriver.pl; \
		echo Test trace$$i...; \
		make rtest$$i > expected; \
		make test$$i > actual; \
		sed -i 's/[0-9]*//g' expected; \
		sed -i 's/[0-9]*//g' actual; \
		diff expected actual > result; \
		sed -i '1d' result; \
		sed -i '/---/d' result; \
		sed -i '/sdriver/d' result; \
		sed -i '/tsh/d' result; \
		if [ -s "result" ]; then \
			let "wrong += 1"; \
			echo Failed; \
		else \
			echo Succeed; \
		fi; \
		rm expected actual; \
    done; \
	echo "WRONG: $$wrong"; \
	rm result;

# clean up
clean:
	rm -f $(FILES) *.o *~
