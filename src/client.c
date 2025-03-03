#include "common.h"

#include <arpa/inet.h>
#include <getopt.h>
#include <rdma/rsocket.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <unistd.h>

static const char *progname;
static const char *address = "127.0.0.1";
static int port = 8912;
static int use_rdma = 0;

static void usage(void);
static void parse_command_options(int argc, char *argv[]);

int
main(int argc, char *argv[])
{
	int rc;
	int sock_fd;
	char sbuf[1024] = {0};
	struct sockaddr_in raddr;

	progname = get_program(argv[0]);

	if (argc > 1) {
		const char *argv1 = argv[1];

		if (strcmp(argv1, "-?") == 0 || strcmp(argv1, "--help") == 0) {
			usage();
			return 0;
		}
	}

	parse_command_options(argc, argv);

	sock_fd = rs_socket(AF_INET, SOCK_STREAM, 0);
	if (sock_fd < 0) {
		write_stderr("could not create %ssocket: %m\n", use_rdma ? "r" : "");
		exit(EXIT_FAILURE);
	}

	memset(&raddr, 0, sizeof(raddr));
	raddr.sin_family = AF_INET;
	raddr.sin_port = htons(port);

	if (inet_pton(AF_INET, address, &raddr.sin_addr) <= 0) {
		write_stderr("inet_pton(%s) failed: %m\n", address);
		rs_close(sock_fd);
		exit(EXIT_FAILURE);
	}

	rc = rs_connect(sock_fd, (struct sockaddr *) &raddr, sizeof(raddr));
	if (rc < 0) {
		write_stderr("could not connect to %s: %m\n", address);
		rs_close(sock_fd);
		exit(EXIT_FAILURE);
	}

	printf("connect server %s use %ssocket\n", address, use_rdma ? "r" : "");

	while (fgets(sbuf, sizeof(sbuf), stdin)) {
		ssize_t nbytes;
		char rbuf[1024] = {0};
		size_t len = strcspn(sbuf, "\n");

		sbuf[len] = '\0';
		nbytes = rs_send(sock_fd, sbuf, len, 0);
		if (nbytes < 0) {
			write_stderr("send data failed: %m\n");
			rs_close(sock_fd);
			exit(EXIT_FAILURE);
		}

		printf("send message ok\n");

		nbytes = rs_recv(sock_fd, rbuf, sizeof(rbuf) - 1, 0);
		if (nbytes < 0) {
			write_stderr("receive messsage failed: %m\n");
			rs_close(sock_fd);
			exit(EXIT_FAILURE);
		} else {
			rbuf[nbytes] = '\0';
			printf("receive server message: '%s'\n", rbuf);
		}
	}

	rs_close(sock_fd);

	return 0;
}

static void
usage(void)
{
	printf("Usage:\n");
	printf("  %s [OPTIONS...]\n", progname);

	printf("\nOptions:\n");
	printf("  -?, --help        show this page and exit\n");
	printf("  -a, --addr=ADDR   specify the server address (default 127.0.0.1)\n");
    printf("  -p, --port=PORT   specify the server port (default 9812)\n");
    printf("  -r, --rdma        use the RDMA protocol\n");
}

static void
parse_command_options(int argc, char *argv[])
{
	int ch;
	char *argv0;
	const char *short_opts = "a:p:r";
	struct option long_opts[] = {
		{"addr", required_argument, NULL, 'a'},
		{"port", required_argument, NULL, 'p'},
		{"rdma", no_argument, NULL, 'r'},
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
			case 'a':
				address = optarg;
				break;
			case 'p':
				port = atoi(optarg);
				break;
			case 'r':
				use_rdma = 1;
				break;
			default:
				do_advice(progname);
				exit(EXIT_FAILURE);
			}
		}
	}

	argv[0] = argv0;
}
