#include <stdbool.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>

#define MAX_ARGS 64
#define MAX_SEGMENTS 16

char **parse_cmd(char *input);

int exec_builtin(char **args); /* returnar 0 vid funnen och -1 vid icke-funnen */

typedef struct {
    char *exec_args[MAX_ARGS];
    char *stdin_redir;
    char *stdout_redir;

} Segment;

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

            Segment segments[MAX_SEGMENTS];
            memset(segments, 0, sizeof(segments));
            int current_segment = 0, arg_count = 0;

            for (int tok = 0; args[tok] != NULL; tok++) {

                if (strcmp(args[tok], "<") == 0) {
                    if (args[tok + 1] != NULL) {
                        segments[current_segment].stdin_redir = args[tok + 1];
                        tok++ // skip filename
                    } else fprintf(stderr, "nash: no filename specified after <\n");
                }
                else if (strcmp(args[tok], ">") == 0) {
                    if (args[tok + 1] != NULL) {
                        segments[current_segment].stdout_redir = args[tok + 1];
                        tok++ // skip filename
                    } else fprintf(stderr, "nash: no filename specified after >\n");
                }
                else if (strcmp(args[tok], "|") == 0) {
                    current_segment++;
                    arg_count = 0;
                }
                else {
                    segments[current_segment].exec_args[arg_count++] = args[tok];
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

        if (args[1] != NULL) exit(strtol(args[1], NULL, 10)); // se till att ingen implicit typ-kastning sker
        else exit(0);
    }
    return -1; // if cmd is not built in
}