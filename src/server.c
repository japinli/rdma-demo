#include "common.h"

#include <arpa/inet.h>
#include <getopt.h>
#include <rdma/rsocket.h>
#include <stdlib.h>
#include <sys/poll.h>
#include <sys/socket.h>
#include <unistd.h>

#define MAX_CLIENTS 10

static const char *progname;
static int port = 8912;
static int use_rdma = 0;
static int timeout = -1;

static void usage(void);
static void parse_command_options(int argc, char *argv[]);
static void server_main(void);

int
main(int argc, char *argv[])
{
	progname = get_program(argv[0]);

	if (argc > 1) {
		const char *argv1 = argv[1];
		if (strcmp(argv1, "-?") == 0 || strcmp(argv1, "--help") == 0) {
			usage();
			return 0;
		}
	}

	parse_command_options(argc, argv);

	server_main();

	return 0;
}

static void
usage(void)
{
	printf("Usage:\n");
	printf("  %s [OPTIONS...]\n", progname);
	printf("\nOptions:\n");
	printf("  -?, --help            show this page and exit\n");
	printf("  -p, --port=PORT       specify the listen port (default: %d)\n", port);
	printf("  -r, --rdma            use the RDMA protocol\n");
	printf("  -t, --timeout=TIMEOUT specify the poll timeout (default: %d)\n", timeout);
}

static void
parse_command_options(int argc, char *argv[])
{
	int ch;
	char *argv0;
	const char *short_opts = "p:rt:";
	struct option long_opts[] = {
		{"port", required_argument, NULL, 'p'},
		{"rdma", no_argument, NULL, 'r'},
		{"timeout", required_argument, NULL, 't'},
		{NULL, 0, NULL, 0}
	};

	/*
	 * Replace the argv[0] with progname, so we can make the getopt_long()
	 * errors more pretty.
	 */
	argv0 = argv[0];
	argv[0] = (char *) progname;


	while (optind < argc) {
		while ((ch = getopt_long(argc, argv, short_opts, long_opts, NULL)) != -1) {
			switch (ch) {
			case 'p':
				port = atoi(optarg);
				break;
			case 'r':
				use_rdma = 1;
				break;
			case 't':
				timeout = atoi(optarg);
				break;
			default:
				do_advice(progname);
				exit(EXIT_FAILURE);
			}
		}
	}

	/* restore the command argument */
	argv[0] = argv0;
}

static void
server_main(void)
{
	int rc;
	int server_fd;
	int nfds = 0;
	struct sockaddr_in addr;
	struct pollfd fds[MAX_CLIENTS + 1];

	server_fd = rs_socket(AF_INET, SOCK_STREAM, 0);
	if (server_fd < 0) {
		write_stderr("could not create %ssocket: %m\n", use_rdma ? "r" : "");
		exit(EXIT_FAILURE);
	}

	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = INADDR_ANY;
	addr.sin_port = htons(port);

	rc = rs_bind(server_fd, (struct sockaddr *) &addr, sizeof(addr));
	if (rc < 0) {
		write_stderr("could not %sbind socket: %m\n", use_rdma ? "r" : "");
		rs_close(server_fd);
		exit(EXIT_FAILURE);
	}

	rc = rs_listen(server_fd, MAX_CLIENTS);
	if (rc < 0) {
		write_stderr("could not %slisten: %m\n", use_rdma ? "r" : "");
		rs_close(server_fd);
		exit(EXIT_FAILURE);
	}

	printf("listen on port %d using %s...\n",
		   port, use_rdma ? "rsocket" : "socket");

	fds[0].fd = server_fd;
	fds[0].events = POLLIN;
	nfds++;

	while (1) {
		rc = rs_poll(fds, nfds, timeout);
		if (rc < 0) {
			write_stderr("%spoll failed: %m\n", use_rdma ? "r" : "");
			break;
		} else if (rc == 0) {
			printf("timeout, no events\n");
			continue;
		}

		for (int i = 0; i < nfds; i++) {
			if (fds[i].revents & POLLIN) {
				if (i == 0) { /* new connection */
					int client_fd = rs_accept(server_fd, NULL, NULL);
					if (client_fd < 0) {
						write_stderr("%saccept failed: %m\n",
									 use_rdma ? "r" : "");
						continue;
					}

					if (nfds > MAX_CLIENTS) {
						write_stderr("reject new connection (max %d)\n",
									 MAX_CLIENTS);
						rs_close(client_fd);
						continue;
					}

					fds[nfds].fd = client_fd;
					fds[nfds].events = POLLIN;
					nfds++;
					printf("new connection, client_fd = %d, total = %d\n",
						   client_fd, nfds - 1);
				} else { /* client */
					ssize_t nbytes;
					char rbuf[1024] = {0};

					nbytes = rs_recv(fds[i].fd, rbuf, sizeof(rbuf) - 1, 0);
					if (nbytes <= 0) {
						printf("client %d disconnect\n", fds[i].fd);
						rs_close(fds[i].fd);

						fds[i] = fds[nfds - 1];
						nfds--;
						i--;
					} else {
						rbuf[nbytes] = '\0';
						printf("client %d: '%s'\n", fds[i].fd, rbuf);

						/* echo */
						rs_send(fds[i].fd, rbuf, nbytes, 0);
					}
				}
			}
		}
	}

	for (int i = 0; i < nfds; i++) {
		rs_close(fds[i].fd);
	}
}
