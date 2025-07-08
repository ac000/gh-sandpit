#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <poll.h>
#include <sys/epoll.h>

#define exit_err(s)		{ perror(s); exit(EXIT_FAILURE); }

static bool use_rdhup;
static bool noread;

static int counter = -1;

#define prmsg(fmt, ...) printf("[%03d] " fmt, counter, ##__VA_ARGS__);

static ssize_t do_read(int fd)
{
	ssize_t bytes_read;
	char buf[4096];

	bytes_read = read(fd, buf, sizeof(buf));
	switch (bytes_read) {
	case -1:
		exit_err("read");
	case 0:
		prmsg("Read 0 bytes from client. Closing connection...\n");
		return 0;
	default:
		prmsg("Read [%ld] bytes from client\n", bytes_read);
		return bytes_read;
	}
}

static void do_poll_test(int cfd)
{
	int nfds;
	struct pollfd pfd = {};

	pfd.fd = cfd;
	pfd.events = POLLIN;
	if (use_rdhup)
		 pfd.events |= POLLRDHUP;

do_poll:
	counter++;
	prmsg("Waiting for client event...\n");
	nfds = poll(&pfd, 1, -1);
	if (nfds == -1)
		exit_err("poll");

	prmsg("Got event from poll(2)\n");

	if (pfd.revents & POLLHUP) {
		prmsg("Got [POLLHUP] event. Closing connection...\n");
	} else if (pfd.revents & POLLRDHUP) {
		prmsg("Got [POLLRDHUP] event. Closing connection...\n");
	} else {
		ssize_t ret;

		prmsg("Didn't get [POLLHUP/POLLRDHUP]");

		if (noread) {
			prmsg(" and read(2) disabled...\n");
			return;
		} else {
			prmsg(" doing read(2)\n");
		}

		ret = do_read(cfd);
		if (ret > 0)
			goto do_poll;
	}

	close(cfd);
}

static void do_epoll_test(int cfd)
{
	int efd;
	int nfds;
	struct epoll_event ev;
	struct epoll_event events;

	efd = epoll_create(1);
	ev.events = EPOLLIN | EPOLLET;
	if (use_rdhup)
		ev.events |= EPOLLRDHUP;
	epoll_ctl(efd, EPOLL_CTL_ADD, cfd, &ev);

do_epoll_wait:
	counter++;
	prmsg("Waiting for client event...\n");
	nfds = epoll_wait(efd, &events, 1, -1);
	if (nfds == -1)
		exit_err("epoll_wait");

	prmsg("Got event from epoll_wait(2)\n");

	if (events.events & EPOLLHUP) {
		prmsg("Got [EPOLLHUP] event. Closing connection...\n");
	} else if (events.events & EPOLLRDHUP) {
		prmsg("Got [EPOLLRDHUP] event. Closing connection...\n");
	} else {
		ssize_t ret;

		prmsg("Didn't get [EPOLLHUP/EPOLLRDHUP]");

		if (noread) {
			prmsg(" and read(2) disabled...\n");
			return;
		} else {
			prmsg(" doing read(2)\n");
		}

		ret = do_read(cfd);
		if (ret > 0)
			goto do_epoll_wait;
	}

	close(cfd);
}

static int create_listen_socket(void)
{
	int sfd;
	int opt = 1;
	struct sockaddr_in6 addr;

	addr.sin6_family = AF_INET6;
	addr.sin6_port = htons(9009);
	inet_pton(AF_INET6, "::1", &addr.sin6_addr);
//	addr.sin6_addr = in6addr_any;

	sfd = socket(AF_INET6, SOCK_STREAM, 0);
	setsockopt(sfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

	bind(sfd, (struct sockaddr *)&addr, sizeof(addr));
	listen(sfd, 5);

	return sfd;
}

int main(int argc, char *argv[])
{
	int lfd;
	int cfd;

	if (argc == 1) {
		printf("Usage: "
		       "%s <epoll|epollrdhup|poll|pollrdhup> [noread]\n",
		       argv[0]);
		exit(EXIT_FAILURE);
	}

	if (strcmp(argv[1], "epollrdhup") == 0 ||
	    strcmp(argv[1], "pollrdhup") == 0)
		use_rdhup = true;

	if (argc > 2)
		noread = true;

	lfd = create_listen_socket();

	printf("Waiting for client connection...\n");
	cfd = accept(lfd, NULL, NULL);
	printf("Client connected\n");

	if (strncmp(argv[1], "epoll", 5) == 0)
		do_epoll_test(cfd);
	else
		do_poll_test(cfd);

	printf("^C to exit\n");
	pause();

	exit(EXIT_SUCCESS);
}
