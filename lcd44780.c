/******************************************************************************/
/*                                                                            */
/* LCD display library for I2C bus, using a HD44780U controller connected to  */
/* a PCF8574 'backpack'. Although the HD44780U is capable of working with 8   */
/* bit commands, the backback means that the 4 bit command mode must be used  */
/* as the backpack's backlight and the register set, enable and               */
/* read/write pin settings for the HD44780U always occupy the lower 4         */
/* bits of each byte sent to the device.                                      */
/*                                                                            */
/* Writen for a Raspberry Pi 3B+ using the Raspbian Buster operating system.  */
/* Prerequisite: PIGPIOD must be installed and running.                       */
/*                                                                            */
/* A 16x2 and 20x4 LCD with the I2C PCF8574 backpack was used in development. */
/*                                                                            */
/* (c) Tim Holyoake, 10th May 2020.                                           */
/*                                                                            */
/******************************************************************************/
#include "lcd44780.h"

/* 44780 LCD library error codes */

#define ROWTOOLOW       -1000   // Row specified as lower than ORIGIN
#define ROWTOOHIGH      -1001   // Page specified as higher than lcdrows+ORIGIN
#define COLOUTOFRANGE   -1002   // Column is lower than ORIGIN or higher than ORIGIN+lcdcols
#define COLTOOLOW       -1003   // Column specified as lower than ORIGIN
#define COLTOOHIGH      -1004   // Column specified as higher than lcdcols+ORIGIN

/* 44780 LCD general definitions */

#define ORIGIN          1       // Defines the origin point for the library.
                                // Default is 1 - top line of the display, and
                                // also for the first character of any line to make
                                // the library a little more FORTRAN friendly.
                                // It should work with ORIGIN=0 (or any other value)
                                // but this is untested.

/* 44780 LCD (HD44780U) instruction set */

#define CLEARDISPLAY            0x01
#define CURSORHOME              0x02
#define ENTRYMODESET            0x04
#define DISPLAYCONTROL          0x08
#define CURSORMOVE              0x10
#define FUNCTIONSET             0x20
#define CGRAMSETADDR            0x40
#define DDRAMSETADDR            0x80

/* 44780 LCD (HD44780U) instruction set flags */

// Combined with ENTRYMODESET instruction (bitwise or required)
#define ENTRYDEC                0x00
#define ENTRYINC                0x01
#define ENTRYRIGHT              0x00
#define ENTRYLEFT               0x02
// Combined with DISPLAYCONTROL instruction (bitwise or required)
#define BLINKOFF                0x00
#define BLINKON                 0x01
#define CURSOROFF               0x00
#define CURSORON                0x02
#define DISPLAYOFF              0x00
#define DISPLAYON               0x04
// Combined with CURSORMOVE instuction (bitwise or required)
#define GOLEFT                  0x00
#define GORIGHT                 0x04
#define GOCURSOR                0x00
#define GODISPLAY               0x08
// Combined with FUNCTIONSET instruction (bitwise or required)
#define CHAR5X8                 0x00
#define CHAR5X10                0x04
#define ONELINE                 0x00
#define TWOLINE                 0x08
#define FOURBIT                 0x00
#define EIGHTBIT                0x10

// Other pins (used by the PCF8574 backpack)
#define BACKLIGHT               0x08
#define ENABLE                  0x04
#define READWRITE               0x02
#define REGISTERSET             0x01


// 44780 LCD global variables;

static char bl44780=BACKLIGHT;  			// LCD backlight on (0x08) or off (0x00).
static uint8_t lcdrows;					// Number of rows on the LCD (1,2 or 4, typically)
static uint8_t lcdcols;					// Number of columns on the LCD (16 or 20 typically)
static uint8_t rowstart[4]={0x00, 0x40, 0x14, 0x54}; 	// Addresses for the start of each row

/* HD44780U internal library functions */

int lcd44780setpos(int pi, int fd, int row, int col) {
/******************************************************************************/
/*                                                                            */
/* Set the cursor to the specified position on the display.                   */
/*                                                                            */
/* Internal library function only.                                            */
/*									      */
/* (c) Tim Holyoake, 19th May 2020.                                           */
/*                                                                            */
/******************************************************************************/
	uint8_t curpos;
	int i;
	char buf;

	curpos=rowstart[row]+col; 	// The desired cursor position is the row
					// address plus the column offset required

	buf=DDRAMSETADDR|curpos;

	i=lcd44780writecmd4(pi,fd,buf);
	
	return(i);
}

/* HD44780U external library functions */

void lcd44780error_fprintf(int errnum) {
/******************************************************************************/
/*                                                                            */
/* Print error code description returned from lcd44780 function to stderr.    */
/* Error codes start at -1000 and descend.                                    */
/*                                                                            */
/* (c) Tim Holyoake, 10th May 2020.                                           */
/*                                                                            */
/******************************************************************************/
        char errcode[5][80]={"Row number too low (less than ORIGIN) specified",
                             "Row number too high (greater than ORIGIN+lcdrows) specified",
                             "Column number out of range",
        	             "Column number too low (less than ORIGIN) specified",
                             "Column number too high (greater than ORIGIN+lcdcols) specified"};

        if ((errnum > ROWTOOLOW) || (errnum < COLTOOHIGH)) {
		fprintf(stderr,"Unknown LCD HD44780U error number(%d)\n",errnum);
        }
        else {
		fprintf(stderr,"%s (%d)\n",errcode[(-1*errnum)-1000],errnum);
        }
        return;
}

int lcd44780str(int pi, int fd, char *writebuf, uint8_t row, uint8_t col) {
/******************************************************************************/
/*                                                                            */
/* Write a string of up to COLUMNS characters at position col of the          */
/* specified row of the display. Row ORIGIN = top row; Row ORIGIN+lcdrows-1   */
/* bottom row.                                                                */
/*                                                                            */
/* (c) Tim Holyoake, 10th May 2020.                                           */
/*                                                                            */
/******************************************************************************/
	int i,count,len;
        char buf[lcdcols];

     	/* Error handling - check row specified is in the range ORIGIN to ORIGIN+lcdrows-1 */

        if (row < ORIGIN) {
		lcd44780error_fprintf(ROWTOOLOW);
		return (ROWTOOLOW);
	} 
	else if (row > ORIGIN+lcdrows-1) {
		lcd44780error_fprintf(ROWTOOHIGH);
		return (ROWTOOHIGH);
	}

        if (col < ORIGIN) {
		lcd44780error_fprintf(COLTOOLOW);
		return (COLTOOLOW);
	} 
	else if (col > ORIGIN+lcdcols-1) {
		lcd44780error_fprintf(COLTOOHIGH);
		return (COLTOOHIGH);
	}

        /* Buffer is truncated to the row length if it is longer than the space left on the row */

	len=strlen(writebuf);
	if (len > lcdcols-col+ORIGIN) len=lcdcols-col+ORIGIN;
        strncpy(buf,writebuf,len);

	/* Set the display to the correct row and column */
	i=lcd44780setpos(pi,fd,row-ORIGIN,col-ORIGIN);

	for (count=0;count<len;count++) { 
		i=lcd44780writedata(pi,fd,buf[count]);
	}

        return(i);
}

int lcd44780chr(int pi, int fd, char *writebuf, uint8_t row, uint8_t col) {
/******************************************************************************/
/*                                                                            */
/* Write a character at row, column on the LCD display.                       */
/* Column ORIGIN+lcdcols-1 Row ORIGIN+lcdrows-1                               */
/* bottom row.                                                                */
/*                                                                            */
/* (c) Tim Holyoake, 20th May 2020.                                           */
/*                                                                            */
/******************************************************************************/
	int i;
        char buf[1];

     	/* Error handling - check row & column specified is in the range ORIGIN to ORIGIN+lcdrows-1 */

        if (row < ORIGIN) {
		lcd44780error_fprintf(ROWTOOLOW);
		return (ROWTOOLOW);
	} 
	else if (row > ORIGIN+lcdrows-1) {
		lcd44780error_fprintf(ROWTOOHIGH);
		return (ROWTOOHIGH);
	}

        if (col < ORIGIN) {
		lcd44780error_fprintf(COLTOOLOW);
		return (COLTOOLOW);
	} 
	else if (col > ORIGIN+lcdcols-1) {
		lcd44780error_fprintf(COLTOOHIGH);
		return (COLTOOHIGH);
	}

        /* Buffer is truncated to 1 character */

        strncpy(buf,writebuf,1);

	/* Set the display to the correct row and column */
	i=lcd44780setpos(pi,fd,row-ORIGIN,col-ORIGIN);

	/* Output the character */
	i=lcd44780writedata(pi,fd,buf[0]);

        return(i);
}

int lcd44780init(int pi, int fd, int rows, int cols) {
/******************************************************************************/
/*                                                                            */
/* Initialize the LCD. See the product data sheet for detailes and other      */
/* options (it can be found on many places on the internet!)                  */
/*                                                                            */
/* To initialize the display, it needs to be sent three sets of commands in 8 */
/* bit mode, to ensure the HD44780U is in a known state before the actual     */
/* initialization sequence is started. The next command (sent in 8 bit mode)  */
/* puts the display into the 4 bit mode, required as we're using I2C with a   */
/* PCF8574 backpack. Everything that then follows is in 4 bit mode - i.e.     */
/* the high nibble, then low nibble of each command or data transfer must be  */
/* sent with the enable bit high, then sent again with it low, to clock it    */
/* into the device. Simple when you know how ...                              */
/*                                                                            */
/* The reasons for this are described in various tutorials and datasheets     */
/* that can be found with ease on t'internet.                                 */
/*                                                                            */
/* (c) Tim Holyoake, 10th May 2020.                                           */
/*                                                                            */
/******************************************************************************/
        int i,count;
	char buf;
	struct timespec t;

	lcdrows=rows;			// Save the display layout
	lcdcols=cols;
	
	t.tv_sec=0;			// Time to sleep = 0 seconds plus a
	t.tv_nsec=100000000L;		// minimum of 100ms (100,000,000 nanoseconds)

 	
	nanosleep(&t, (struct timespec *)NULL);			// Wait for power up

	buf=((FUNCTIONSET|EIGHTBIT)>>4)&0x0F;		        // Weird initialization sequence to
	for (count=1; count<=3; count++) {			// put the HD44780U into a known state
	 	i=lcd44780writecmd8(pi,fd,buf);			// (8 bit mode) before setting it into 4 bit mode.
		nanosleep(&t, (struct timespec *)NULL);		// Slow, so delay needed
	}

	buf=((FUNCTIONSET|FOURBIT)>>4)&0x0F;			// Set display to use 4 bit cmnds
								// Absolutely required if I2C is used!
	i=lcd44780writecmd8(pi,fd,buf);
	nanosleep(&t, (struct timespec *)NULL);			// Slow, so delay needed

	/* We're now definitely in 4 bit mode, so no longer need to shift the command down into the
           bottom 4 bits (before it's shifted up 4 and combined with the backlight and enable bits) */

	buf=FUNCTIONSET|FOURBIT|TWOLINE;			// Set display to use 2 lines
	i=lcd44780writecmd4(pi,fd,buf);

	buf=DISPLAYCONTROL|DISPLAYOFF|CURSOROFF|BLINKOFF;	// Turn the display off
	i=lcd44780writecmd4(pi,fd,buf);

	buf=ENTRYMODESET|ENTRYRIGHT;				// Set the entrymode (left to right)
	i=lcd44780writecmd4(pi,fd,buf);

	lcd44780clear(pi,fd);					// Clear the display, cursor home

	buf=DISPLAYCONTROL|DISPLAYON|BLINKOFF|CURSOROFF;	// Turn the display back on
	i=lcd44780writecmd4(pi,fd,buf);				// cursor and blink off

	return(i);
}

int lcd44780clear(int pi, int fd)
/******************************************************************************/
/*                                                                            */
/* Clear the display (which returns the cursor to the home position, too.)    */
/*                                                                            */
/* Prerequisite - lcd44780init must have been successfully called first.      */
/*                                                                            */
/* (c) Tim Holyoake, 17th May 2020.                                           */
/*                                                                            */
/******************************************************************************/
{
	int i;	
	char buf=CLEARDISPLAY;
	struct timespec t;

	t.tv_sec=0;			// Time to sleep = 0 seconds plus a
	t.tv_nsec=100000000L;		// minimum of 100ms (100,000,000 nanoseconds)

	i=lcd44780writecmd4(pi,fd,buf);				// Clearing the display is slow
	nanosleep(&t, (struct timespec *)NULL);			// so a delay is required.

	return(i);
}

int lcd44780home(int pi, int fd)
/******************************************************************************/
/*                                                                            */
/* Send cursor to the home position on the display.			      */
/*                                                                            */
/* Prerequisite - lcd44780init must have been successfully called first.      */
/*                                                                            */
/* (c) Tim Holyoake, 20th May 2020.                                           */
/*                                                                            */
/******************************************************************************/
{
	int i;	
	char buf=CURSORHOME;
	struct timespec t;

	t.tv_sec=0;			// Time to sleep = 0 seconds plus a
	t.tv_nsec=50000000L;		// minimum of 50ms (50,000,000 nanoseconds)

	i=lcd44780writecmd4(pi,fd,buf);				// Can be slow, so
	nanosleep(&t, (struct timespec *)NULL);			// a delay is required.

	return(i);
}


int lcd44780writecmd8(int pi, int fd, char data)
/******************************************************************************/
/*                                                                            */
/* Write an 8 bit command instruction to the HD44780U. (REGISTETSET = 0x00)   */
/*                                                                            */
/* Note: This can only be used during the initialization sequence, as the I2C */
/*       backpack (PCF8574) used with this device reqires 4 bit operation in  */
/*       normal usage.                                                        */
/*                                                                            */
/*       The data is sent twice - enable high, then low.                      */
/*                                                                            */
/* (c) Tim Holyoake, 17th May 2020.                                           */
/*                                                                            */
/******************************************************************************/
{
	int i;

	data=((data<<4)&0xF0)|bl44780|ENABLE;
	i=i2c_write_device(pi,fd,&data,1);
	data=data&(~ENABLE);
	i=i2c_write_device(pi,fd,&data,1);

	return (i);
}

int lcd44780writecmd4(int pi, int fd, char data)
/******************************************************************************/
/*                                                                            */
/* Write two 4 bit instructions the HD44780U. (REGISTTERSET = 0x00)           */
/*                                                                            */
/* Note: This is required for usage once the display has been initialized. It */
/*       takes an 8 bit command used by the HD44780U and splits it into two   */
/*       4 bit nibbles, placed into the top 4 bits of high and low variables. */
/*       The lower 4 bits of each of these are then populated with the        */
/*       appropriate setings for the backlight (0x08=ON),                     */
/*       enable bit (ENABLE Ox04=HIGH), read/write bit (READWRITE 0x02=READ)  */
/*       and register set bit (REGISTERSET 0x00 = command register,           */
/*       0x01 = data register).                                               */
/*                                                                            */
/*       The data is sent twice for each nibble - enable high, then low.      */
/*                                                                            */
/* (c) Tim Holyoake, 17th May 2020.                                           */
/*                                                                            */
/******************************************************************************/
{
	int i;
	char high, low;

	high=data&0xF0;
	low=(data<<4)&0xF0;

	high=high|bl44780|ENABLE;
	i=i2c_write_device(pi,fd,&high,1);
	high=high&(~ENABLE);
	i=i2c_write_device(pi,fd,&high,1);
	
	low=low|bl44780|ENABLE;
	i=i2c_write_device(pi,fd,&low,1);
	low=low&(~ENABLE);
	i=i2c_write_device(pi,fd,&low,1);

	return (i);
}

int lcd44780writedata(int pi, int fd, char data)
/******************************************************************************/
/*                                                                            */
/* Write an 8 bit data byte as two 4 bit instructions to the HD44780U.        */
/* (REGISTERSET = 0x00)                                                       */
/*                                                                            */
/* This function is identical to lcd44780writecmd, but addresses the data     */
/* register, to write characters onto the display. The register select bit is */
/* therefore 1, rather than 0.                                                */
/*                                                                            */
/* (c) Tim Holyoake, 17th May 2020.                                           */
/*                                                                            */
/******************************************************************************/
{
	int i;
	char high, low;

	high=data&0xF0;
	low=(data<<4)&0xF0;

	high=high|bl44780|REGISTERSET|ENABLE;
	i=i2c_write_device(pi,fd,&high,1);
	high=high&(~ENABLE);
	i=i2c_write_device(pi,fd,&high,1);
	
	low=low|bl44780|REGISTERSET|ENABLE;
	i=i2c_write_device(pi,fd,&low,1);
	low=low&(~ENABLE);
	i=i2c_write_device(pi,fd,&low,1);

	return (i);
}

int lcd44780backlight(int pi, int fd, uint8_t setting)
/******************************************************************************/
/*                                                                            */
/* Change the backlight setting (0=OFF, any other value = ON)                 */
/*                                                                            */
/* This data (0x00 for OFF, 0x08 for on) is written directly to the I2C bus   */
/* as the command is not used by the HD44708U - just the PCF8574 backpack.    */
/*                                                                            */
/* (c) Tim Holyoake, 17th May 2020.                                           */
/*                                                                            */
/******************************************************************************/
{
	int i;

	if (setting == 0) bl44780=0x00;
	else bl44780=BACKLIGHT;

	i=i2c_write_device(pi,fd,&bl44780,1);

	return(i);
}

int lcd44780setdisplay(int pi, int fd, uint8_t mode, uint8_t blink, uint8_t cursor)
/******************************************************************************/
/*                                                                            */
/* Set the display on or off, blink on or off and cursor on or off.           */
/*                                                                            */
/* If mode, blink or cursor are zero, this sets the respective part of the    */
/* display to off. Any other value is treated as on.                          */
/*                                                                            */
/* (c) Tim Holyoake, 22nd May 2020.                                           */
/*                                                                            */
/******************************************************************************/
{
	int i;
	char cmd=DISPLAYCONTROL;

	cmd = (mode == 0) ? cmd|DISPLAYOFF : cmd|DISPLAYON;

	cmd = (blink == 0) ? cmd|BLINKOFF : cmd|BLINKON;

	cmd = (cursor == 0) ? cmd|CURSOROFF : cmd|CURSORON;

	i=lcd44780writecmd4(pi,fd,cmd);

	return (i);
}
