#include"sig.h"
#include<time.h>
#include<signal.h>
#include<stddef.h>

int start_timer(int period_ms, int sig) {
	timer_t timerid;
	struct sigevent sev;
	sev.sigev_notify = SIGEV_SIGNAL;
	sev.sigev_signo = sig;
	sev.sigev_value.sival_ptr = &timerid;
	if (timer_create(CLOCK_MONOTONIC, &sev, &timerid) < 0) 
		return -1;

	struct itimerspec its;
	its.it_value.tv_sec = 0;
	its.it_value.tv_nsec = period_ms*1000000;
	its.it_interval.tv_sec = its.it_value.tv_sec;
	its.it_interval.tv_nsec = its.it_value.tv_nsec;
	if (timer_settime(timerid, 0, &its, NULL) < 0)
		return -1;
	return 0;
}

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
