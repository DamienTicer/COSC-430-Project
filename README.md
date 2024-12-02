# COSC-430-Project

1. Alter smallsh from Chapter 5 so that it handles interrupts more like a real shell:

Your smallsh process shouldn’t be terminated upon <Ctrl-C> type-in. Currently, if you type <Ctrl-C>, your smallsh process will be terminated and exited.
When you type in <Ctrl-C> in middle of executing your smallsh process, only your foreground process needs to be terminated. And, your smallsh process needs to print the next “Command>” prompt on a next line.
When you type in <Ctrl-C> in middle of executing you smallsh process, your background processes should keep running.


2. Unix shells support the notion of job control, which allows users to move jobs back and forth between background and foreground, and to change the process state (running, stopped, or terminated) of the processes in a job. Typing ctrl-c causes a SIGINT signal to be delivered to each process in the foreground job. The default action for SIGINT is to terminate each process. Similarly, typing ctrl-z causes a SIGTSTP signal to be delivered to each process in the foreground job. The default action for SIGTSTP is to place a process in the stopped state, where it remains until it is awakened by the receipt of a SIGCONT signal. Unix shells also provide various built-in commands that support job control. For example:
jobs: List the running and stopped background jobs.
bg <job>: Change a stopped background job to a running background job.
fg <job>: Change a stopped or running background job to a running in the foreground.
kill <job>: Terminate a job.

Implement these commands in your smallsh.c program. You have to use your own signal handler routines for these purposes along with a simple data structure for maintaining the jobs and processes in each job. You may use process group concept in this implementation for maintaining processes in each job (job may be interpreted as a process group). If your program use fork() system calls, multiple children processes will be created which belong to the same job as your original process which spawned the children processes.

Example
Suppose that you have a program that includes 1 fork system call. Let’s name that executable file as “prog1”.  

For example, you may use the following program as “prog1.c”


int main()
{  
      int i=0;
      for (i=0; i<60; i++)
	sleep(1);
}


Suppose that you execute the following sequence of commands in less than 60 seconds. 

Command> prog1&
Command> prog1&
Command> prog1
…
Then there are 3 jobs created where each job contains 2 processes. One job is running in foreground, and two jobs are running in background.


What to submit

You need to submit the following:

your “smallsh.c” and “smallsh.h” program files.
MS Word document including the screenshot from the following scenarios:

(1) Scenario 1
$> ./smallsh
Command> ./prog1&
Command> ./prog1&
Command> ./prog1
^C

(2) Scenario 2
$> ./smallsh
Command> ./prog1&
Command> ./prog1&
Command> ./prog1
^Z

(3) Scenario 3
$> ./smallsh
Command> ./prog1&
Command> ./prog1&
Command> ./prog1
^C
Command> jobs

(4) Scenario 4
$> ./smallsh
Command> ./prog1&
Command> ./prog1&
Command> ./prog1
^Z
Command> jobs

(5) Scenario 5
$> ./smallsh
Command> ./prog1&
Command> ./prog1&
Command> ./prog1
^Z
Command> jobs
Command> fg 3

(6) Scenario 6
$> ./smallsh
Command> ./prog1&
Command> ./prog1&
Command> ./prog1
^Z
Command> jobs
Command> bg 3
Command> jobs
