#include <cstdio>
#include <string>
#include <vector>
#include <cstdlib>
#include <unistd.h>
#include <iostream>
#include <fstream>
#include <fcntl.h>
#include <signal.h>

#define MAX_BUF 1024

#define STATUS_FAILURE 1
#define STATUS_SUCCESS 0


#define BG_OPERATOR  "&"
#define AND_OPERATOR  "&&"
#define OR_OPERATOR   "||"
#define PIPE_OPERATOR "|"
#define END_OPERATOR "\n"
#define SEMI_OPERATOR ";"
#define EOF_OPERATOR "EXIT"


#define IN_REDIRECT   "<"
#define OUT_REDIRECT  ">"


typedef enum {
    EMP,
    AND,
    OR,
    PIPE,
    END,
    EXIT,
    BG
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
 and (c != '&') and (c != '|') and (c != ';' or is_in_quote)) {
        if ((c == '"') or (c == '\'')) {
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
        if (c == ';') return AND_OPERATOR;
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
    } else if (strcmp(token, BG_OPERATOR) == 0) {
        return BG;
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

    int pfd[execution_pipe.size() - 1][2];

    /*
     * pipe initialization
     */
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

        pid = fork();
        if (pid == 0) {
            if (i == execution_pipe.size() - 1) { // last cmd in pipe
                if (i - 1 >= 0)
                    dup2(pfd[i - 1][0], STDIN_FILENO);
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

        do {
            w = waitpid(pid, &wstatus, WUNTRACED | WCONTINUED);
            if (w == -1) {
                perror("waitpid");
                exit(EXIT_FAILURE);
            }
        } while (!WIFEXITED(wstatus) && !WIFSIGNALED(wstatus));
//        std::cerr << "   Process " << pid <<  " exited: " << WEXITSTATUS(wstatus) << std::endl;

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

void clean_pipeline(std::vector<std::tuple<std::vector<Command *>, Operator>> &pipeline) {
    for (int i = 0; i < pipeline.size(); ++i) {
        for (Command *cmd : std::get<0>(pipeline[i])) {
//            std::cout << cmd->argv[0] << std::endl;
            delete cmd;
        }
    }
    pipeline.clear();
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



int curr_pid;

void handle_sigint(int signo) {
    if (signo == SIGINT) {
        if (curr_pid) {
            kill(curr_pid, SIGINT);
            curr_pid = 0;
        } else {
            exit(EXIT_FAILURE);
        }
    }
}

void delete_zombies(int sig)
{
    pid_t kidpid;
    int status;

    printf("Inside zombie deleter:  ");
    while ((kidpid = waitpid(-1, &status, WNOHANG)) > 0)
    {
        printf("Child %ld terminated\n", kidpid);
    }
}

void handle_sigchld(int sig) {
    int wstatus;
    pid_t w;
    do {
        w = waitpid((pid_t)(-1), &wstatus, 0);
        if (w == -1) {
            perror("waitpid");
            exit(EXIT_FAILURE);
        }
    } while (!WIFEXITED(wstatus) && !WIFSIGNALED(wstatus));
    std::cerr << "Process " << w <<  " exited: " << WEXITSTATUS(wstatus) << std::endl;
//
//    int saved_errno = errno;
//    while (waitpid((pid_t)(-1), 0, WNOHANG) > 0) {}
//    errno = saved_errno;
}

void
run_pipeline(std::vector<std::tuple<std::vector<Command *>, Operator>> &pipeline, bool is_in_bg) {
    int prev_status = 0;

    pid_t pid, w;
    int wstatus;

    switch (pid = fork()) {
        case -1:
            perror("fork");
            exit(EXIT_FAILURE);
        case 0:
            for (int i = 0; i < pipeline.size(); ++i) {
                std::vector<Command *> execution_pipe = std::get<0>(pipeline[i]);
                Operator curr_operator = std::get<1>(pipeline[i]);

                prev_status = execute(prev_status, execution_pipe, curr_operator);
            }
            _exit(EXIT_SUCCESS);
        default:
            if (!is_in_bg) {
                curr_pid = pid;
                wait4(pid, &wstatus, 0, NULL);
                std::cerr << "Process " << pid <<  " exited: " << WEXITSTATUS(wstatus) << std::endl;
            }
    }

}


int main() {
    signal(SIGINT, handle_sigint);
//    signal(SIGCHLD, delete_zombies);
//    int in = open("/Users/kopbob/dev/sphere/multithreading/sfera-mail-mt/practice/4-shell/tests/6.sh", O_RDONLY);
//    dup2(in, STDIN_FILENO);

    while (check_eof() == STATUS_SUCCESS) {
        std::vector<std::tuple<std::vector<Command *>, Operator>> pipeline;

        std::vector<Command *> execution_pipe;
        Operator curr_op = EMP, prev_op = EMP;

        while (curr_op != END and curr_op != BG) {
            Command *cmd = NULL;
            read_next(cmd, curr_op);

            execution_pipe.push_back(cmd);

            if (curr_op == PIPE)
                continue;

            pipeline.push_back(std::make_tuple(execution_pipe, prev_op));
            prev_op = curr_op;

            execution_pipe.clear();
        }

        if (curr_op == BG) {
            run_pipeline(pipeline, true);
        } else {
            run_pipeline(pipeline, false);
        }

        clean_pipeline(pipeline);
    }

//    close(in);
}
