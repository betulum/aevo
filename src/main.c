#include"arg.h"
#include"sig.h"
#include"worker.h"
#include<stdlib.h>
#include<unistd.h>
#include<stdio.h>
#include<signal.h>
//#include<time.h>
#include<stddef.h>
#include<pthread.h>
#include<ev.h>

int main(int argc, char **argv)
{
	struct sThreadData tdata;
	if (processArgs(argc, argv, &tdata.args) < 0) {
		printHelp();
		return EXIT_FAILURE;
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
	sigaddset(&sigset, SIGRTMIN);
	if (pthread_sigmask(SIG_BLOCK, &sigset, NULL) != 0) {
		perror("pthread_sigmask");
		return EXIT_FAILURE;
	}

	struct signalData sdata;
	sdata.ncpu = sysconf(_SC_NPROCESSORS_ONLN); 
	sdata.wloop = malloc(sdata.ncpu * sizeof(struct ev_loop*));
	sdata.wasync = malloc(sdata.ncpu * sizeof(struct mev_async *));
	pthread_t *worker = malloc(sdata.ncpu * sizeof(pthread_t));
	
	for(int i=0; i<sdata.ncpu; i++) {
		tdata.loop = sdata.wloop[i] = ev_loop_new(EVFLAG_NOSIGMASK);
		sdata.wasync[i] = malloc(sizeof(struct mev_async));
		tdata.async_watcher = (struct ev_async *)sdata.wasync[i];
		if (pthread_create(worker + i, NULL, worker_function, &tdata) != 0) {
			perror("pthread_create");
			return EXIT_FAILURE;
		}
		usleep(100);
	}
	pthread_sigmask(SIG_UNBLOCK, &sigset, NULL);
	if (start_timer(250, SIGRTMIN) < 0) {
		perror("timer_create or timer_settime");
		return EXIT_FAILURE;
	}

	struct ev_loop *mainloop = ev_default_loop(0);
	ev_signal_init(&sdata.sigint_watcher, sigint_cb, SIGINT);
	ev_signal_init(&sdata.sigterm_watcher, sigterm_cb, SIGTERM);
	ev_signal_init(&sdata.sigtimer_watcher, sigtimer_cb, SIGRTMIN);
	ev_signal_start(mainloop, &sdata.sigint_watcher);
	ev_signal_start(mainloop, &sdata.sigterm_watcher);
	ev_signal_start(mainloop, &sdata.sigtimer_watcher);

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

