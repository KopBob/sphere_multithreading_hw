//
// Create event_base
//
// without config
struct event_base *event_base_new(void);

// with config
struct event_config *event_config_new(void);
struct event_base *event_base_new_with_config(const struct
event_config *cfg);
void event_config_free(struct event_config *cfg);

// deallocating
void event_base_free(struct event_base *base);


//
// Run Event Loop
//
#define EVLOOP_ONCE 0x01
#define EVLOOP_NONBLOCK 0x02
#define EVLOOP_NO_EXIT_ON_EMPTY 0x04
int event_base_loop(struct event_base *base, int flags); // No flags
int event_base_dispatch(struct event_base *base); // Stop
int event_base_loopexit(struct event_base *base, const struct timeval *tv);
int event_base_loopbreak(struct event_base *base);

// Is stopping?
int event_base_got_exit(struct event_base *base);
int event_base_got_break(struct event_base *base);


//
// Create Event
//
#define EV_TIMEOUT 0x01
#define EV_READ 0x02
#define EV_WRITE 0x04
#define EV_SIGNAL 0x08
#define EV_PERSIST 0x10
#define EV_ET 0x20
// callback prototype
typedef void (*event_callback_fn)(evutil_socket_t, short, void *);

struct event *event_new(struct event_base *base,
                        evutil_socket_t fd,
                        short what, // Events ||
                        event_callback_fn cb,
                        void *arg); // event_self_cbarg() if event struct needed
void event_free(struct event *event);

// Example
void cb_func(evutil_socket_t fd, short what, void *arg) {
    const char *data = arg;
    printf("Got an event on socket %d:%s%s%s%s [%s]", 
        (int) fd,
        (what&EV_TIMEOUT) ? " timeout" : "",
        (what&EV_READ) ? " read" : "",
        (what&EV_WRITE) ? " write" : "",
        (what&EV_SIGNAL) ? " signal" : "",
        data);
}


// Example
void main_loop(evutil_socket_t fd1, evutil_socket_t fd2) {
    struct event *ev1, *ev2;
    struct timeval five_seconds = {5,0};
    struct event_base *base = event_base_new();
        /* The caller has already set up fd1, fd2 somehow, and
    make them nonblocking. */
    ev1 = event_new(base, fd1, EV_TIMEOUT|EV_READ|EV_PERSIST, cb_func, (char*)"Reading event");
    ev2 = event_new(base, fd2, EV_WRITE|EV_PERSIST, cb_func, (char*)"Writing event");

    event_add(ev1, &five_seconds);
    event_add(ev2, NULL);

    event_base_dispatch(base);
}

// Timeout-only events.
#define evtimer_new(base, callback, arg) \
    event_new((base), -1, 0, (callback), (arg))
#define evtimer_add(ev, tv) \
    event_add((ev),(tv))
#define evtimer_del(ev) \
    event_del(ev)
#define evtimer_pending(ev, tv_out) \
    event_pending((ev), EV_TIMEOUT, (tv_out))
#define evtimer_assign(event, base, callback, arg)
    event_assign(event, base, -1, 0, callback, arg)

// Signal events.
#define evsignal_new(base, signum, cb, arg) \
    event_new(base, signum, EV_SIGNAL|EV_PERSIST, cb, arg)
#define evsignal_add(ev, tv) \
    event_add((ev),(tv))
#define evsignal_del(ev) \
    event_del(ev)
#define evsignal_pending(ev, what, tv_out) \
    event_pending((ev), (what), (tv_out))
#define evsignal_assign(event, base, signum, callback, arg)
    event_assign(event, base, signum, EV_SIGNAL|EV_PERSIST,
callback, arg)



// 
// bufferevent
// 

// Сразу вешаем буффер на сокет
struct bufferevent *bufferevent_socket_new(
    struct event_base *base,
    evutil_socket_t fd,
    enum bufferevent_options options
)

int bufferevent_socket_connect(
    struct bufferevent *bev,
    struct sockaddr *address,
    int addrlen
);

int bufferevent_socket_connect_hostname(
    struct bufferevent *bev,
    struct evdns_base *dns_base,
    int family,
    const char *hostname,
    int port
);

int bufferevent_socket_get_dns_error(struct bufferevent *bev);

void bufferevent_free(struct bufferevent *bev);


// BEV_EVENT_READING
// BEV_EVENT_WRITING
// BEV_EVENT_ERROR
// BEV_EVENT_TIMEOUT
// BEV_EVENT_EOF
// BEV_EVENT_CONNECTED


// data callback
typedef void (*bufferevent_data_cb)(struct bufferevent *bev, void *ctx);

// event callback
typedef void (*bufferevent_event_cb)(struct bufferevent *bev, short events, void *ctx);

// set up event and callbacks on bufferevent
void bufferevent_setcb(struct bufferevent *bufev,
    bufferevent_data_cb readcb,
    bufferevent_data_cb writecb,
    bufferevent_event_cb eventcb,
    void *cbarg
);

// get callbacks
void bufferevent_getcb(struct bufferevent *bufev,
    bufferevent_data_cb *readcb_ptr,
    bufferevent_data_cb *writecb_ptr,
    bufferevent_event_cb *eventcb_ptr,
    void **cbarg_ptr
);

// 
void bufferevent_enable(struct bufferevent *bufev, short events);
void bufferevent_disable(struct bufferevent *bufev, short events);

short bufferevent_get_enabled(struct bufferevent *bufev);


// Get internal input and output buffers
// this feature can be used for redirecting
// output from one socket to input of another socket

//
struct evbuffer *bufferevent_get_input(struct bufferevent *bufev);
struct evbuffer *bufferevent_get_output(struct bufferevent *bufev);


int bufferevent_write(struct bufferevent *bufev, const void *data, size_t size);
int bufferevent_write_buffer(struct bufferevent *bufev, struct evbuffer *buf);

size_t bufferevent_read(struct bufferevent *bufev, void *data, size_t size);
int bufferevent_read_buffer(struct bufferevent *bufev, struct evbuffer *buf);

void bufferevent_set_timeouts(
    struct bufferevent *bufev,
    const struct timeval *timeout_read,
    const struct timeval *timeout_write
);

int bufferevent_flush(struct bufferevent *bufev,
    short iotype,
    enum bufferevent_flush_mode state
);





// 0
struct event_base *event_base_new(void);


// 1
struct bufferevent *bufferevent_socket_new(
    struct event_base *base,
    evutil_socket_t fd, // -1 for bufferevent_socket_connect
    enum bufferevent_options options
)


// 2
int bufferevent_socket_connect(
    struct bufferevent *bev,
    struct sockaddr *address,
    int addrlen
);


// 3
void bufferevent_setcb(
    struct bufferevent *bufev,
    bufferevent_data_cb readcb,
    bufferevent_data_cb writecb,
    bufferevent_event_cb eventcb,
    void *cbarg // store severs bufferevent here
);

// 3.1
typedef void (*bufferevent_data_cb)( // For EV_READ and for EV_WRITE
    struct bufferevent *bev,
    void *ctx // Get server bufferevent from ctx
);

typedef void (*bufferevent_event_cb)(
    struct bufferevent *bev,
    short events,
    void *ctx
);

struct proxy_info {
    struct bufferevent *other_bev;
};
void read_callback_proxy(struct bufferevent *bev, void *ctx) {
    struct proxy_info *inf = ctx;

    bufferevent_read_buffer(bev,
        bufferevent_get_output(inf->other_bev));
}

void write_callback_proxy(struct bufferevent *bev, void *ctx) {
    struct proxy_info *inf = ctx;

    bufferevent_write_buffer(bev,
        bufferevent_get_input(inf->other_bev));
}
