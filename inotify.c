#include <errno.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/inotify.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include "hslog/hslog.h"

char* __dirname;
char* __filename;	

static void handle_events(int fd, char *filename)
{

	char buf[4096]
	    __attribute__ ((aligned(__alignof__(struct inotify_event))));

	const struct inotify_event *event;
	int i;
	ssize_t len;
	char *ptr;

	for (;;) {

		len = read(fd, buf, sizeof buf);
		if (len == -1 && errno != EAGAIN) {
			perror("read");
			exit(EXIT_FAILURE);
		}

		if (len <= 0)
			break;

		for (ptr = buf; ptr < buf + len;
		    	ptr += sizeof(struct inotify_event) + event->len) {
			event = (const struct inotify_event *) ptr;
			/* Print event type */
			//printf("Eve %x\n",event->mask);
			if (event->len) {
				if (event->mask & IN_CLOSE_WRITE)
					if (!strcmp(filename, event->name)) {
						printf("IN_CLOSE_WRITE:  %s\n", event->name);
    						hslog_init( UDP_SERVICE_LOG , "/etc/" SERVICE_LOG_CFG, 0, 1);
				}
			}
		}
	}
}

int watch_file(char *file, char *filename)
{
	char buf;
	int fd, i, poll_num;
	int wd;
	nfds_t nfds;
	struct pollfd fds[1];

	/* Create the file descriptor for accessing the inotify API */

	fd = inotify_init1(IN_NONBLOCK);
	if (fd == -1) {
		perror("inotify_init1");
		exit(EXIT_FAILURE);
	}

	wd = inotify_add_watch(fd, file, IN_MODIFY | IN_CLOSE | IN_OPEN);
	if (wd == -1) {
		fprintf(stderr, "Cannot watch '%s'\n", file);
		perror("inotify_add_watch");
		return -1;
	}

	nfds = 1;
	fds[0].fd = fd;
	fds[0].events = POLLIN;

	/* Wait for events and/or terminal input */

	printf("Listening for events.\n");
	while (1) {
		poll_num = poll(fds, nfds, -1);
		if (poll_num == -1) {
			if (errno == EINTR)
				continue;
			perror("poll");
			exit(EXIT_FAILURE);
		}
	
		if (poll_num > 0) {
			if (fds[0].revents & POLLIN) {
				handle_events(fd, filename);
			}
		}
	}
	printf("Listening for events stopped.\n");
	close(fd);
	exit(EXIT_SUCCESS);
}

void* inotify_service(void *arg)
{
	watch_file(__dirname, __filename);
}

int start_inotify_service(char *dir,char *file)
{
    __dirname = dir;
    __filename = file;	
   pthread_t tid;

    if (!pthread_create(&tid, NULL, inotify_service, NULL))
		return 0;
    return -1;
}
