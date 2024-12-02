#include "smallsh.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

// Function declarations
int userin(char *p);
int gettok(char **outptr);
int inarg(char c);
int procline(void);
int runcommand(char **cline, int where);

/* program buffers and work pointers */
static char inpbuf[MAXBUF], tokbuf[2*MAXBUF], *ptr = inpbuf, *tok = tokbuf;

/* print prompt and read a line */
int userin(char *p)
{
    int c, count;
    
    /* initialization for alter routines */
    ptr = inpbuf;
    tok = tokbuf;

    /* display prompt */
    printf("%s", p);

    count = 0;

    while(1)
    {
        if ((c = getchar()) == EOF)
            return (EOF);
        if (count < MAXBUF)
            inpbuf[count++] = c;
        if ( c == '\n' && count < MAXBUF)
        {
            inpbuf[count] = '\0';
            return count;
        }

        /* if line too long restart */
        if (c == '\n')
        {
            printf("smallsh: input line too long\n");
            count = 0;
            printf("%s", p);
        }
    }
}

/* get token, place into tokbuf */
int gettok (char **outptr)
{
    int type;

    /* set the outptr string to tok */
    *outptr = tok;

    /* strip white space from the buffer containing the tokens */
    while (*ptr == ' ' || *ptr == '\t')
        ptr++;

    /* set the token pointer to the first token in the buffer */
    *tok++ = *ptr;

    /* set the type variable depending on the token in the buffer */
    switch(*ptr++){
        case '\n':
            type = EOL;
            break;
        case '&':
            type = AMPERSAND;
            break;
        case ';':
            type = SEMICOLON;
            break;
        default:
            type = ARG;
            /* keep reading valid ordinary characters */
            while (inarg(*ptr))
                *tok++ = *ptr++;
    }

    *tok++ = '\0';
    return type;
}

static char special [] = {' ', '\t', '&', ';', '\n', '\0'};

int inarg(char c)
{
    char *wrk;

    for (wrk = special; *wrk; wrk++)
    {
        if (c == *wrk)
            return (0);
    }

    return (1);
}

int procline(void) /* process input line */
{
    char *arg[MAXARG + 1]; /* pointer array for runcommand */
    int toktype; /* type of token in command */
    int narg; /* number of arguments so far */
    int type; /* FOREGROUND or BACKGROUND */

    narg = 0;

    for(;;) /* loop forever */
    {
        /* take action according to token type */
        switch(toktype = gettok(&arg[narg])){
            case ARG: 
                if (narg < MAXARG)
                    narg++;
                break;
            case EOL:
            case SEMICOLON:
            case AMPERSAND:
                if (toktype == AMPERSAND)
                    type = BACKGROUND;
                else
                    type = FOREGROUND;
                
                if (narg != 0)
                {
                    arg[narg] = NULL;
                    runcommand(arg, type);
                }

                if (toktype == EOL)
                    return 0;
                
                narg = 0;
                break;
        }
    }
}

/* execute a command with optional wait */
int runcommand(char **cline, int where)
{
    pid_t pid;
    int status;

    switch(pid = fork()){
        case -1:
            perror("smallsh");
            return (-1);
        case 0:
            execvp(*cline, cline);
            perror(*cline);
            exit(1);
    }

    /* code for parent */
    /* if background process print pid and exit */
    if (where == BACKGROUND)
    {
        printf("[Process id %d]\n", pid);
        return (0);
    }

    /* wait until process pid exits */
    if (waitpid(pid, &status, 0) == -1)
        return (-1);
    else   
        return (status);
}

char *prompt = "Command> "; /* prompt */

int main()
{
    while (userin(prompt) != EOF)
        procline();

    return 0;
}