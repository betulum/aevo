#ifndef _ARGS_H
#define _ARGS_H

struct sArgs {
	char *ip;
	int port;
	char *dir;
	int daemon;
};

void printHelp();
int processArgs(int argc, char **argv, struct sArgs *args);

#endif
