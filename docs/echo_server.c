#include <arpa/inet.h>
#include <infiniband/verbs.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

// RDMA 参数结构体
struct rdma_params {
  uint32_t qp_num;
  uint16_t lid;
  union ibv_gid gid;
  char unique_id[32];
};

// 初始化 RDMA 资源并生成参数
struct rdma_params generate_rdma_params(struct ibv_context *ctx,
                                        struct ibv_pd *pd, struct ibv_cq *cq,
                                        struct ibv_qp **qp) {
  struct ibv_qp_init_attr qp_attr = {
      .qp_type = IBV_QPT_RC,
      .send_cq = cq,
      .recv_cq = cq,
      .cap = {.max_send_wr = 16, .max_recv_wr = 16}};
  *qp = ibv_create_qp(pd, &qp_attr);

  struct ibv_port_attr port_attr;
  ibv_query_port(ctx, 1, &port_attr);
  if (port_attr.state != IBV_PORT_ACTIVE) {
    fprintf(stderr, "Port not active\n");
    exit(1);
  }

  union ibv_gid gid;
  ibv_query_gid(ctx, 1, 0, &gid);

  struct rdma_params params = {
      .qp_num = (*qp)->qp_num, .lid = port_attr.lid, .gid = gid};
  sprintf(params.unique_id, "%d-%ld", getpid(), time(NULL));

  struct ibv_qp_attr attr = {.qp_state = IBV_QPS_INIT,
                             .pkey_index = 0,
                             .port_num = 1,
                             .qp_access_flags = IBV_ACCESS_LOCAL_WRITE |
                                                IBV_ACCESS_REMOTE_WRITE};
  ibv_modify_qp(*qp, &attr,
                IBV_QP_STATE | IBV_QP_PKEY_INDEX | IBV_QP_PORT |
                    IBV_QP_ACCESS_FLAGS);

  return params;
}

// QP 切换到 RTR 和 RTS
void qp_to_rtr_rts(struct ibv_qp *qp, struct rdma_params client_params) {
  struct ibv_qp_attr attr = {
      .qp_state = IBV_QPS_RTR,
      .path_mtu = IBV_MTU_1024,
      .dest_qp_num = client_params.qp_num,
      .rq_psn = 0,
      .max_dest_rd_atomic = 1,
      .min_rnr_timer = 12,
      .ah_attr = {
          .is_global = 1,
          .grh = {.dgid = client_params.gid, .sgid_index = 0, .hop_limit = 1},
          .dlid = client_params.lid,
          .port_num = 1}};
  ibv_modify_qp(qp, &attr,
                IBV_QP_STATE | IBV_QP_AV | IBV_QP_PATH_MTU | IBV_QP_DEST_QPN |
                    IBV_QP_RQ_PSN);

  attr.qp_state = IBV_QPS_RTS;
  attr.sq_psn = 0;
  ibv_modify_qp(qp, &attr, IBV_QP_STATE | IBV_QP_SQ_PSN);
}

int main() {
  // TCP 服务器初始化
  int tcp_fd = socket(AF_INET, SOCK_STREAM, 0);
  struct sockaddr_in addr = {.sin_family = AF_INET,
                             .sin_addr.s_addr = INADDR_ANY,
                             .sin_port = htons(5432)};
  bind(tcp_fd, (struct sockaddr *)&addr, sizeof(addr));
  listen(tcp_fd, 5);
  printf("Echo server listening on port 5432...\n");

  // RDMA 初始化
  struct ibv_device **dev_list = ibv_get_device_list(NULL);
  struct ibv_context *ctx = ibv_open_device(dev_list[0]);
  struct ibv_pd *pd = ibv_alloc_pd(ctx);
  struct ibv_cq *cq = ibv_create_cq(ctx, 128, NULL, NULL, 0);

  while (1) {
    int client_fd = accept(tcp_fd, NULL, NULL);
    pid_t pid = fork();
    if (pid == 0) { // 子进程
      close(tcp_fd);

      // 生成 RDMA 参数
      struct ibv_qp *qp;
      struct rdma_params server_params = generate_rdma_params(ctx, pd, cq, &qp);

      // 通过 TCP 发送参数
      write(client_fd, &server_params, sizeof(server_params));

      // 接收客户端的 RDMA 参数
      struct rdma_params client_params;
      read(client_fd, &client_params, sizeof(client_params));

      // QP 切换到 RTR 和 RTS
      qp_to_rtr_rts(qp, client_params);

      // RDMA 接收客户端数据
      char buffer[1024];
      struct ibv_mr *mr =
          ibv_reg_mr(pd, buffer, sizeof(buffer), IBV_ACCESS_LOCAL_WRITE);
      struct ibv_sge sge = {
          .addr = (uint64_t)buffer, .length = sizeof(buffer), .lkey = mr->lkey};
      struct ibv_recv_wr wr = {.sg_list = &sge, .num_sge = 1};
      struct ibv_recv_wr *bad_wr;
      ibv_post_recv(qp, &wr, &bad_wr);

      struct ibv_wc wc;
      while (ibv_poll_cq(cq, 1, &wc) < 1)
        ;
      if (wc.status == IBV_WC_SUCCESS) {
        printf("Received from client: %s\n", buffer);

        // RDMA 发送回客户端（Echo）
        struct ibv_sge send_sge = {
            .addr = (uint64_t)buffer, .length = wc.byte_len, .lkey = mr->lkey};
        struct ibv_send_wr send_wr = {
            .opcode = IBV_WR_SEND, .sg_list = &send_sge, .num_sge = 1};
        ibv_post_send(qp, &send_wr, &bad_wr);

        while (ibv_poll_cq(cq, 1, &wc) < 1)
          ;
        if (wc.status == IBV_WC_SUCCESS) {
          printf("Echo sent back to client\n");
        }
      }

      // 关闭 TCP 和清理
      close(client_fd);
      ibv_dereg_mr(mr);
      ibv_destroy_qp(qp);
      ibv_destroy_cq(cq);
      ibv_dealloc_pd(pd);
      ibv_close_device(ctx);
      ibv_free_device_list(dev_list);
      exit(0);
    }
    close(client_fd);
  }
  return 0;
}
