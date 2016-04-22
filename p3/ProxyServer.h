//
// Created by Bors Kopin on 12.04.16.
//

#ifndef P3_PROXYSERVER_H
#define P3_PROXYSERVER_H

#include <evdns.h>
#include "event2/event.h"

#include "ProxyConfig.h"


struct accept_ctx {
    struct event_base *base;
    std::vector<sockaddr_in> *servers_sockaddrs;
};


class ProxyServer {
    ProxyConfig config;

public:
    ProxyServer(ProxyConfig _config);

    void run_event_loop();

};

static void on_sigint_cb(evutil_socket_t, short, void *);

static void on_accept_cb(struct evconnlistener *, evutil_socket_t, struct sockaddr *, int, void *);

static void buffered_on_event_cb(struct bufferevent *bev, short events, void *arg);

static void read_callback_proxy(struct bufferevent *bev, void *ctx);


#endif //P3_PROXYSERVER_H
