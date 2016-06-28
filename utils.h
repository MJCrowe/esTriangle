
/* ************************************************************************* *

  Project Title : Star Monitor

  Module Name : PSF.h

  Description : A header file containing macros, structures, global variables
    and prototypes of timer functions for the starmon program.

  Modification Record :
  Change Date     Author Description
  --------------------------------------
  1.0  28.04.13   Micro  Created from AIPFns.h.
  1.1  23.06.16   Micro  Add urandom,urandom1,init_keyboard,getkeycode,
                         restore_terminal,uelapsedtime.

 * ************************************************************************* */



#ifndef __TIMER_H__
#define __TIMER_H__

/* ************************************************************************* *
 * FUNCTION PROTOTYPES
 * ************************************************************************* */

char *mytimedate(void) ;

void resettimer(long ltim) ;

long elapsedtime(long ltim) ;

double uelapsedtime(long ltim) ;

int urandom(int limit) ;

int urandom1(void) ;

int init_keyboard(void) ;

int getkeycode(int keyboard_fd) ;

void restore_terminal(void) ;

#endif // __TIMER_H__


