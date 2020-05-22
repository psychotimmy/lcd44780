#
# Makefile for the HD44780U I2C LCD library and samples.
# (C) Tim Holyoake, 10th May 2020.
# Typing 'make' will create the library and sample programs
#

CC = gcc
RM = rm
CFLAGS = -Wall -lpigpiod_if2

default: lcd44780test

lcd44780.a: lcd44780.o
	ar -crs lcd44780.a lcd44780.o

lcd44780.o:  lcd44780.c lcd44780.h
	$(CC) $(CFLAGS) -c lcd44780.c

lcd44780test.o: lcd44780test.c lcd44780.h
	$(CC) $(CFLAGS) -c lcd44780test.c

lcd44780test: lcd44780test.o lcd44780.a
	$(CC) $(CFLAGS) -o lcd44780test lcd44780test.o lcd44780.a

clean: 
	$(RM) *.a *.o lcd44780test
