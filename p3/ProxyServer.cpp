#include "ProxyServer.h"

#include <err.h>
#include <iostream>
#include <csignal>

#include <event2/listener.h>
#include <event2/bufferevent.h>
#include <event2/bufferevent_compat.h>


ProxyServer::ProxyServer(ProxyConfig _config) {
    config = _config;
}


void ProxyServer::run_event_loop() {
    int ret;

    struct event_base *base;
    struct evconnlistener *listener;
    unsigned int listener_options;
    struct event *signal_event;

    /* enable libevent debug mode */
    event_enable_debug_mode();

    /* create event base */
    base = event_base_new();
    if (base == NULL)
        throw std::system_error(errno, std::system_category());

    struct accept_ctx *accept_arg = new accept_ctx;
    accept_arg->base = base;
    accept_arg->servers_sockaddrs = &config.servers_sockaddrs;

    /* create listener event */
    listener_options = LEV_OPT_REUSEABLE | LEV_OPT_CLOSE_ON_FREE;
    listener = evconnlistener_new_bind(base, on_accept_cb, (void *) accept_arg,
                                       listener_options, -1,
                                       (struct sockaddr *) &(config.master_sockaddr),
                                       sizeof(config.master_sockaddr));
    if (listener == NULL)
        throw std::system_error(errno, std::system_category());

    /* create SIGINT event */
    signal_event = event_new(base, SIGINT, EV_SIGNAL | EV_PERSIST,
                             on_sigint_cb, event_self_cbarg());
    if (!signal_event || event_add(signal_event, NULL) < 0)
        throw std::system_error(errno, std::system_category());

    /* run event loop */
    ret = event_base_dispatch(base);
    if (ret == -1)
        throw std::system_error(errno, std::system_category());

    /* clean up */
    evconnlistener_free(listener);
    event_free(signal_event);
    event_base_free(base);
    delete accept_arg;

    libevent_global_shutdown();
}


static void
on_sigint_cb(evutil_socket_t fd, short event, void *arg) {
    struct event *sender = (struct event *) arg;
    struct event_base *base = event_get_base(sender);
    event_base_loopbreak(base);
}


static void
read_callback_proxy(struct bufferevent *bev, void *ctx) {
    struct bufferevent *other_bev = (struct bufferevent *) ctx;
    bufferevent_read_buffer(bev, bufferevent_get_output(other_bev));
}


static void
buffered_on_event_cb(struct bufferevent *bev, short events, void *arg) {
    if (events & EVBUFFER_EOF) {
        printf("Client disconnected.\n");
    }
    else {
//        throw std::system_error(errno, std::system_category());
        warn("Client socket error, disconnecting.\n");
    }

    bufferevent_free(bev);
    bufferevent_free((struct bufferevent *) arg);
}


static void
on_accept_cb(struct evconnlistener *listener, evutil_socket_t client_socket_fd,
             struct sockaddr *sa, int socklen, void *arg) {
    struct accept_ctx *ctx;
    int ret;
    int server_sock_fd;
    unsigned int server_idx;

    ctx = (struct accept_ctx *) arg;

    /* create sever socket */
    server_sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    ret = evutil_make_socket_nonblocking(server_sock_fd);
    if (ret == -1)
        throw std::system_error(errno, std::system_category());

    /* connect to random server */
    server_idx = (unsigned int) (rand() % ctx->servers_sockaddrs->size());
    ret = connect(server_sock_fd, (struct sockaddr *) &ctx->servers_sockaddrs->at(server_idx),
                  sizeof(sockaddr_in));
//    if (ret == -1)
//        throw std::system_error(errno, std::system_category());

    struct bufferevent *client_buf_ev = bufferevent_socket_new(ctx->base, client_socket_fd, BEV_OPT_CLOSE_ON_FREE);
    if (client_buf_ev == NULL)
        throw std::system_error(errno, std::system_category());

    struct bufferevent *server_buf_ev = bufferevent_socket_new(ctx->base, server_sock_fd, BEV_OPT_CLOSE_ON_FREE);
    if (server_buf_ev == NULL)
        throw std::system_error(errno, std::system_category());

    bufferevent_setcb(client_buf_ev, read_callback_proxy, NULL, buffered_on_event_cb, (void *) server_buf_ev);
    bufferevent_enable(client_buf_ev, EV_READ);


    bufferevent_setcb(server_buf_ev, read_callback_proxy, NULL, buffered_on_event_cb, (void *) client_buf_ev);
    bufferevent_enable(server_buf_ev, EV_READ);

    std::cout << "Connection." << std::endl;
}


int main(int argc, char const *argv[]) {
    ProxyConfig proxy_config;
    proxy_config.from_config_file(argv[1]);

    ProxyServer proxyServer(proxy_config);
    proxyServer.run_event_loop();

    return 0;
}

