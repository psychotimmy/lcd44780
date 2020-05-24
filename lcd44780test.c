/******************************************************************************/
/*                                                                            */
/* Skeleton test progam for the                                               */
/* 44780 LCD display library for I2C bus.                                     */
/* Tested on a Raspberry Pi 3B+ using the Raspbian Buster operating system.   */
/*                                                                            */
/* Prerequisite: PIGPIOD must be installed and running.                       */
/*                                                                            */
/* (c) Tim Holyoake, 10th May 2020.                                           */
/*                                                                            */
/******************************************************************************/
#include "lcd44780.h"

/* Testing loop */

int main() {
        int ipi,fdlcd, count, x,y;
	char c;
	time_t t;

        ipi=pigpio_start(NULL,NULL);	// Initialise connection to pigpiod */ 
        if (ipi < 0) {
		fprintf(stderr,"Failed to connect to pigpiod - error %d\n",ipi);
                exit(1);
        }

        fdlcd=i2c_open(ipi,1,LCD44780ADDR,0); // Get handle to LCD display
        if (fdlcd < 0) {
		fprintf(stderr,"Failed to initialize LCD - error %d\n",fdlcd);
                exit(1);
        }

	// Initialize the LCD display

        if (lcd44780init(ipi,fdlcd,4,20) == 0) {

		lcd44780str(ipi,fdlcd,"Hello World!",1,5);
		lcd44780setdisplay(ipi,fdlcd,1,1,1);
		sleep(2);
		lcd44780setdisplay(ipi,fdlcd,0,1,1);
		sleep(2);
		lcd44780setdisplay(ipi,fdlcd,1,0,1);
		sleep(2);
		lcd44780setdisplay(ipi,fdlcd,1,1,0);
		sleep(2);
		lcd44780setdisplay(ipi,fdlcd,1,0,0);
		sleep(2);
		lcd44780backlight(ipi,fdlcd,0);
		sleep(2);
		lcd44780backlight(ipi,fdlcd,1);
		sleep(2);
		lcd44780str(ipi,fdlcd,"ABCDEFGHIJLKMNOPQRTU",2,1);
		lcd44780str(ipi,fdlcd,"VWXYZ0123456789!£*$%",3,1);
		lcd44780str(ipi,fdlcd,"qwertyuiopasdghjklzx",4,1);
		sleep(2);
		lcd44780clearline(ipi,fdlcd,2,1);
		lcd44780clearline(ipi,fdlcd,3,10);
                lcd44780clearline(ipi,fdlcd,4,20);
		sleep(2);
		lcd44780clear(ipi,fdlcd);
		srand((unsigned) time(&t));
		for (count=0;count<1000;count++) {
			x=(rand()%20)+1;
			y=(rand()%4)+1;
			c=(rand()%95)+32;
			lcd44780chr(ipi,fdlcd,&c,y,x);
		}
		sleep(2);
		lcd44780clear(ipi,fdlcd);
		lcd44780str(ipi,fdlcd,"Bye!",1,19);
		sleep(2);
		lcd44780clear(ipi,fdlcd);
		lcd44780str(ipi,fdlcd,"Bye!",1,10);
		sleep(2);
		lcd44780str(ipi,fdlcd,"Bye!",1,15);
		sleep(2);
		lcd44780str(ipi,fdlcd,"Bye!",1,3);
	}

        /* Clean up and exit */

        i2c_close(ipi,fdlcd);
        pigpio_stop(ipi);

	return(0);
}
