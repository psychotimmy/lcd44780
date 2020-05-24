/******************************************************************************/
/*                                                                            */
/* Header file for                                                            */
/* HD44780U LCD display library for I2C bus.                                  */
/* Writen for a Raspberry Pi 3B+ using the Raspbian Buster operating system.  */
/* Prerequisite: PIGPIOD must be installed and running.                       */
/*                                                                            */
/* (c) Tim Holyoake, 10th May 2020.                                           */
/*                                                                            */
/******************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <string.h>
#include <time.h>
#include <pigpiod_if2.h>

/* 44780 LCD I2C device address */

#define LCD44780ADDR      			0x27    // I2C address of LCD.

/* Declare 44780 LCD library functions as externals */

extern void lcd44780error_fprintf(int errnum);
extern int lcd44780str(int pi, int fd, char *writebuf, uint8_t row, uint8_t col);
extern int lcd44780chr(int pi, int fd, char *writebuf, uint8_t row, uint8_t col);
extern int lcd44780clearline(int pi, int fd, uint8_t row,uint8_t col);
extern int lcd44780writecmd8(int pi, int fd, char data);
extern int lcd44780writecmd4(int pi, int fd, char data);
extern int lcd44780writedata(int pi, int fd, char data);
extern int lcd44780backlight(int pi, int fd, uint8_t setting);
extern int lcd44780setdisplay(int pi, int fd, uint8_t mode, uint8_t blink, uint8_t cursor);
extern int lcd44780clear(int pi, int fd);
extern int lcd44780home(int pi, int fd);
extern int lcd44780init(int pi, int fd, int rows, int cols);
