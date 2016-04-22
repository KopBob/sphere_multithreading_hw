#include "EchoServer.h"

#include <cstdlib>
#include <zconf.h>
#include <err.h>
#include <system_error>

#include <csignal>

#include <event2/bufferevent.h>
#include <event2/listener.h>

#include "ProxyConfig.h"


void EchoServer::run_event_loop(struct sockaddr_in master_sockaddr) {
    int ret;
    struct event_base *base;
    struct evconnlistener *listener;
    unsigned int listener_options;
    struct event *signal_event;
    sockaddr_in sin;

    /* enable libevent debug mode */
    event_enable_debug_mode();

    /* create event base */
    base = event_base_new();
    if (base == NULL)
        throw std::system_error(errno, std::system_category());

    /* create listener event */
    listener_options = LEV_OPT_REUSEABLE | LEV_OPT_CLOSE_ON_FREE | LEV_OPT_LEAVE_SOCKETS_BLOCKING;
    listener = evconnlistener_new_bind(base, on_accept_cb, (void *) base,
                                       listener_options, -1,
                                       (struct sockaddr *) &master_sockaddr, sizeof(master_sockaddr));
    if (listener == NULL)
        throw std::system_error(errno, std::system_category());

    /* create SIGINT event */
    signal_event = event_new(base, SIGINT, EV_SIGNAL | EV_PERSIST, on_sigint_cb, event_self_cbarg());
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

    libevent_global_shutdown();
}


static void
on_sigint_cb(evutil_socket_t fd, short event, void *arg) {
    struct event *sender = (struct event *) arg;
    struct event_base *base = event_get_base(sender);
    event_base_loopbreak(base);
}


static void
buffered_on_read_cb(struct bufferevent *bev, void *arg) {
    bufferevent_write_buffer(bev, bev->input);
}


static void
buffered_on_write_cb(struct bufferevent *bev, void *arg) {

}


static void
buffered_on_event_cb(struct bufferevent *bev, short events, void *arg) {
    struct client *client = (struct client *) arg;

    if (events & EVBUFFER_EOF) {
        printf("Client disconnected.\n");
    }
    else {
        warn("Client socket error, disconnecting.\n");
    }
    bufferevent_free(client->buf_ev);
    close(client->fd);
    free(client);
}


static void
on_accept_cb(struct evconnlistener *listener, evutil_socket_t fd,
             struct sockaddr *sa, int socklen, void *user_data) {
    struct event_base *base = (struct event_base *) user_data;
    struct bufferevent *bev;
    struct client *client;

    client = (struct client *) calloc(1, sizeof(struct client));
    client->fd = fd;
    client->buf_ev = bufferevent_socket_new(base, fd, BEV_OPT_CLOSE_ON_FREE);
    if (client->buf_ev == NULL)
        throw std::system_error(errno, std::system_category());

    bufferevent_setcb(client->buf_ev, buffered_on_read_cb, NULL, buffered_on_event_cb, client);
    bufferevent_enable(client->buf_ev, EV_READ);
}


int main(int argc, char const *argv[]) {
    ProxyConfig proxy_config;
    proxy_config.from_config_file(argv[1]);

    EchoServer echoServer;
    echoServer.run_event_loop(proxy_config.master_sockaddr);

    return 0;
}
