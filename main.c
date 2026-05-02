#include <stdbool.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>


char **parse_cmd(char *input);

int exec_builtin(char **args); /* returnar 0 vid funnen och -1 vid icke-funnen */

int main () {

    char buff[1024];
    int p[2];
    while (1) {
        memset(buff, '\0', 1024);
        fgets(buff, 1024, stdin);
        if (buff[strlen(buff) - 1] == '\n') buff[strlen(buff) - 1] = '\0';
        char **args = parse_cmd(buff);

        if (args[0] != NULL && exec_builtin(args) != 0) {

            pid_t pid;
            int last_stdin = 0;
            char  **cmd = args;
            bool pipe_found = false;
            for (int tok = 0; args[tok] != NULL; tok++) {

                if (*args[tok] == '|') {
                    args[tok] = NULL, pipe_found = true;

                    pipe(p);
                    pid = fork();
                    if (pid != 0) close(p[1]);
                    if (pid == 0) {
                        dup2(last_stdin, STDIN_FILENO);
                        dup2(p[1], STDOUT_FILENO);
                        execvp(cmd[0], cmd);
                        wait(NULL);
                    }
                    last_stdin = p[0];
                    cmd = args + tok + 1;
                }
            }
            if (pipe_found == false) {
                pid = fork();
                if (pid == 0) execvp(args[0], args);
                wait(NULL);
            } else {
                pid = fork();
                if (pid == 0) {
                    dup2(last_stdin, STDIN_FILENO);
                    execvp(cmd[0], cmd);
                    wait(NULL);
                }

            }

        }
    }

    // free()
}

char **parse_cmd(char *input) {

    char **cmd = malloc(sizeof(char *));
    char *tok = strtok(input, " ");
    if (tok == NULL) cmd[0] = NULL;

    int i = 0;
    while (tok != NULL) {
        cmd[i++] = strdup(tok);

        cmd = realloc(cmd, sizeof(char *) * (i + 1));
        tok = strtok(NULL, " ");
    }
    cmd = realloc(cmd, sizeof(char *) * (i + 1));
    cmd[i] = NULL;
    return cmd;
}

int exec_builtin(char **args) {

    // check for "cd"
    if (strcmp(args[0], "cd") == 0) {

        if (args[1] == NULL) {
            fprintf(stderr, "cd: missing arguments\n");
            return -1;
        }

        if (chdir(args[1]) != 0) {
            perror("cd failed");
            return -1;
        }
        return 0;

    }

    // check for "exit"
    if (strcmp(args[0], "exit") == 0) {

        if (args[1] != NULL) exit(atoi(args[1]));
        else exit(0);
    }
    return -1; // if cmd is not built in
}