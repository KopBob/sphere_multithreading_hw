#include <sys/socket.h>
#include <netinet/in.h>
#include <system_error>
#include <sys/fcntl.h>
#include <sys/event.h>
#include <strings.h>
#include <unistd.h>
#include <vector>
#include <string>
#include <map>

// Mac OS X

#define MAX_BUF 5


int set_nonblock(int fd) {
    int flags;

#if defined(O_NONBLOCK)
    if (-1 == (flags = fcntl(fd, F_GETFL, 0)))
        flags = 0;
    return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
#else
    flags = 1;
    return ioctl(fd, FIOBIO, &flags);
#endif
}


class Server {
    int port;
    int master_sock_fd;

    const static char *welcome_msg;

    void mainLoop();
    void initMasterSock();
public:
    Server(int _port);
    ~Server() { };
    void run();
};


const char *Server::welcome_msg = "Welcome\n";


Server::Server(int _port)
        : port(_port) { }


void Server::initMasterSock() {
    master_sock_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (master_sock_fd == -1)
        throw std::system_error(errno, std::system_category());

    set_nonblock(master_sock_fd); // Non block socket

    int optval = 1; // Reusable descriptor if we kill app
    setsockopt(master_sock_fd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));


    struct sockaddr_in SockAddr;
    SockAddr.sin_family = AF_INET;
    SockAddr.sin_port = htons(port);
    SockAddr.sin_addr.s_addr = INADDR_ANY;

    int res = bind(master_sock_fd, (struct sockaddr *) &SockAddr, sizeof(SockAddr));
    if (res == -1)
        throw std::system_error(errno, std::system_category());

    res = listen(master_sock_fd, SOMAXCONN);
    if (res == -1)
        throw std::system_error(errno, std::system_category());
}


void Server::mainLoop() {
    int KQueue = kqueue();

    struct kevent changeEvent;
    struct kevent eventlist[100];

    bzero(&changeEvent, sizeof(changeEvent));
    EV_SET(&changeEvent, master_sock_fd, EVFILT_READ, EV_ADD, 0, 0, 0);
    kevent(KQueue, &changeEvent, 1, NULL, 0, NULL);

    std::map<int, std::string> clientsMap;

    while (true) {
        bzero(eventlist, 100 * sizeof(struct kevent));
        int kN = kevent(KQueue, NULL, 0, eventlist, 100, NULL);

        for (int i = 0; i < kN; ++i) {
            if (eventlist[i].filter == EVFILT_READ) {
                if (eventlist[i].ident == master_sock_fd) {
                    int slave_sock_fd = accept(master_sock_fd, 0, 0);
                    if (slave_sock_fd == -1)
                        throw std::system_error(errno, std::system_category());
                    set_nonblock(slave_sock_fd);

                    bzero(&changeEvent, sizeof(changeEvent));
                    EV_SET(&changeEvent, slave_sock_fd, EVFILT_READ, EV_ADD, 0, 0, 0);
                    kevent(KQueue, &changeEvent, 1, NULL, 0, NULL);

                    clientsMap[slave_sock_fd] = "";


                    send(slave_sock_fd, welcome_msg, strlen(welcome_msg), SO_NOSIGPIPE);

                    fprintf(stdout, "LOG: accepted connection\n");
                    fflush(stdout);

                } else {
                    char buffer[MAX_BUF];
                    int n = (int) read(eventlist[i].ident, buffer, MAX_BUF);
                    buffer[n] = '\0';

                    if (n <= 0) {
                        fprintf(stdout, "LOG: connection terminated\n");
                        fflush(stdout);

                        close((int) eventlist[i].ident);
                        clientsMap.erase((int) eventlist[i].ident);
                    } else {
                        std::string msg = clientsMap[eventlist[i].ident].append(buffer);

                        if (msg[msg.size() - 1] == '\n') {
                            fprintf(stdout, "LOG MSG: %s", msg.c_str());
                            fflush(stdout);

                            for (auto &kv : clientsMap)
                                send(kv.first, msg.c_str(), msg.size(), SO_NOSIGPIPE);

                            clientsMap[eventlist[i].ident] = "";
                        }
                    }

                }


            }

        }
    }
}


void Server::run() {
    initMasterSock();
    mainLoop();
}


int main() {
    Server s = Server(3100);
    s.run();
    return 0;
}

