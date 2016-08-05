#ifndef _WORKER_H
#define _WORKER_H

struct sArgs;
struct ev_loop;
struct ev_async;
struct sThreadData {
	struct sArgs args;
	struct ev_loop *loop;
	struct ev_async *async_watcher;
};
void *worker_function(void *);

#endif
