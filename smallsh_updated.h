
/* smallsh.h -- defs for smallsh command processor */
#include <unistd.h>
#include <stdio.h>
#include <sys/wait.h>
#include <signal.h>
#include <string.h>

#define EOL 1 /* End of Line */
#define ARG 2 /* Normal Arguments */
#define AMPERSAND 3
#define SEMICOLON 4

#define MAXARG 512 /* max. no. command args */
#define MAXBUF 512 /* max. length input line */

#define FOREGROUND 0
#define BACKGROUND 1

// Define job structure for job control
typedef struct job {
    int id;                 // Job ID
    pid_t pgid;             // Process group ID
    char command[MAXBUF];   // Command string
    int status;             // 0: running, 1: stopped
    struct job* next;       // Pointer to next job
} job_t;

// Function declarations for job control
void handle_sigint(int sig);
void handle_sigtstp(int sig);
void add_job(pid_t pgid, char* command, int status);
void remove_job(pid_t pgid);
void list_jobs();
void bg_job(int job_id);
void fg_job(int job_id);
void kill_job(int job_id);
