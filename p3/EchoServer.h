#ifndef P3_ECHOSERVER_H
#define P3_ECHOSERVER_H

#include <netinet/in.h>
#include <event2/event.h>


struct client {
    evutil_socket_t fd;
    struct bufferevent *buf_ev;
};


class EchoServer {
public:
    void run_event_loop(struct sockaddr_in master_sockaddr);
};


static void on_sigint_cb(evutil_socket_t, short, void *);

static void on_accept_cb(struct evconnlistener *, evutil_socket_t, struct sockaddr *, int, void *);

static void buffered_on_read_cb(struct bufferevent *, void *);

static void buffered_on_write_cb(struct bufferevent *, void *);

static void buffered_on_event_cb(struct bufferevent *, short, void *);


#endif //P3_ECHOSERVER_H
