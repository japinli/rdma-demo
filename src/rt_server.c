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
int timeout = -1;

typedef struct rdma_param
{
	uint32_t qp_num;
	uint16_t lid;
	union ibv_gid gid;
	char unique_id[32];
} rdma_param_t;

static struct ibv_context *get_rdma_param(rdma_param_t *param);

void
write_stderr(const char *fmt, ...)
{
	va_list args;

	va_start(args, fmt);
	vfprintf(stderr, fmt, args);
	va_end(args);

	exit(1);
}

void
rdma_main(int sockfd)
{
	ssize_t rc;
	char buffer[1024] = { 0 };
	char auth[] = "auth message";
	struct ibv_context *ctx = NULL;
	rdma_param_t param;

	rc = send(sockfd, auth, sizeof(auth) - 1, 0);
	if (rc == -1) {
		write_stderr("send auth message failed: %m\n");
	}

	rc = recv(sockfd, buffer, sizeof(buffer), 0);
	if (rc == -1) {
		write_stderr("recv message failed: %m\n");
	}

	printf("received: '%s'\n", buffer);

	ctx = get_rdma_param(&param);
	if (ctx == NULL) {
		close(sockfd);
		exit(-1);
	}

	/* create protection domain and completion queue */
	{
		struct ibv_pd *pd;
		struct ibv_cq *cq;
		struct ibv_qp *qp;
		struct ibv_qp_init_attr qp_attr = {
			.qp_type = IBV_QPT_RC,
			.cap = { .max_send_wr = 16, .max_recv_wr = 16},
		};

		pd = ibv_alloc_pd(ctx);
		if (pd == NULL) {
			close(sockfd);
			ibv_close_device(ctx);
			exit(-1);
		}

		cq = ibv_create_cq(ctx, 128, NULL, NULL, 0);
		if (cq == NULL) {
			close(sockfd);
			ibv_dealloc_pd(pd);
			ibv_close_device(ctx);
			exit(-1);
		}

		qp_attr.send_cq = cq;
		qp_attr.recv_cq = cq;

		qp = ibv_create_qp(pd, &qp_attr);
		if (qp == NULL) {
			close(sockfd);
			ibv_dealloc_pd(pd);
			ibv_close_device(ctx);
			exit(-1);
		}

		param.qp_num = qp->qp_num;
	}

	printf("RDMA parameters: qp_num: %u, lid: %u\n", param.qp_num, param.lid);

	close(sockfd);
	exit(0);
}

void
server_main(void)
{
	int rc;
	int fd;
	int nfds = 0;
	struct pollfd fds[10];
	struct sockaddr_in addr;

	fd = socket(AF_INET, SOCK_STREAM, 0);
	if (fd < 0) {
		write_stderr("could not create socket: %m\n");
	}

	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = INADDR_ANY;
	addr.sin_port = htons(port);

	rc = bind(fd, (struct sockaddr *) &addr, sizeof(addr));
	if (rc < 0) {
		write_stderr("could not bind socket: %m\n");
	}

	rc = listen(fd, 10);
	if (rc < 0) {
		write_stderr("could not listen: %m\n");
	}

	printf("listen on port %d...\n", port);

	fds[0].fd = fd;
	fds[0].events = POLLIN;
	nfds++;

	while (1) {
		rc = poll(fds, nfds, timeout);
		if (rc < 0) {
			write_stderr("poll failed: %m\n");
		} else if (rc == 0) {
			printf("timeout expired\n");
			continue;
		}

		if (fds[0].revents & POLLIN) {
			pid_t pid;
			int clifd = accept(fd, NULL, NULL);
			if (clifd < 0) {
				printf("accept failed: %m\n");
				continue;
			}

			pid = fork();
			if (pid == -1) {
				printf("could not fork child process\n");
				continue;
			}

			if (pid == 0) {				/* child process */
				close(fd);	/* close listen fd */

				rdma_main(clifd);
			} else {
				close(clifd);	/* close child socket */
				printf("create child process (%d) to handle request\n", (int) pid);
			}
		}
	}
}

int
main(void)
{
	server_main();

	return 0;
}

static struct ibv_context *
get_rdma_param(rdma_param_t *param)
{
	int num_devices = 0;
	struct ibv_context *ctx = NULL;
	struct ibv_device **devices = NULL;

	devices = ibv_get_device_list(&num_devices);
	if (devices == NULL) {
		return -1;
	}

	for (struct ibv_device *dev = devices[0]; dev != NULL; dev++) {
		struct ibv_device_attr devattr;

		ctx = ibv_open_device(dev);
		if (ctx == NULL) {
			continue;
		}

		if (ibv_query_device(ctx, &devattr) != 0) {
			ibv_close_device(ctx);
			ctx = NULL;
			continue;
		}

		for (int port = 1; port <= devattr.phys_port_cnt; port++) {
			int gid_index = -1;
			union ibv_gid gid;
			struct ibv_port_attr port_attr;

			if (ibv_query_port(ctx, port, &port_attr) != 0) {
				continue;
			}

			for (int idx = 0; idx < port_attr.gid_tbl_len; idx++) {
				if (ibv_query_gid(ctx, port, idx, &gid) != 0) {
					continue;
				}

				gid_index = idx;
			}

			if (gid_index != -1) {
				param->gid = gid;
				param->lid = port_attr.lid;
				ibv_free_device_list(devices);
				return ctx;
			}
		}

		ibv_close_device(ctx);
	}

	ibv_free_device_list(devices);

	return NULL;
}
