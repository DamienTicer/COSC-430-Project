/* smallsh.h -- defs for smallsh command processor */
#include <unistd.h>
#include <stdio.h>
#include <sys/wait.h>

#define EOL 1 /* End of Line*/
#define ARG 2 /* Normal Arguments*/
#define AMPERSAND 3
#define SEMICOLON 4

#define MAXARG 512 /* max. no. command args */
#define MAXBUF 512 /* max. length input line*/

#define FOREGROUND 0
#define BACKGROUND 1