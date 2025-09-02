#include "common.h"

#include <stdlib.h>

#include <infiniband/verbs.h>

int
main(void)
{
	int	num_devices = 0 ;
	int port_num = 0;
	struct ibv_device **devices = NULL;
	struct ibv_context *ctx = NULL;
	struct ibv_pd *pd = NULL;
	struct ibv_cq *cq = NULL;
	struct ibv_qp *qp = NULL;

	devices = ibv_get_device_list(&num_devices);
	if (devices == NULL) {
		write_stderr("could not get available RDMA devices: %m\n");
		exit(EXIT_FAILURE);
	}

	printf("get %d available RDMA device%s\n",
		   num_devices, num_devices > 1 ? "s" : "");

	for (int i = 0; i < num_devices; i++) {
		struct ibv_device_attr attr;
		const char *name = ibv_get_device_name(devices[i]);

		printf("device index %d, device name: %s\n", i, name);

		ctx = ibv_open_device(devices[i]);
		if (ctx == NULL) {
			write_stderr("could open device \"%s\"", name);
			continue;
		}

		if (ibv_query_device(ctx, &attr) != 0) {
			write_stderr("could not query device \"%s\"", name);
			ibv_close_device(ctx);
			continue;
		}

		printf("version:    %s\n", attr.fw_ver);
		printf("port count: %d\n", attr.phys_port_cnt);
		printf("max_qp:     %d\n", attr.max_qp);
		printf("max_qp_wr:  %d\n", attr.max_qp_wr);

		/* Note that the port number starts with 1. */
		port_num = 0;
		for (int j = 1; j <= attr.phys_port_cnt; j++) {
			struct ibv_port_attr port_attr;

			if (ibv_query_port(ctx, j, &port_attr) != 0) {
				write_stderr("could not query device \"%s\" port %d\n", name, j);
				continue;
			}

			printf("	state:       %d\n", port_attr.state);
			printf("	lid:         %d\n", port_attr.lid);
			printf("    gid_tbl_len: %d\n", port_attr.gid_tbl_len);

			for (int idx = 0; idx < port_attr.gid_tbl_len; idx++) {
				union ibv_gid gid;

				if (ibv_query_gid(ctx, j, idx, &gid) == -1) {
					write_stderr("could not get gid for device \"%s\", port %d, index %d\n",
								 name, j, idx);
					continue;
				}
			}
		}

		if (ibv_close_device(ctx) == -1) {
			write_stderr("could not close device \"%s\"", name);
		}
		ctx = NULL;
	}

	ctx = ibv_open_device(devices[0]);

	ibv_free_device_list(devices);

	printf("create protection domain and completion queue\n");

	printf("num_comp_vectors: %d\n", ctx->num_comp_vectors);

	pd = ibv_alloc_pd(ctx);
	if (pd == NULL) {
		write_stderr("could not allocate protection domain\n");
		ibv_close_device(ctx);
		exit(EXIT_FAILURE);
	}

	cq = ibv_create_cq(ctx, 128, NULL, NULL, 0);
	if (cq == NULL) {
		write_stderr("could not create completion queue\n");
		ibv_dealloc_pd(pd);
		ibv_close_device(ctx);
		exit(EXIT_FAILURE);
	}

	ibv_destroy_cq(cq);
	ibv_dealloc_pd(pd);
	ibv_close_device(ctx);
	return 0;
}
