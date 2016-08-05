#include"arg.h"
#include"worker.h"
#include<stdlib.h>
#include<stdio.h>
#include<signal.h>
#include<unistd.h>
#include<stddef.h>
#include<pthread.h>
#include<ev.h>

struct signalData {
	struct ev_signal sigint_watcher;
	struct ev_signal sigterm_watcher;
	int ncpu;
	struct ev_loop **wloop;
	struct ev_async **wasync;
};

void send_term(struct ev_loop *loop, struct signalData *sdata) {
	for (int i=0; i<sdata->ncpu ; i++)
		ev_async_send(sdata->wloop[i], sdata->wasync[i]);
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


int main(int argc, char **argv)
{
	struct sThreadData tdata;
	if (processArgs(argc, argv, &tdata.args) < 0) {
		printHelp();
		exit(EXIT_FAILURE);
	}

	if (chdir(tdata.args.dir) < 0) {
		perror("chdir");
		return EXIT_FAILURE;
	}	
	
	if (tdata.args.daemon) {
		if (daemon(1,0) < 0) {
			perror("daemon");
			return EXIT_FAILURE;
		}
	}

	sigset_t sigset;
	sigemptyset(&sigset);
	sigaddset(&sigset, SIGINT);
	sigaddset(&sigset, SIGTERM);
	if (pthread_sigmask(SIG_BLOCK, &sigset, NULL) != 0) {
		perror("pthread_sigmask");
		return EXIT_FAILURE;
	}

	struct signalData sdata;
	sdata.ncpu = sysconf(_SC_NPROCESSORS_ONLN); 
	sdata.wloop = malloc(sdata.ncpu * sizeof(struct ev_loop*));
	sdata.wasync = malloc(sdata.ncpu * sizeof(struct ev_async*));
	pthread_t *worker = malloc(sdata.ncpu * sizeof(pthread_t));
	
	for(int i=0; i<sdata.ncpu; i++) {
		tdata.loop = sdata.wloop[i] = ev_loop_new(EVFLAG_NOSIGMASK);
		tdata.async_watcher = sdata.wasync[i] = malloc(sizeof(ev_async));
		if (pthread_create(worker + i, NULL, worker_function, &tdata) != 0) {
			perror("pthread_create");
			return EXIT_FAILURE;
		}
		usleep(100);
	}
	pthread_sigmask(SIG_UNBLOCK, &sigset, NULL);
	struct ev_loop *mainloop = ev_default_loop(0);
	ev_signal_init(&sdata.sigint_watcher, sigint_cb, SIGINT);
	ev_signal_init(&sdata.sigterm_watcher, sigterm_cb, SIGTERM);
	ev_signal_start(mainloop, &sdata.sigint_watcher);
	ev_signal_start(mainloop, &sdata.sigterm_watcher);

	ev_loop(mainloop, 0);
	
	if (!tdata.args.daemon) 
		printf("Mainloop finished\n");
	
	for(int i=0; i<sdata.ncpu; i++)
		pthread_join(worker[i], NULL);
	
	free(worker);
	for (int i=0; i<sdata.ncpu; i++) {
		free(sdata.wasync[i]);
		ev_loop_destroy(sdata.wloop[i]);
	}
	free(sdata.wasync);
	free(sdata.wloop);
	return EXIT_SUCCESS;
}	

