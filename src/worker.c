#include<ev.h>
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<sys/stat.h>
#include<sys/types.h>
#include<fcntl.h>
#include"arg.h"
#include"sig.h"
#include"worker.h"

void ok(int sock, int size) {
	char buf[64];
	sprintf(buf, "HTTP/1.0 200 OK\r\n");
	send(sock, buf, strlen(buf), MSG_NOSIGNAL);
	sprintf(buf, "Content-Type: text/html\r\n");
	send(sock, buf, strlen(buf), MSG_NOSIGNAL);	
	sprintf(buf, "Content-Length: %d\r\n\r\n", size);
	send(sock, buf, strlen(buf), MSG_NOSIGNAL);	
}

void fail(int sock) {
	char buf[64];
	sprintf(buf, "HTTP/1.0 400 BAD REQUEST\r\n");
	send(sock, buf, strlen(buf), MSG_NOSIGNAL);
	sprintf(buf, "Content-Type: text/html\r\n\r\n");
	send(sock, buf, strlen(buf), MSG_NOSIGNAL);	
}

void nofile(int sock, const char *path) {
	char buf[64];
	sprintf(buf, "HTTP/1.0 404 RESOURCE NOT AVAILABLE\r\n");
	send(sock, buf, strlen(buf), MSG_NOSIGNAL);
	sprintf(buf, "Content-Type: text/html\r\n");
	send(sock, buf, strlen(buf), MSG_NOSIGNAL);	
	sprintf(buf, "Content-Length: 0\r\n\r\n");
	send(sock, buf, strlen(buf), MSG_NOSIGNAL);	
}

void senddata(const char *path, int sock) {
	char rpath[256];
	sprintf(rpath, ".%s", path);
	char *idx = strchr(rpath, '?');
	if (idx != NULL)
		*idx='\0';

	int fd = open(rpath, O_RDONLY);
	if (fd < 0) {
		nofile(sock, path);
	} else {
		char buf[1024*1024];	
		int bytes = read(fd, buf, sizeof(buf));
		if (bytes < 0) 
			nofile(sock, path);
		else {
			ok(sock, bytes);
			int ret = 0, send_bytes = 0;
			while (send_bytes < bytes && ret >= 0 )
				send_bytes += ret = send(sock, buf, bytes, MSG_NOSIGNAL);
		}
		close(fd);
	}
}

void read_cb(struct ev_loop *loop, struct ev_io *watcher, int revents)
{
	char buf[1024];
	ssize_t rec = recv(watcher->fd, buf, 1024, MSG_NOSIGNAL);
	if (rec < 0) { 
		return;
	} else if(rec > 0) {
		char path[256];
		int res = sscanf(buf, "GET %s", path);
 		if (res == 0 || res == EOF) 
			fail(watcher->fd);
		else { 
			senddata(path, watcher->fd);
			//send(watcher->fd, buf, rec, MSG_NOSIGNAL);
		}
	}
        close(watcher->fd);
	ev_io_stop(loop, watcher);
	free(watcher);
}

void accept_cb(struct ev_loop *loop, struct ev_io *watcher, int revents)
{
	int sock = accept(watcher->fd, 0, 0);
	struct ev_io *w_client = malloc(sizeof(struct ev_io));
	
	ev_io_init(w_client, read_cb, sock, EV_READ);
	ev_io_start(loop, w_client);
}

void async_cb(struct ev_loop *loop, struct ev_async *watcher, int revents)
{
	struct mev_async *async_data = (struct mev_async *)watcher;
	switch (async_data->request) {
	case 0:
		ev_break(loop, EVBREAK_ALL);
		break;
	case 1:
		// send data
		break;
	}
}

void *worker_function(void *arg) 
{
	struct sThreadData *tdata = (struct sThreadData *)arg;
	int sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	
	struct sockaddr_in addr;
	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(tdata->args.port);
	addr.sin_addr.s_addr = inet_addr(tdata->args.ip);
	bind(sock, (struct sockaddr *) &addr, sizeof(addr));
	listen(sock, SOMAXCONN);

	struct ev_loop *loop = tdata->loop;//ev_loop_new(EVFLAG_NOSIGMASK);
	struct ev_io w_accept;
	ev_io_init(&w_accept, accept_cb, sock, EV_READ);
	ev_io_start(loop, &w_accept);
	
	//struct ev_async w_exit;
	ev_async_init(tdata->async_watcher, async_cb);
	ev_async_start(loop, tdata->async_watcher);
	
	ev_loop(loop, 0);
	
	if (!tdata->args.daemon) printf("ev_loop finished\n");
	//ev_loop_destroy(loop);
	return NULL;
}

