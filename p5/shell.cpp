#include <cstdio>
#include <string>
#include <vector>
#include <cstdlib>
#include <unistd.h>
#include <iostream>
#include <fstream>
#include <stdio.h>
#include <fcntl.h>

#define MAX_BUF 1024

#define STATUS_FAILURE 1
#define STATUS_SUCCESS 0


#define BG_OPERATOR  "&"
#define AND_OPERATOR  "&&"
#define OR_OPERATOR   "||"
#define PIPE_OPERATOR "|"
#define END_OPERATOR "\n"
#define EOF_OPERATOR "EXIT"

#define IN_REDIRECT   "<"
#define OUT_REDIRECT  ">"


typedef enum {
    EMP,
    AND,
    OR,
    PIPE,
    END,
    EXIT
} Operator;

typedef enum {
    IN,
    OUT,
} Redirection;


struct Command {
    std::vector<std::string> argv;
    std::string in;
    std::string out;
    int fd_in;
    int fd_out;

    Command() : in(""), out(""), fd_in(-1), fd_out(-1) { };

    Command(std::string cmd) : in(""), out(""), fd_in(-1), fd_out(-1) {
        argv.push_back(cmd);
    }

    void set_redirection(Redirection redirection, std::string source) {
        if (redirection == IN) {
            in = source;
        } else {
            out = source;
        }
    }

    char **get_argv_as_c_array() {
        std::vector<char *> cstrings;

        for (size_t i = 0; i < argv.size(); ++i)
            cstrings.push_back(const_cast<char *>(argv[i].c_str()));
        cstrings.push_back(NULL);
        return &cstrings[0];
    }
};


/*
 * Input reader funcitons
 */
std::string next_token();

void skipws() {
    int c;
    while ((c = getchar()) == ' ' or c == '\t');
    ungetc(c, stdin);
}

void skipws_nl() {
    int c;
    while ((c = getchar()) == ' ' or c == '\t' or c == '\n');
    ungetc(c, stdin);
}

std::string
next_token() {
    int c, i, ret;
    bool is_in_quote = false;

    skipws();

    std::string token;
    while ((c = getchar()) != EOF and (c != '\n') \
                and (c != ' ' or is_in_quote) \
                and (c != '<') and (c != '>') \
                and (c != '&') and (c != '|')) {
        if (c == '"') {
            is_in_quote = !is_in_quote;
            continue;
        }

        token += (char) c;
    }
    if (token.empty()) {
        if (c == '&') {
            if ((c = getchar()) == '&')
                return AND_OPERATOR;
            ungetc(c, stdin);
            return BG_OPERATOR;
        };
        if (c == '|') {
            if ((c = getchar()) == '|')
                return OR_OPERATOR;
            ungetc(c, stdin);
            return PIPE_OPERATOR;
        }
        if (c == '\n') return END_OPERATOR;
        if (c == '>') return OUT_REDIRECT;
        if (c == '<') return IN_REDIRECT;
    }

    ungetc(c, stdin);

    return token;
}

std::string read_from_pipe(int in_fd) {
    std::string output;

    char buffer[MAX_BUF];
    int n = (int) read(in_fd, buffer, MAX_BUF);
    buffer[n] = '\0';
//    while ((int) read(in_fd, buffer, MAX_BUF)) {
//        output += buffer;
//    }
    output += buffer;
    return output;
}


/*
 * Command builder
 */


int is_operator(const char *token) {
    if (strcmp(token, AND_OPERATOR) == 0) {
        return AND;
    } else if (strcmp(token, OR_OPERATOR) == 0) {
        return OR;
    } else if (strcmp(token, PIPE_OPERATOR) == 0) {
        return PIPE;
    } else if (strcmp(token, END_OPERATOR) == 0) {
        return END;
    }

    return -1;
}

int is_redirection(const char *token) {
    if (strcmp(token, IN_REDIRECT) == 0) {
        return IN;
    } else if (strcmp(token, OUT_REDIRECT) == 0) {
        return OUT;
    }

    return -1;
}

void
read_next(Command *&cmd, Operator &op) {
    int ret;
    std::string token;
    Redirection redirection;

    while (!(token = next_token()).empty()) {
        if ((ret = is_operator(token.c_str())) != -1) {
            op = (Operator) ret;
            return;
        } else {
            if (cmd) {
                if ((ret = is_redirection(token.c_str())) != -1) {
                    redirection = (Redirection) ret;
                    std::string source = next_token();
                    cmd->set_redirection(redirection, source);
                } else {
                    cmd->argv.push_back(token);
                }
            } else {
                cmd = new Command(token);
            }
        }
    }
}


/*
 * Execution
 */


void
check_status(int wstatus) {
    if (WIFEXITED(wstatus)) {
        printf("exited, status=%d\n", WEXITSTATUS(wstatus));
    } else if (WIFSIGNALED(wstatus)) {
        printf("killed by signal %d\n", WTERMSIG(wstatus));
    } else if (WIFSTOPPED(wstatus)) {
        printf("stopped by signal %d\n", WSTOPSIG(wstatus));
    } else if (WIFCONTINUED(wstatus)) {
        printf("continued\n");
    }
}

int
execute_pipe(std::vector<Command *> execution_pipe) {
    int ret;
    int wstatus;
    pid_t pid, w;

    int master_pfd[2];
    int pfd[execution_pipe.size() - 1][2];

    /*
     * pipe initialization
     */
    pipe(master_pfd);
    for (int l = 0; l < execution_pipe.size(); ++l)
        pipe(pfd[l]);

    for (int i = 0; i < execution_pipe.size(); ++i) {
        auto cmd = execution_pipe[i];
        if (!cmd->in.empty()) {
            cmd->fd_in = open(cmd->in.c_str(), O_RDONLY);
        }

        if (!cmd->out.empty()) {
            cmd->fd_out = open(cmd->out.c_str(), O_WRONLY | O_TRUNC | O_CREAT, S_IRUSR | S_IRGRP | S_IWGRP | S_IWUSR);
        }
    }

    /*
     * fork child processes
     */
    for (int i = 0; i < execution_pipe.size(); ++i) {
        auto cmd = execution_pipe[i];

        if ((pid = fork()) == 0) {
            if (i == execution_pipe.size() - 1) { // last cmd in pipe
                if (i - 1 >= 0)
                    dup2(pfd[i - 1][0], STDIN_FILENO);
                dup2(master_pfd[1], STDOUT_FILENO);
            } else if (i == 0) { // first cmd in pipe
                dup2(pfd[i][1], STDOUT_FILENO);
            } else { // intermediate cmd in pipe
                dup2(pfd[i - 1][0], STDIN_FILENO);
                dup2(pfd[i][1], STDOUT_FILENO);
            }

            if (cmd->fd_in != -1)
                dup2(cmd->fd_in, STDIN_FILENO);

            if (cmd->fd_out != -1)
                dup2(cmd->fd_out, STDOUT_FILENO);

            for (int l = 0; l < execution_pipe.size(); ++l) {
                close(pfd[l][0]);
                close(pfd[l][1]);
            }
            close(master_pfd[0]);
            close(master_pfd[1]);

            execvp(execution_pipe[i]->argv[0].c_str(), execution_pipe[i]->get_argv_as_c_array());
            perror("execl");
            _exit(EXIT_FAILURE);
        } else if (pid == -1) {
            perror("fork");
            exit(EXIT_FAILURE);
        }
    }

    if (pid != 0) {
        for (int l = 0; l < execution_pipe.size(); ++l) {
            auto cmd = execution_pipe[l];
            if (cmd->fd_in != -1)
                close(cmd->fd_in);

            if (cmd->fd_out != -1)
                close(cmd->fd_out);

            close(pfd[l][0]);
            close(pfd[l][1]);
        }
        close(master_pfd[1]);
        std::cout << read_from_pipe(master_pfd[0]);
        close(master_pfd[0]);

        do {
            w = waitpid(pid, &wstatus, WUNTRACED | WCONTINUED);
            if (w == -1) {
                perror("waitpid");
                exit(EXIT_FAILURE);
            }

//            check_status(wstatus);
        } while (!WIFEXITED(wstatus) && !WIFSIGNALED(wstatus));
        return WEXITSTATUS(wstatus);
    }
}

int
execute(int prev_status, std::vector<Command *> execution_pipe, Operator op) {
    switch (op) {
        case EMP:
            break;
        case AND:
            if (prev_status == EXIT_FAILURE)
                return prev_status;
            break;
        case OR:
            if (prev_status != EXIT_SUCCESS)
                break;
            return prev_status;
    }

    return execute_pipe(execution_pipe);
}


/*
 * misc
 */

void clean_execution_pipe(std::vector<Command *> &execution_pipe) {
    for (Command *cmd : execution_pipe) {
        delete cmd;
    }
    execution_pipe.clear();
}


int
check_eof() {
    skipws_nl();

    int c;
    if ((c = getchar()) == EOF) {
        return STATUS_FAILURE;
    }
    ungetc(c, stdin);
    return STATUS_SUCCESS;
}

void sig_handler(int signo)
{
    if (signo == SIGINT)
        printf("received SIGINT\n");

    // kill (pid_t pid, int signum)
}

int main() {
//    signal(SIGINT, sig_handler);
//    sleep(10);

//    std::vector<std::tuple<Command*, Operator>> pipeline;

    int in = open("/Users/kopbob/dev/sphere/multithreading/sfera-mail-mt/practice/4-shell/tests/5.sh", O_RDONLY);
    dup2(in, STDIN_FILENO);

    while(check_eof() == STATUS_SUCCESS) {
        std::vector<Command *> execution_pipe;
        Operator op = EMP, next_op = EMP;
        int prev_status;

        while (op != END) {
            Command *cmd = NULL;
            read_next(cmd, op);

            execution_pipe.push_back(cmd);

            if (op == PIPE)
                continue;

            prev_status = execute(prev_status, execution_pipe, next_op);
            next_op = op;

            clean_execution_pipe(execution_pipe);
        }
    }

    close(in);
}
