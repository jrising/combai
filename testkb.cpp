#include <stdio.h>
#include <termios.h>

/********************************************************/

int kbhit(void)
{
  struct termios term, oterm;
  int fd = 0;
  int c = 0;
  
  /* get the terminal settings */
  tcgetattr(fd, &oterm);

  /* get a copy of the settings, which we modify */
  memcpy(&term, &oterm, sizeof(term));

  /* put the terminal in non-canonical mode, any 
     reads timeout after 0.1 seconds or when a 
     single character is read */
  term.c_lflag = term.c_lflag & (!ICANON);
  term.c_cc[VMIN] = 0;
  term.c_cc[VTIME] = 0;
  tcsetattr(fd, TCSANOW, &term);

  /* get input - timeout after 0.1 seconds or 
     when one character is read. If timed out
     getchar() returns -1, otherwise it returns
     the character */
  c=getchar();

  /* reset the terminal to original state */
  tcsetattr(fd, TCSANOW, &oterm);

  /* if we retrieved a character, put it back on
     the input stream */
  if (c != -1)
    ungetc(c, stdin);

  /* return 1 if the keyboard was hit, or 0 if it
     was not hit */
  return ((c!=-1)?1:0);
}

/********************************************************/

int getch()
{
  int c, i, fd=0;
  struct termios term, oterm;
  
  /* get the terminal settings */
  tcgetattr(fd, &oterm);
  
  /* get a copy of the settings, which we modify */
  memcpy(&term, &oterm, sizeof(term));
  
  /* put the terminal in non-canonical mode, any 
     reads will wait until a character has been
     pressed. This function will not time out */
  term.c_lflag = term.c_lflag & (!ICANON);
  term.c_cc[VMIN] = 1;
  term.c_cc[VTIME] = 0;
  tcsetattr(fd, TCSANOW, &term);

  /* get a character. c is the character */
  c=getchar();

  /* reset the terminal to its original state */
  tcsetattr(fd, TCSANOW, &oterm);

  /* return the charcter */
  return c;
}

/********************************************************/

/* simple test program */

#define CTRLC 3

main(int argc, char **argv)
{
  char c;
              
  /* test if the keyboard has been hit. 
     NOTE: will always miss, unless you manage to 
     hit the keyboard within 0.1 seconds of the
     program starting.*/
  if (kbhit())
    printf("Hit!\n");
  else
    printf("Miss!\n");

  /* wait until the keyboard has been hit. Then
     notify the user and remove the character off
     stdin */
  while (!kbhit());
  printf("Keyboard has been hit\n");
  /*getch();*/

  /* while the user doesn't hit CTRL-C, keep getting
     the next character and printing its value */
  while ((c=getch()) != CTRLC)
    printf("Key pressed %i (%c)\n", c, c);
}

/********************************************************/
