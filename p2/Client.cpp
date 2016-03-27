#include <string>
#include <unistd.h>
#include <system_error>
#include <arpa/inet.h>

// Mac OS X

#define MAX_BUF 1024


class Client {
    std::string ip;
    int port;

    int client_sock_fd;
    char buffer[MAX_BUF];

    void initConnection();

    void mainLoop();

public:
    Client(const char *_ip, int _port);

    ~Client();

    void run();
};


Client::Client(const char *_ip, int _port)
        : ip(_ip),
          port(_port) { }


void Client::initConnection() {
    struct sockaddr_in addr;

    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = INADDR_ANY;

    client_sock_fd = socket(AF_INET, SOCK_STREAM, 0);

    if (!inet_aton(ip.c_str(), &addr.sin_addr))
        throw std::system_error(errno, std::system_category());

    if (connect(client_sock_fd, (struct sockaddr *) &addr, sizeof(addr)))
        throw std::system_error(errno, std::system_category());
}


void Client::mainLoop() {
    while (true) {
        fd_set Set;

        FD_ZERO(&Set);
        FD_SET(0, &Set);
        FD_SET(client_sock_fd, &Set);

        int res = select(client_sock_fd + 1, &Set, 0, 0, 0);

        if (res < 0)
            throw std::system_error(errno, std::system_category());

        if (FD_ISSET(0, &Set)) { // Read message from keyboard
            char c;
            int i = 0;

            while (((c = (char) getchar()) != '\n') && i < 10000) {
                buffer[i] = c;
                i++;
            }
            if (c == '\n') buffer[i] = '\n';
            buffer[i + 1] = '\0';

            int pos = 0;
            while (strlen(buffer) > MAX_BUF) {
                write(client_sock_fd, buffer + pos, MAX_BUF);
                pos += MAX_BUF;
            }
            send(client_sock_fd, buffer + pos, strlen(buffer), SO_NOSIGPIPE);
        }
        if (FD_ISSET(client_sock_fd, &Set)) { // Read message from socket
            int n = (int) read(client_sock_fd, buffer, MAX_BUF);
            buffer[n] = '\0';

            if (n <= 0)
                throw std::system_error(errno, std::system_category());
            printf("%s", buffer);
        }
    }
}


void Client::run() {
    initConnection();
    mainLoop();
}


Client::~Client() {
    shutdown(client_sock_fd, 2);
}


int main() {
    Client app("127.0.0.1", 3100);
    app.run();
    return 0;
}
