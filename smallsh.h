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

#define STOPPED 2  /* Job is stopped */
#define RUNNING 3  /* Job is running */

typedef struct {
    int id;            /* Job ID */
    pid_t pgid;        /* Process group ID */
    char command[MAXBUF]; /* Command string */
    int status;        /* RUNNING, STOPPED */
} Job;

#define MAXJOBS 20  /* Maximum number of jobs */
