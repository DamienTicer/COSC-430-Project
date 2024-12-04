
#include "smallsh_updated.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>

job_t* job_list = NULL; // Head of the job list
int job_counter = 0;    // Counter for job IDs
pid_t fg_pgid = 0;      // Foreground process group ID

// Function declarations
void handle_sigint(int sig);
void handle_sigtstp(int sig);
void add_job(pid_t pgid, char* command, int status);
void remove_job(pid_t pgid);
void list_jobs();
void bg_job(int job_id);
void fg_job(int job_id);
void kill_job(int job_id);
int userin(char *p);
int gettok(char **outptr);
int inarg(char c);
int procline(void);
int runcommand(char **cline, int where);

// Updated signal handler for SIGINT (Ctrl-C)
void handle_sigint(int sig) {
    if (fg_pgid != 0) {
        kill(-fg_pgid, SIGINT); // Send SIGINT to foreground process group
    }
}

// Updated signal handler for SIGTSTP (Ctrl-Z)
void handle_sigtstp(int sig) {
    if (fg_pgid != 0) {
        kill(-fg_pgid, SIGTSTP); // Send SIGTSTP to foreground process group
    }
}

// Add a new job to the job list
void add_job(pid_t pgid, char* command, int status) {
    job_t* new_job = (job_t*)malloc(sizeof(job_t));
    new_job->id = ++job_counter;
    new_job->pgid = pgid;
    strncpy(new_job->command, command, MAXBUF);
    new_job->status = status;
    new_job->next = job_list;
    job_list = new_job;
}

// Remove a job from the job list
void remove_job(pid_t pgid) {
    job_t** current = &job_list;
    while (*current) {
        if ((*current)->pgid == pgid) {
            job_t* temp = *current;
            *current = (*current)->next;
            free(temp);
            return;
        }
        current = &((*current)->next);
    }
}

// List all jobs with their status
void list_jobs() {
    job_t* current = job_list;
    while (current) {
        printf("[%d] %s %s", current->id, current->status ? "Stopped" : "Running", current->command);
        current = current->next;
    }
}

// Resume a stopped job in the background
void bg_job(int job_id) {
    job_t* current = job_list;
    while (current) {
        if (current->id == job_id) {
            kill(-current->pgid, SIGCONT); // Send SIGCONT to process group
            current->status = 0; // Mark as running
            return;
        }
        current = current->next;
    }
    printf("Job %d not found", job_id);
}

// Bring a job to the foreground
void fg_job(int job_id) {
    job_t* current = job_list;
    while (current) {
        if (current->id == job_id) {
            fg_pgid = current->pgid;
            tcsetpgrp(STDIN_FILENO, fg_pgid); // Give terminal control
            kill(-current->pgid, SIGCONT); // Resume process group
            int status;
            waitpid(-fg_pgid, &status, WUNTRACED);
            if (WIFSTOPPED(status)) {
                current->status = 1; // Mark as stopped
            } else {
                remove_job(current->pgid); // Remove completed job
            }
            fg_pgid = 0; // Reset foreground group
            tcsetpgrp(STDIN_FILENO, getpgrp()); // Return terminal control
            return;
        }
        current = current->next;
    }
    printf("Job %d not found", job_id);
}

// Terminate a job
void kill_job(int job_id) {
    job_t* current = job_list;
    while (current) {
        if (current->id == job_id) {
            kill(-current->pgid, SIGKILL); // Send SIGKILL to process group
            remove_job(current->pgid); // Remove job from list
            return;
        }
        current = current->next;
    }
    printf("Job %d not found", job_id);
}

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

// Main function: initialize signal handlers and process commands
int main() {
    signal(SIGINT, handle_sigint);  // Set up SIGINT handler
    signal(SIGTSTP, handle_sigtstp); // Set up SIGTSTP handler

    char* prompt = "Command> ";
    while (userin(prompt) != EOF) {
        procline();
    }
    return 0;
}