#include"sig.h"
#include<stddef.h>

void send_term(struct ev_loop *loop, struct signalData *sdata) {
	for (int i=0; i<sdata->ncpu ; i++) {
		sdata->wasync[i]->request = 0;
		ev_async_send(sdata->wloop[i], (struct ev_async *)sdata->wasync[i]);
	}
	ev_break(loop, EVBREAK_ALL);
}
void sigint_cb(struct ev_loop *loop, struct ev_signal *watcher, int revents) {
	struct signalData *sdata = (struct signalData *)((char *)watcher-offsetof(struct signalData, sigint_watcher));
	send_term(loop, sdata);
}


void sigterm_cb(struct ev_loop *loop, struct ev_signal *watcher, int revents) {
	struct signalData *sdata = (struct signalData *)((char *)watcher-offsetof(struct signalData, sigterm_watcher));
	send_term(loop, sdata);
}

void sigtimer_cb(struct ev_loop *loop, struct ev_signal *watcher, int revents) {
	struct signalData *sdata = (struct signalData *)((char *)watcher-offsetof(struct signalData, sigtimer_watcher));
	for (int i=0; i<sdata->ncpu ; i++) {
		sdata->wasync[i]->request = 1;
		ev_async_send(sdata->wloop[i], (struct ev_async *)sdata->wasync[i]);
	}
}	
