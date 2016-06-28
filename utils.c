/* 
  This module contains code with functions concerned with time and
 other system type calls.

 20/7/93 Micro : Timer functions are calibrated for the mx486 compiler.
 26/7/93       : clock() NOT working on i860 using gettim().
 14/4/13 1.0   : Modified just to use clock_gettime(). Link in -lrt

  Modification Record :
  Change Date     Author Description
  --------------------------------------
  1.0  28.04.13   Micro  Created from AIPFns.h.
  1.1  23.06.16   Micro  Add urandom,urandom1,init_keyboard,getkeycode,
                         restore_terminal,uelapsedtime.
*/


/* Standard C library header files */
#include <time.h>  
#include <sys/time.h>  
#include <stdio.h>  
#include <stdlib.h>  
#include <ctype.h>
#include <fcntl.h>
#include <math.h>
#include <linux/input.h>
#include <termios.h>
#include <unistd.h>
#include <string.h>


#include "utils.h"



// Modified so that you use & specify multiple timers.
#define MAXNTIMERS  100

// For keyboard input of the escape key.
#define KEYBOARD     "/dev/input/event0"   // Maybe event1

// Command line to get the input device event number.
char *GET_DEVICE_COMMANDS = 
"grep -E 'Handlers|EV=' /proc/bus/input/devices |"
"grep -B1 'EV=120013' |"
"grep -Eo 'event[0-9]+' |"
"grep -Eo '[0-9]+' |"
"tr -d '\n'" ;



struct timespec timsp[MAXNTIMERS] ;  // store multiple timers start times.
clockid_t clockid = CLOCK_MONOTONIC ;
//clockid_t clockid = CLOCK_REALTIME ;

struct termios oldterminal;  // Origin Terminal settings




char *mytimedate(void) 
{
  time_t s ;
  struct tm *t ;
  char *ts ;

  time ( &s ) ;                    /* seconds from 1970 */
  t = localtime ( &s ) ;           /* pieces of time */
  ts = asctime(t) ;                /* convert to a string */
  return ( ts ) ;      
} // mytimedate



void resettimer(long ltim)
{
  struct timespec *ts = &timsp[ltim] ;

  ts->tv_sec = ts->tv_nsec = 0 ;
  clock_gettime(clockid,ts) ;

} /* resettimer */
  


/* Return elapsed time in millisecs (as per HP1000) */
long elapsedtime(long ltim) 
{
  struct timespec *ts = &timsp[ltim] ;
  struct timespec tf ;
  long time_ms = 0 ;

  tf.tv_sec = tf.tv_nsec = 0 ;
  clock_gettime(clockid,&tf) ;

  time_ms = (tf.tv_sec - ts->tv_sec)*1000 +		// seconds
            (tf.tv_nsec - ts->tv_nsec)/1000000 ;	// nanoseconds

  return time_ms ;

} // elapsedtime   




/* Return elapsed time in microsecs */
double uelapsedtime(long ltim) 
{
  struct timespec *ts = &timsp[ltim] ;
  struct timespec tf ;
  double time_us = 0 ;

  tf.tv_sec = tf.tv_nsec = 0 ;
  clock_gettime(clockid,&tf) ;

  time_us = (tf.tv_sec - ts->tv_sec)*1000000.0 +     // seconds
            (tf.tv_nsec - ts->tv_nsec)/1000.0 ;	     // nanoseconds

  return time_us ;

} // elapsedtime   



// Uniform random number distribution (range 1 to limit)
int urandom(int limit)
{
    // Make sure r = [0 to (limit-1)]
    int   r = (int) floor((rand() * (double) limit) / (RAND_MAX + (double) 1.0)) ;
    return r + 1 ;
} // urandom


// Plus or minus one [-1 , 1].
int urandom1(void)
{
    return urandom(2) * 2 - 3 ;
} // urandom1





// Try and get the keyboard device input event number.

char *getDevice(const char *cmd)
{
#define MAXNCHARS  1000
#define BLOCK       128
    static char result[MAXNCHARS];
    char buffer[MAXNCHARS];
    FILE *pipe = popen(cmd,"r") ;
    
    strcpy(result,"") ;
    while ( !feof(pipe) )
        fgets(buffer, BLOCK, pipe) ;
    pclose(pipe);

// Restriction : Only allow a first digit number else use default. 
    if ( isdigit( buffer[0] ) )
        sprintf(result,"/dev/input/event%c",buffer[0]) ;
    else
        strcpy(result,KEYBOARD) ;

    return result ;
}



/***********************************************************
 * Name: init_keyboard
 *
 * Arguments: None
 *
 * Description: Sets up the keyboard to one key input no wait.
 *
 * Returns: keyboard id.
 *
 ***********************************************************/
int init_keyboard(void)
{
    struct termios newt;
    int keyboard_fd ;
    char *keyboard = KEYBOARD ;

    keyboard = getDevice(GET_DEVICE_COMMANDS) ;

    // Disable immediate echoing.
    tcgetattr(0, &oldterminal);  /* Save terminal settings */
    newt = oldterminal;          /* Init new settings */
    newt.c_lflag &= ~(ICANON | ECHO);   /* Change settings */
    tcsetattr(0, TCSANOW, &newt);       /* Apply settings */

    // Open Device
    keyboard_fd = open(keyboard, O_RDONLY|O_NONBLOCK) ;

    if ( keyboard_fd >= 0 )
        printf("Opened keyboard input(fd=%d) with '%s'\n",keyboard_fd,keyboard) ;
    else
        printf("Unable to open() keyboard input with '%s'\n",keyboard) ;

    return keyboard_fd ;

} // init_keyboard




/***********************************************************
 * Name: getkeycode
 *
 * Arguments:
 *     keyboard_fd - keyboard id.
 *
 * Description: Check the keyboard and return is key code.
 *   Note: The keycode is NOT ASCII code. ESC = 1
 *                         
 * Returns: keycode. 
 *
 ***********************************************************/
int getkeycode(int keyboard_fd)
{
#define NREADS      10
#define NEVENTS      3
    struct input_event ev[NEVENTS];
    int i, j, key, size = sizeof(struct input_event);
//    struct timeval keytime;  // Used to ignore multiple same key events.

//    printf(".") ; fflush(NULL) ;

    key = -1 ;
    for ( j = 0 ; j < NREADS ; ++j ) {

        read(keyboard_fd, ev, size * NEVENTS) ;
   
        for ( i = 0 ; i < NEVENTS ; ++i ) { 
            if ( ev[i].value == 1 && ev[i].type == EV_KEY )
/*
                if (keytime.tv_sec != ev[i].time.tv_sec || keytime.tv_usec != ev[i].time.tv_usec)
                {
                      printf("Count %d : Event %d => Time = (%d,%d), Code[%d]\n",
                             j,i,(int) ev[i].time.tv_sec,(int) ev[i].time.tv_usec,ev[i].code);
                      fflush(NULL) ;
                      keytime = ev[i].time ;  
*/            
                      return ev[i].code ;
//                }
        } // for i
    } // several read()

    return key ;

} // getkeycode



void restore_terminal(void)
{
    // Restore terminal settings.
    tcsetattr(0, TCSANOW, &oldterminal);  /* Apply saved settings */
} // restore_terminal

