#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>

/* The purpose of this part is to work with fork, exec, and wait to create new processes
 * and use pipe to communicate between parent/child processes.
 *
 * You should implement a code to illustrate the following command: ls / | wc -l.
 * This command prints out the number of files in the root path:
 * ls / shows the files/directories in the root path,
 * and its output will be piped through | to wc -l, which counts the number of lines.
 * */

enum {BUFFER_SIZE = 100};
// char write_msg[BUFFER_SIZE] = "Hello";
char read_msg[BUFFER_SIZE];

int main() {

    // File descriptors (fd[0] for reading, fd[1] for writing)
    int fd[2];

    // Create the pipe
    if(pipe(fd) != -1) {

        // Create new process (important that this is after the creation of pipe).
        pid_t pid = fork();

        if (pid > 0) { // Parent process is running

            // Since the parent is only reading from the pipe:
            close(fd[1]);

            // Wait for his child to terminate.
            wait(NULL);

            // read(fd[0], read_msg, BUFFER_SIZE);
            // printf("%s", read_msg);

            // Redirect std input to fd[0]
            // (so that wc takes input from fd[0]).
            dup2(fd[0], STDIN_FILENO);

            char *args[] = {"wc", "-l", NULL};
            execv("/usr/bin/wc", args);

        } else if (pid == 0) { // Child process is running

            // Since the child is only writing on the pipe:
            close(fd[0]);

            // Redirect std output to fd[1]
            // (so that output of execv is written on fd[1]).
            dup2(fd[1], STDOUT_FILENO);

            // Execute the command ls
            char * args[] = {"ls", "/", NULL};
            execv("/bin/ls", args);
        }
    }
    return 0;
}
