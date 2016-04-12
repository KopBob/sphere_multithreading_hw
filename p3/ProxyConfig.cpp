#include <arpa/inet.h>
#include <sys/socket.h>

#include <system_error>


#include "ProxyConfig.h"


struct sockaddr_in to_sockaddr(const char *ip, int port) {
    struct sockaddr_in sin;

    memset(&sin, 0, sizeof(struct sockaddr_in));
    sin.sin_family = AF_INET;
    sin.sin_port = htons(port);

    if (ip) {
        inet_pton(AF_INET, ip, &(sin.sin_addr));
    } else {
        sin.sin_addr.s_addr = INADDR_ANY;
    }

    return sin;
}

void ProxyConfig::from_config_file(const char *path_to_config) {
    FILE *fp;
    int master_sock_port;

    fp = fopen(path_to_config, "r+");
    if (fp == NULL)
        throw std::system_error(errno, std::system_category());

    fscanf(fp, "%d,", &master_sock_port);
    master_sockaddr = to_sockaddr(NULL, master_sock_port);

    while (true) {
        char ip[INET_ADDRSTRLEN];
        int port;

        int ret = fscanf(fp, "%16[^:]:%d,", ip, &port);
        if (ret == EOF or ret != 2)
            break;

        servers_sockaddrs.push_back(to_sockaddr(ip, port));
    }
    fclose(fp);
}
