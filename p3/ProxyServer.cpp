#include "ProxyServer.h"

#include <cstdlib>
#include <system_error>

#include <csignal>

#include <event2/bufferevent.h>
#include <event2/listener.h>
#include <evdns.h>
#include <iostream>


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

    /* create listener event */
    listener_options = LEV_OPT_REUSEABLE | LEV_OPT_CLOSE_ON_FREE | LEV_OPT_LEAVE_SOCKETS_BLOCKING;
    listener = evconnlistener_new_bind(base, on_accept_cb, (void *) base,
                                       listener_options, -1,
                                       (struct sockaddr *) &(config.master_sockaddr), sizeof(config.master_sockaddr));
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


/*void
proxy_read_cb(struct bufferevent *bev, void *ctx) {
    struct proxy_info *inf = (struct proxy_info *) ctx;
    bufferevent_read_buffer(bev, bufferevent_get_output(inf->other_bev));
}


void
proxy_write_cb(struct bufferevent *bev, void *ctx) {
    struct proxy_info *inf = (struct proxy_info *) ctx;
    bufferevent_write_buffer(bev, bufferevent_get_input(inf->other_bev));
}*/


static void
on_sigint_cb(evutil_socket_t fd, short event, void *arg) {
    struct event *sender = (struct event *) arg;
    struct event_base *base = event_get_base(sender);
    event_base_loopbreak(base);
}


static void
on_accept_cb(struct evconnlistener *listener, evutil_socket_t fd,
             struct sockaddr *sa, int socklen, void *arg) {
    struct event_base *base = (struct event_base *) arg;
    std::cout << "Connection." << std::endl;
}


int main(int argc, char const *argv[]) {
    ProxyConfig proxy_config;
    proxy_config.from_config_file(argv[1]);

    ProxyServer proxyServer(proxy_config);
    proxyServer.run_event_loop();

    return 0;
}

