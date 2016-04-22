#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <iostream>
#include <vector>
#include <fcntl.h>

#include <event2/event.h>
#include <event2/event_compat.h>
#include <event2/bufferevent.h>

#include "ProxyConfig.h"

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


int accept_sock(struct sockaddr_in sa) {
    int sock_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock_fd == -1)
        throw std::system_error(errno, std::system_category());

    set_nonblock(sock_fd); // Non block socket
    int option = 1;
    setsockopt(sock_fd, SOL_SOCKET, SO_REUSEADDR, (char *) &option, sizeof(option));

    int res = bind(sock_fd, (struct sockaddr *) &sa, sizeof(struct sockaddr_in));
    if (res == -1)
        throw std::system_error(errno, std::system_category());

    return sock_fd;
}


struct accept_info {
    struct event_base *base;
    std::vector<sockaddr_in> *servers_sockaddrs;
};

struct proxy_info {
    struct bufferevent *other_bev;
};

void read_callback_proxy(struct bufferevent *bev, void *ctx) {
    struct proxy_info *inf = (struct proxy_info *) ctx;
    bufferevent_read_buffer(bev, bufferevent_get_output(inf->other_bev));
}

void write_callback_proxy(struct bufferevent *bev, void *ctx) {
    struct proxy_info *inf = (struct proxy_info *) ctx;
    bufferevent_write_buffer(bev, bufferevent_get_input(inf->other_bev));
}

void event_callback(struct bufferevent *bev, short events, void *ptr) {
    if (events & BEV_EVENT_CONNECTED) {
        printf("Connect okay.\n");
    } else if (events & (BEV_EVENT_ERROR | BEV_EVENT_EOF)) {
        struct event_base *base = (struct event_base *) ptr;
        if (events & BEV_EVENT_ERROR) {
            int err = bufferevent_socket_get_dns_error(bev);
            if (err)
                printf("DNS error: %s\n", evutil_gai_strerror(err));
        }
        printf("Closing\n");
        bufferevent_free(bev);
        event_base_loopexit(base, NULL);
    }
}

void accept_callback(evutil_socket_t fd, short ev, void *arg) {
    int res;

    struct accept_info *ctx = (struct accept_info *) arg;

    int random_server_idx = rand() % (int) ((*ctx->servers_sockaddrs).size() - 0 + 1);
    sockaddr_in server_sockaddr_in = (*ctx->servers_sockaddrs)[random_server_idx];

    int server_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
//    set_nonblock(server_fd); // Non block socket

    int optval = 1; // Reusable descriptor if we kill app
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));

    res = connect(server_fd, (struct sockaddr *) &server_sockaddr_in, sizeof(struct sockaddr_in));
    if (res)
        throw std::system_error(errno, std::system_category());

    int slave_fd = accept(fd, 0, 0);
    if (slave_fd == -1)
        throw std::system_error(errno, std::system_category());

    set_nonblock(slave_fd);

    struct bufferevent *client_bev;
    struct bufferevent *server_bev;

    struct proxy_info *client_info;
    struct proxy_info *server_info;

    client_bev = bufferevent_socket_new(ctx->base, slave_fd, BEV_OPT_CLOSE_ON_FREE);
    server_bev = bufferevent_socket_new(ctx->base, server_fd, BEV_OPT_CLOSE_ON_FREE);

    // Client
    client_info = (struct proxy_info *) malloc(sizeof(struct proxy_info));
    client_info->other_bev = server_bev;

    bufferevent_setcb(client_bev, read_callback_proxy, write_callback_proxy, event_callback, client_info);

    bufferevent_enable(client_bev, EV_READ | EV_WRITE);

    // Server
    server_info = (struct proxy_info *) malloc(sizeof(struct proxy_info));
    server_info->other_bev = client_bev;

    bufferevent_setcb(server_bev, read_callback_proxy, write_callback_proxy, event_callback, server_info);
    bufferevent_enable(server_bev, EV_READ | EV_WRITE);
}


class Proxy {
    int master_sock_fd;
    std::vector<sockaddr_in> *servers_sockaddrs;

public:
    Proxy();

    ~Proxy();

    void init(ProxyConfig config);
};

void Proxy::init(ProxyConfig config) {
    int ret;

    struct event *accept_event;
    struct event_base *base = event_base_new();

    master_sock_fd = accept_sock(config.master_sockaddr);
    servers_sockaddrs = &config.servers_sockaddrs;

    ret = listen(master_sock_fd, SOMAXCONN);
    if (ret == -1)
        throw std::system_error(errno, std::system_category());

    if (!base)
        throw std::system_error(errno, std::system_category());

    struct accept_info *info;
    info = (struct accept_info *) malloc(sizeof(struct accept_info));
    info->base = base;
    info->servers_sockaddrs = servers_sockaddrs;

    accept_event = event_new(base, master_sock_fd, EV_READ | EV_PERSIST, accept_callback, info);
    event_add(accept_event, NULL);

    /* run event loop */
    ret = event_base_dispatch(base);
    if (ret == -1)
        throw std::system_error(errno, std::system_category());
}

Proxy::Proxy() {
    servers_sockaddrs = new std::vector<sockaddr_in>();
}

Proxy::~Proxy() {

}


int main(int argc, char const *argv[]) {
    ProxyConfig proxy_config;
    proxy_config.from_config_file(argv[1]);

    Proxy proxy;
    proxy.init(proxy_config);

    return 0;
}
