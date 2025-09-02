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
void qp_to_rtr_rts(struct ibv_qp *qp, struct rdma_params server_params) {
  struct ibv_qp_attr attr = {
      .qp_state = IBV_QPS_RTR,
      .path_mtu = IBV_MTU_1024,
      .dest_qp_num = server_params.qp_num,
      .rq_psn = 0,
      .max_dest_rd_atomic = 1,
      .min_rnr_timer = 12,
      .ah_attr = {
          .is_global = 1,
          .grh = {.dgid = server_params.gid, .sgid_index = 0, .hop_limit = 1},
          .dlid = server_params.lid,
          .port_num = 1}};
  ibv_modify_qp(qp, &attr,
                IBV_QP_STATE | IBV_QP_AV | IBV_QP_PATH_MTU | IBV_QP_DEST_QPN |
                    IBV_QP_RQ_PSN);

  attr.qp_state = IBV_QPS_RTS;
  attr.sq_psn = 0;
  ibv_modify_qp(qp, &attr, IBV_QP_STATE | IBV_QP_SQ_PSN);
}

int main() {
  // TCP 客户端初始化
  int tcp_fd = socket(AF_INET, SOCK_STREAM, 0);
  struct sockaddr_in addr = {.sin_family = AF_INET, .sin_port = htons(5432)};
  inet_pton(AF_INET, "172.16.100.2", &addr.sin_addr);
  if (connect(tcp_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
    perror("TCP connect failed");
    return 1;
  }
  printf("Connected to echo server. Enter a line to send (Ctrl+D to quit):\n");

  // RDMA 初始化
  struct ibv_device **dev_list = ibv_get_device_list(NULL);
  struct ibv_context *ctx = ibv_open_device(dev_list[0]);
  struct ibv_pd *pd = ibv_alloc_pd(ctx);
  struct ibv_cq *cq = ibv_create_cq(ctx, 128, NULL, NULL, 0);

  // 接收服务器的 RDMA 参数
  struct rdma_params server_params;
  read(tcp_fd, &server_params, sizeof(server_params));

  // 生成客户端 RDMA 参数
  struct ibv_qp *qp;
  struct rdma_params client_params = generate_rdma_params(ctx, pd, cq, &qp);

  // 发送客户端参数给服务器
  write(tcp_fd, &client_params, sizeof(client_params));

  // QP 切换到 RTR 和 RTS
  qp_to_rtr_rts(qp, server_params);

  // RDMA 数据缓冲区
  char send_buffer[1024];
  char recv_buffer[1024];
  struct ibv_mr *send_mr =
      ibv_reg_mr(pd, send_buffer, sizeof(send_buffer), IBV_ACCESS_LOCAL_WRITE);
  struct ibv_mr *recv_mr =
      ibv_reg_mr(pd, recv_buffer, sizeof(recv_buffer), IBV_ACCESS_LOCAL_WRITE);

  // 主循环：读取用户输入并发送
  while (fgets(send_buffer, sizeof(send_buffer), stdin) != NULL) {
    // 移除换行符
    size_t len = strlen(send_buffer);
    if (len > 0 && send_buffer[len - 1] == '\n') {
      send_buffer[len - 1] = '\0';
      len--;
    }

    // RDMA 发送数据
    struct ibv_sge send_sge = {.addr = (uint64_t)send_buffer,
                               .length = len + 1,
                               .lkey = send_mr->lkey};
    struct ibv_send_wr send_wr = {
        .opcode = IBV_WR_SEND, .sg_list = &send_sge, .num_sge = 1};
    struct ibv_send_wr *bad_wr;
    ibv_post_send(qp, &send_wr, &bad_wr);

    struct ibv_wc wc;
    while (ibv_poll_cq(cq, 1, &wc) < 1)
      ;
    if (wc.status == IBV_WC_SUCCESS) {
      printf("Sent to server: %s\n", send_buffer);
    } else {
      fprintf(stderr, "Send failed: %d\n", wc.status);
      break;
    }

    // RDMA 接收回显数据
    struct ibv_sge recv_sge = {.addr = (uint64_t)recv_buffer,
                               .length = sizeof(recv_buffer),
                               .lkey = recv_mr->lkey};
    struct ibv_recv_wr recv_wr = {.sg_list = &recv_sge, .num_sge = 1};
    ibv_post_recv(qp, &recv_wr, &bad_wr);

    while (ibv_poll_cq(cq, 1, &wc) < 1)
      ;
    if (wc.status == IBV_WC_SUCCESS) {
      printf("Received echo from server: %s\n", recv_buffer);
    } else {
      fprintf(stderr, "Receive failed: %d\n", wc.status);
      break;
    }
  }

  // 清理
  close(tcp_fd);
  ibv_dereg_mr(send_mr);
  ibv_dereg_mr(recv_mr);
  ibv_destroy_qp(qp);
  ibv_destroy_cq(cq);
  ibv_dealloc_pd(pd);
  ibv_close_device(ctx);
  ibv_free_device_list(dev_list);
  printf("Client exiting\n");
  return 0;
}
