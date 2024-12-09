#include "smallsh.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>
#include <signal.h>

//Global variables
static Job jobs[MAXJOBS];
static int job_count = 0;

// Function declarations
int userin(char *p);
int gettok(char **outptr);
int inarg(char c);
int procline(void);
int runcommand(char **cline, int where);
void add_job(pid_t pgid, char *command, int status);
void delete_job(pid_t pgid);
Job *find_job_by_id(int id);
void list_jobs();
void change_job_status(pid_t pgid, int status);


/* program buffers and work pointers */
static char inpbuf[MAXBUF], tokbuf[2*MAXBUF], *ptr = inpbuf, *tok = tokbuf;

/* Global flag for foreground process ID */
static pid_t foreground_pid = -1;

/* Signal handler for SIGINT */
void handle_sigtstp(int signo) {
    if (foreground_pid > 0) {
        kill(-foreground_pid, SIGTSTP); // Stop the foreground process group
        change_job_status(foreground_pid, STOPPED);
        printf("\n");
        fflush(stdout);
    }
}

//Job Handlers
/* Add a job to the jobs array */
void add_job(pid_t pgid, char *command, int status) {
    if (job_count >= MAXJOBS) {
        printf("Job table full, cannot add job.\n");
        return;
    }
    jobs[job_count].id = job_count + 1;
    jobs[job_count].pgid = pgid;
    strncpy(jobs[job_count].command, command, MAXBUF - 1);
    jobs[job_count].command[MAXBUF - 1] = '\0';
    jobs[job_count].status = status;
    job_count++;
}

/* Delete a job from the jobs array */
void delete_job(pid_t pgid) {
    for (int i = 0; i < job_count; i++) {
        if (jobs[i].pgid == pgid) {
            for (int j = i; j < job_count - 1; j++) {
                jobs[j] = jobs[j + 1];
            }
            job_count--;
            return;
        }
    }
}

/* Find a job by its ID */
Job *find_job_by_id(int id) {
    for (int i = 0; i < job_count; i++) {
        if (jobs[i].id == id) {
            return &jobs[i];
        }
    }
    return NULL;
}

/* Change the status of a job */
void change_job_status(pid_t pgid, int status) {
    for (int i = 0; i < job_count; i++) {
        if (jobs[i].pgid == pgid) {
            jobs[i].status = status;
            return;
        }
    }
}

/* List all jobs */
void list_jobs() {
    for (int i = 0; i < job_count; i++) {
        printf("[%d] %s (%s)\n", jobs[i].id, 
            jobs[i].command, 
            jobs[i].status == RUNNING ? "Running" : "Stopped");
    }
}

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

        // Built-in commands
        if (strcmp(arg[0], "jobs") == 0) {
            list_jobs();  // List jobs when "jobs" is entered
        } else if (strcmp(arg[0], "bg") == 0) {
            Job *job = find_job_by_id(atoi(arg[1]));
            if (job) {
                kill(-job->pgid, SIGCONT);  // Resume job in background
                job->status = RUNNING;
            } else {
                printf("No such job\n");
            }
        } else if (strcmp(arg[0], "fg") == 0) {
            Job *job = find_job_by_id(atoi(arg[1]));
            if (job) {
                foreground_pid = job->pgid;  // Move job to foreground
                kill(-job->pgid, SIGCONT);   // Resume job
                waitpid(-job->pgid, NULL, 0);  // Wait for it to complete
                delete_job(job->pgid);  // Remove from jobs list
            } else {
                printf("No such job\n");
            }
        } else if (strcmp(arg[0], "kill") == 0) {
            if (arg[1] == NULL) {
                printf("Usage: kill <job_id>\n");
            } else {
                Job *job = find_job_by_id(atoi(arg[1]));  // Find job by ID
                if (job) {
                    // Send SIGKILL to the entire process group
                    if (kill(-job->pgid, SIGKILL) == -1) {
                        perror("kill");  // Print error if unable to send signal
                    } else {
                        printf("Job [%d] (%s) terminated.\n", job->id, job->command);
                        delete_job(job->pgid);  // Remove the job from the list
                    }
                } else {
                    printf("No such job\n");
                }
            }
        } else {
            runcommand(arg, type);  // Run other commands
        }
    }

    if (toktype == EOL)
        return 0;

    narg = 0;
    break;
        }
    }
}

/* execute a command with optional wait */
int runcommand(char **cline, int where) {
    pid_t pid;
    int status;

    switch (pid = fork()) {
        case -1:
            perror("smallsh");
            return (-1);
        case 0:
            // Child process
            setpgid(0, 0);  // Set the child process group to its own PID
            signal(SIGINT, SIG_DFL);
            signal(SIGTSTP, SIG_DFL);
            execvp(*cline, cline);
            perror(*cline);
            exit(1);
    }

    // Parent process
    if (where == BACKGROUND) {
        setpgid(pid, pid);  // Ensure the child process group is set
        printf("[Process id %d]\n", pid);
        add_job(pid, cline[0], RUNNING);  // Add background job
        return (0);
    }

    foreground_pid = pid;
    setpgid(pid, pid);  // Ensure the foreground process group is set

    if (waitpid(pid, &status, WUNTRACED) == -1) {
        perror("waitpid");
        return (-1);
    }

    if (WIFSTOPPED(status)) {
        printf("\n[Process %d stopped]\n", pid);
        add_job(pid, cline[0], STOPPED);  // Add stopped job
    }

    foreground_pid = -1;
    return status;
}

char *prompt = "Command> "; /* prompt */

int main()
{
    // Set up SIGINT handler
    signal(SIGINT, handle_sigtstp);

    while (userin(prompt) != EOF)
        procline();

    return 0;
}
