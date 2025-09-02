# API References

## ibv_get_device_list

``` c
struct ibv_device **ibv_get_device_list(int *num_devices);
```

### 描述

`ibv_get_device_list()` 返回 `NULL` 结尾的数组，该数组中包含当前可用的 RDMA 设备。参数 `num_devices` 是可选的；当它不为 `NULL` 时，则将其设置为数组中返回的设备数量。

### 返回值

`ibv_get_device_list()` 返回可用 RDMA 设备的数组，或者如果请求失败则设置 `errno` 并返回 `NULL`。如果未找到任何设备，则将 `num_devices` 设置为 `0`，并返回非 `NULL`。

!!! note "ERRORS"

    - `EPERM` 没有权限
    - `ENOSYS` 内核不支持 RDMA 设备
    - `ENOMEM` 内存不足，无法完成该操作。

## ibv_free_device_list

``` c
void ibv_free_device_list(struct ibv_device **list);
```

### 描述

`ibv_free_device_list()` 释放由 `ibv_get_device_list()` 函数返回的设备列表数组。

### 返回值

`ibv_free_device_list()` 不返回任何值。

## ibv_open_device

``` c
struct ibv_context *ibv_open_device(struct ibv_device *device);
```

### 描述

`ibv_open_device()` 打开设备并创建上下文供进一步使用。

### 返回值

`ibv_open_device()` 返回指向分配的设备上下文指针，如果请求失败则返回 `NULL`。

## ibv_close_device

``` c
int ibv_close_device(struct ibv_context *context);
```

### 描述

`ibv_close_device()` 关闭设备上下文。

### 返回值

`ibv_close_device()` 成功返回 `0`，失败返回 `-1`。


## ibv_query_device

``` c
int ibv_query_device(struct ibv_context *context,
                     struct ibv_device_attr *device_attr);
```

### 描述

`ibv_query_device()` 返回 `context` 设备的属性。参数 `device_attr` 是指向 `ibv_device_attr` 结构的指针，定义在 `infiniband/verbs.h` 中。

``` c
struct ibv_device_attr {
        char                    fw_ver[64];             /* 固件版本 */
        uint64_t                node_guid;              /* 节点 GUID (网络字节序) */
        uint64_t                sys_image_guid;         /* 系统镜像 GUID (网络字节序) */
        uint64_t                max_mr_size;            /* 可注册的最大连续区块 */
        uint64_t                page_size_cap;          /* 支持的内存移位大小 */
        uint32_t                vendor_id;              /* 供应商 ID，符合 IEEE */
        uint32_t                vendor_part_id;         /* 供应商提供的零件 ID */
        uint32_t                hw_ver;                 /* 硬件版本 */
        int                     max_qp;                 /* 支持的最大 QP 数量 */
        int                     max_qp_wr;              /* 任何工作队列中未完成工作记录的最大数量 */
        unsigned int            device_cap_flags;       /* HCA 功能掩码 */
        int                     max_sge;                /* 对于非 RDMA 中读操作 QP 中 SQ 和 RQ 的每个 WR 最大 s/g 数量 */
        int                     max_sge_rd;             /* RDMA 读取作的每个 WR 的最大 s/g 数 */
        int                     max_cq;                 /* 支持的最大 CQ 数量 */
        int                     max_cqe;                /* 每个 CQ 的最大 CQE 容量数量 */
        int                     max_mr;                 /* 支持的最大 MR 数量 */
        int                     max_pd;                 /* 支持的最大 PD 数量 */
        int                     max_qp_rd_atom;         /* 每个 QP 可以完成的 RDMA 读取和原子作的最大数量 */
        int                     max_ee_rd_atom;         /* 每个 EEC 可以完成的 RDMA 读取和原子作的最大数量 */
        int                     max_res_rd_atom;        /* 此 HCA 作为目标用于 RDMA 读取和原子作的最大资源数量 */
        int                     max_qp_init_rd_atom;    /* 启动 RDMA 读取和原子作的每个 QP 的最大深度 */
        int                     max_ee_init_rd_atom;    /* 每个 EEC 的最大深度用于启动 RDMA 读取和原子操作 */
        enum ibv_atomic_cap     atomic_cap;             /* 原子操作支持级别 */
        int                     max_ee;                 /* 支持的 EE 上下文的最大数量 */
        int                     max_rdd;                /* 支持的 RD 域的最大数量 */
        int                     max_mw;                 /* 支持的最大内存窗口数量 */
        int                     max_raw_ipv6_qp;        /* 支持的原始 IPv6 数据报 QP 的最大数量 */
        int                     max_raw_ethy_qp;        /* 支持的以太网数据报 QP 的最大数量 */
        int                     max_mcast_grp;          /* 支持的最大多播组数量 */
        int                     max_mcast_qp_attach;    /* 每个组播可附加的最大 QP 数量 */
        int                     max_total_mcast_qp_attach;/* 可附加到组播组的最大 QP 数量 */
        int                     max_ah;                 /* 支持的最大地址句柄数量 */
        int                     max_fmr;                /* 支持的最大 FMR 数量 */
        int                     max_map_per_fmr;        /* 取消映射操作之前，每个 FMR 的最大（重新）映射次数要求 */
        int                     max_srq;                /* 支持的最大 SRQ 数量 */
        int                     max_srq_wr;             /* 每个 SRQ 的最大 WR 数量 */
        int                     max_srq_sge;            /* 每个 SRQ 的最大 s/g 数量 */
        uint16_t                max_pkeys;              /* 最大分区数 */
        uint8_t                 local_ca_ack_delay;     /* 本地 CA 确认延迟 */
        uint8_t                 phys_port_cnt;          /* 物理端口数 */
};
```

其中 `enum ibv_atomic_cap` 可能的值为：

- `IBV_ATOMIC_NONE`: 不支持原子操作。
- `IBV_ATOMIC_HCA`: 支持 HCA 级别的原子操作。
- `IBV_ATOMIC_GLOB`: 支持全局原子操作。

### 返回值

`ibv_query_device()` 成功返回 `0`，否则返回 `errno`（代表失败的原因）。

## ibv_query_port

``` c
int ibv_query_port(struct ibv_context *context, uint8_t port_num,
                   struct ibv_port_attr *port_attr);
```

### 描述

`ibv_query_port()` 通过 `port_attr` 指针返回设备上下文 `port_num` 端口的属性。参数 `port_attr` 是一个 `ibv_port_attr` 结构，定义在 `<infiniband/verbs.h>` 中。

``` c
struct ibv_port_attr {
        enum ibv_port_state     state;          /* 逻辑端口状态 */
        enum ibv_mtu            max_mtu;        /* 该端口支持的最大 MTU */
        enum ibv_mtu            active_mtu;     /* 实际的 MTU */
        int                     gid_tbl_len;    /* 源 GID 表长度 */
        uint32_t                port_cap_flags; /* 端口能力 */
        uint32_t                max_msg_sz;     /* 最大消息大小 */
        uint32_t                bad_pkey_cntr;  /* 不良 P_Key 计数器 */
        uint32_t                qkey_viol_cntr; /* Q_Key 违规计数器 */
        uint16_t                pkey_tbl_len;   /* 分区表长度 */
        uint16_t                lid;            /* 基础端口 LID */
        uint16_t                sm_lid;         /* 子网管理器（SM）的 LID */
        uint8_t                 lmc;            /* LID 掩码控制 */
        uint8_t                 max_vl_num;     /* 最大虚拟通道数 */
        uint8_t                 sm_sl;          /* 子网管理器服务级别 */
        uint8_t                 subnet_timeout; /* 子网传播延迟 */
        uint8_t                 init_type_reply;/* SM 执行的初始化类型 */
        uint8_t                 active_width;   /* 当前活跃链路宽度 */
        uint8_t                 active_speed;   /* 如果 speed<XDR 则为当前活跃链路的速度，否则为 NDR */
        uint8_t                 phys_state;     /* 物理端口状态 */
        uint8_t                 link_layer;     /* 端口的链路层协议 */
        uint8_t                 flags;          /* 端口能力标志 */
        uint16_t                port_cap_flags2;/* 扩展端口能力标志 */
        uint32_t                active_speed_ex;/* 当前活跃链路的速度，如果为 0，则以 active_speed 替代 */
};
```

`enum ibv_port_state` 可能的值为：

- `IBV_PORT_NOP (0)`: 未定义。
- `IBV_PORT_DOWN (1)`: 关闭。
- `IBV_PORT_INIT (2)`: 初始化。
- `IBV_PORT_ARMED (3)`: 准备。
- `IBV_PORT_ACTIVE (4)`: 激活。
- `IBV_PORT_NOP (5)`: 延迟激活。

`enum ibv_mtu` 可能的值为：

- `IBV_MTU_256 (1)`: 256 字节。
- `IBV_MTU_512 (2)`: 512 字节。
- `IBV_MTU_1024 (3)`: 1024 字节。
- `IBV_MTU_2048 (4)`: 2048 字节。
- `IBV_MTU_4096 (5)`: 4096 字节。

`link_layer` 可能的值为：

- `IBV_LINK_LAYER_UNSPECIFIED`: 未指定。
- `IBV_LINK_LAYER_INFINIBAND`: InfiniBand。
- `IBV_LINK_LAYER_ETHERNET`: RoCE。

## ibv_query_gid

``` c
int ibv_query_gid(struct ibv_context *context, uint8_t port_num,
                  int index, union ibv_gid *gid);
```

### 描述

`ibv_query_gid()` 用于查询指定 InfiniBand 设备（由 `context` 表示）的某个端口（由 `port_num` 指定）的 GID 表中的某个条目（由 `index` 指定），并将结果存储在 `gid` 中。

- GID（Global Identifier）是 InfiniBand 网络中用于标识端口的全局唯一标识符。
- 每个 InfiniBand 端口可以有多个 GID，通常存储在一个 GID 表中。
- GID 通常与 IPv6 地址相关联，用于 InfiniBand 的全局路由（Global Routing）。

可以通过 `ibv_query_port()` 函数返回的端口属性中的 `port_attr.gid_tbl_len` 确定 GID 表的条目数量，有效范围为 `[0, port_attr.gid_tbl_len - 1]`。

### 返回值

`ibv_query_gid()` 成功时返回 `0`，错误时返回 `-1`。

## ibv_alloc_pd

``` c
struct ibv_pd *ibv_alloc_pd(struct ibv_context *context);
```

### 描述

`ibv_alloc_pd()` 为 `context` 的 RDMA 设备创建一个保护域（PD），管理 RDMA 资源的访问权限。

保护域用于保证安全并提供隔离性，防止非法的内存访问，创建 PD 后用于绑定 QP 和 MR。

### 返回值

`ibv_alloc_pd()` 成功时返回指向分配的保护域（PD）指针，失败时返回 `NULL`。

## ibv_dealloc_pd

``` c
int ibv_dealloc_pd(struct ibv_pd *pd);
```

### 描述

`ibv_dealloc_pd()` 用于释放保护域（PD）资源。

### 返回值

`ibv_dealloc_pd()` 成功返回 `0`，否则返回 `errno`（代表失败的原因）。

!!! note "注意"

    当还有其他资源关联到正在释放的 PD 时可能出错。

## ibv_create_cq

``` c
struct ibv_cq *ibv_create_cq(struct ibv_context *context, int cqe,
                             void *cq_context,
                             struct ibv_comp_channel *channel,
                             int comp_vector);
```

### 描述

`ibv_create_cq()` 为 RDMA 设备上下文 `context` 创建一个至少包含 `cqe` 个条目的完成队列 (CQ)。指针 `cq_context` 将用于设置 CQ 结构的用户上下文指针。参数 `channel` 是可选的；如果不为 `NULL`，则将使用完成通道 `channel` 返回完成事件。CQ 将使用完成向量 `comp_vector` 来发出完成事件信号；它必须至少为零且小于 `context->num_comp_vectors`。

### 返回值

`ibv_create_cq()` 成功时返回指向 CQ 的指针，失败时返回 `NULL`。

## ibv_destroy_cq

``` c
int ibv_destroy_cq(struct ibv_cq *cq);
```

### 描述

`ibv_destroy_cq()` 用于销毁 CQ 对象。

### 返回值

`ibv_destroy_cq()` 成功返回 0，否则返回 errno（代表失败的原因）。

!!! note "注意"

    当还有 QP 与该 CQ 关联时，`ibv_destroy_cq()` 将会失败。


