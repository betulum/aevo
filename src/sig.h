#ifndef _SIG_H
#define _SIG_H

#include<ev.h>

struct mev_async {
	struct ev_async watcher;
	int request;
};

struct signalData {
	struct ev_signal sigint_watcher;
	struct ev_signal sigterm_watcher;
	struct ev_signal sigtimer_watcher;
	int ncpu;
	struct ev_loop **wloop;
	struct mev_async **wasync;
};

int start_timer(int period_ms, int sig);
void send_term(struct ev_loop *loop, struct signalData *sdata);
void sigint_cb(struct ev_loop *loop, struct ev_signal *watcher, int revents);
void sigterm_cb(struct ev_loop *loop, struct ev_signal *watcher, int revents);
void sigtimer_cb(struct ev_loop *loop, struct ev_signal *watcher, int revents);

#endif
