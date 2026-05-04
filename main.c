#include <unistd.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>

#define MAX_ARGS 64
#define MAX_SEGMENTS 16

char **parse_input(char *input); // returns an array of strings to each token in the input separated by spaces.
void free_input(char **args);

int exec_builtin(char **args); // returns 0 if built-in cmd's like 'cd' are found, -1 when not.

typedef struct {
    char *exec_args[MAX_ARGS];
    char *stdin_redir;
    char *stdout_redir;

} Segment; /* struct that will, upon distinguishing tokens returned by parse_cmd(),
              contain valid args and two pointers to a file name's (if < or > is found, else NULL) */

int main () {

    char buff[1024];
    while (1) {
        memset(buff, '\0', 1024);
        fgets(buff, 1024, stdin);
        if (buff[strlen(buff) - 1] == '\n') buff[strlen(buff) - 1] = '\0';
        char **args = parse_input(buff);

        if (args[0] != NULL && exec_builtin(args) != 0) {

            Segment segments[MAX_SEGMENTS] = {0};
            int current_segment = 0, arg_count = 0;
            for (int tok = 0; args[tok] != NULL; tok++) {

                if (strcmp(args[tok], "<") == 0) {
                    if (args[tok + 1] != NULL) {
                        segments[current_segment].stdin_redir = args[tok + 1];
                        tok++; // skip filename
                    } else fprintf(stderr, "nash: no filename specified after '<'\n");
                }
                else if (strcmp(args[tok], ">") == 0) {
                    if (args[tok + 1] != NULL) {
                        segments[current_segment].stdout_redir = args[tok + 1];
                        tok++; // skip filename
                    } else fprintf(stderr, "nash: no filename specified after '>'\n");
                }
                else if (strcmp(args[tok], "|") == 0) {
                    current_segment++;
                    arg_count = 0;
                }
                else {
                    segments[current_segment].exec_args[arg_count++] = args[tok];
                }

            }

            int total_segments = current_segment + 1;
            int pipefds[2], last_stdin = STDIN_FILENO;
            for (int i = 0; i < total_segments; i++) {

                if (i < total_segments - 1)
                    pipe(pipefds);

                pid_t child = fork();
                if (child == 0) {

                    if (i > 0) {
                        dup2(last_stdin, STDIN_FILENO);
                    }

                    if (i < total_segments - 1) {
                        dup2(pipefds[1], STDOUT_FILENO);
                        close(pipefds[0]);
                        close(pipefds[1]);
                    }

                    if (segments[i].stdout_redir) {
                        int fd_out = open(segments[i].stdout_redir, O_WRONLY | O_TRUNC | O_CREAT, 0644);
                        if (fd_out == -1) {
                            fprintf(stderr, "nash: %s: %s\n", segments[i].stdout_redir, strerror(errno));
                            exit(1);
                        }
                        dup2(fd_out, STDOUT_FILENO);
                    }

                    if (segments[i].stdin_redir) {
                        int fd_in = open(segments[i].stdin_redir, O_RDONLY);
                        if (fd_in == -1) {
                            fprintf(stderr, "nash: %s: %s\n", segments[i].stdin_redir, strerror(errno));
                            exit(1);
                        }
                        dup2(fd_in, STDIN_FILENO);
                    }

                    execvp(segments[i].exec_args[0], segments[i].exec_args);
                    exit(1);
                }
                else {
                    waitpid(child, NULL, 0);

                    if (i > 0) {
                        close(last_stdin);
                    }

                    if (i < total_segments - 1) {
                        close(pipefds[1]);
                        last_stdin = pipefds[0];
                    }
                }
            }

        }
        free_input(args);
    }


}

char **parse_input(char *input) {

    char **cmd = malloc(sizeof(char *));
    if (cmd == NULL) {
        perror("malloc failed");
        exit(1);
    }
    char *tok = strtok(input, " ");
    if (tok == NULL) {
        cmd[0] = NULL;
        return cmd;
    }

    int i = 0;
    while (tok != NULL) {
        cmd[i++] = strdup(tok);

        char **temp = realloc(cmd, sizeof(char *) * (i + 1));
        if (temp == NULL) {
            free_input(cmd);
            perror("realloc failed");
            exit(1);
        }
        cmd = temp;

        tok = strtok(NULL, " ");
    }

    cmd[i] = NULL;
    return cmd;
}

void free_input(char **args) {
    for (int i = 0; args[i] != NULL; i++)
        free(args[i]);

    free(args);
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

        if (args[1] != NULL) exit(strtol(args[1], NULL, 10)); // se till att ingen implicit typ-kastning sker
        else exit(0);
    }
    return -1; // if cmd is not built in
}