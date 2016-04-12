//
// Created by Bors Kopin on 12.04.16.
//

#ifndef P3_PROXYSERVER_H
#define P3_PROXYSERVER_H

#include "event2/event.h"

#include "ProxyConfig.h"


class ProxyServer {
    ProxyConfig config;

public:
    ProxyServer(ProxyConfig _config);

    void run_event_loop();

};

void proxy_read_cb(struct bufferevent *bev, void *ctx);

void proxy_write_cb(struct bufferevent *bev, void *ctx);

static void on_sigint_cb(evutil_socket_t, short, void *);

static void on_accept_cb(struct evconnlistener *, evutil_socket_t, struct sockaddr *, int, void *);


#endif //P3_PROXYSERVER_H
