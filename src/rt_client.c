#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#include <sys/socket.h>
#include <arpa/inet.h>
#include <poll.h>
#include <unistd.h>

#include <infiniband/verbs.h>

int port = 9871;

void
write_stderr(const char *fmt, ...)
{
	va_list args;

	va_start(args, fmt);
	vfprintf(stderr, fmt, args);
	va_end(args);

	exit(1);
}

int
main(void)
{
	int rc;
	int sockfd;
	struct sockaddr_in raddr;

	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0) {
		write_stderr("could not create socket: %m\n");
	}

	memset(&raddr, 0, sizeof(raddr));
	raddr.sin_family = AF_INET;
	raddr.sin_port = htons(port);
	if (inet_pton(AF_INET, "172.16.100.2", &raddr.sin_addr) <= 0) {
		close(sockfd);
		write_stderr("inet_pton() failed: %m\n");
	}

	rc = connect(sockfd, (struct sockaddr *) &raddr, sizeof(raddr));
	if (rc < 0) {
		close(sockfd);
		write_stderr("could not connect to server: %m\n");
	}

	printf("connect to server\n");

	{
		ssize_t ret;
		char buffer[1024] = { 0 };

		ret = recv(sockfd, buffer, sizeof(buffer), 0);
		if (ret == -1) {
			write_stderr("recv auth message failed: %m\n");
		}
		printf("received: '%s'\n", buffer);

		ret = send(sockfd, "hello", 5, 0);
		if (ret == -1) {
			write_stderr("could not send data to server: %m\n");
		}

		close(sockfd);
	}

	return 0;
}
