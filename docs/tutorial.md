# RDMA

## RDMA 是什么？

RDMA 全称是 **R**emote **D**irect **M**emory **A**ccess（远程直接内存访问）。它是一种网络通信技术，允许计算机在不涉及操作系统或处理器的情况下，直接从一台计算机的内存访问另一台计算机的内存数据。这种技术通过减少数据传输过程中的 CPU 干预，大幅降低了延迟并提高了带宽利用率。

RDMA 提供从一个主机（存储或计算）内存到另一个主机内存的直接内存访问，无需操作系统和 CPU 介入，从而以更低的延迟、更低的 CPU 负载和更高的带宽提高网络和主机性能。相比之下，TCP/IP 通信通常需要复制，这会增加延迟并小号大量 CPU 和内存资源。

## 术语

在进入 RDMA 世界之前，我们需要先了解其基本术语。

- Access Layer 接入层：用于访问互连结构（VPI™、InfiniBand®、以太网、FCoE）的低级操作系统基础设施。它包括支持上层网络协议、中间件和管理代理所需的所有基本传输服务。
- AH, Address Handler 地址句柄：描述 UD QP 中使用远程端路径的对象。
- CA, Channel Adapter 通道适配器：终止 InfiniBand 链接并执行传输级函数的设备。
- CI, Channel Interface 通道接口：通过网络适配器、相关固件和设备驱动程序的组合实现的通道呈现给 Verbs Consumer。
- CM, Communication Manager 通信管理者：
  * 负责建立、维护和发布 RC 和 UC QP 服务类型的通信的实体。
  * 服务 ID 解析协议可以帮助 UD 服务的用户找到支持其所需服务的 QP。
  * 终端节点的每个 IB 端口中都有一个 CM。
- Compare & Swap 比较和交换：指示远程 QP 读取 64 位值，将其与提供的比较数据进行比较，如果相等，则将其替换位 QP 中提供的交换数据。
- CQ, Completion Queue 完成队列：包含 CQE 的队列（FIFO）。
- CQE, Completion Queue Entry 完成队列条目：CQ 中的一个条目，用于描述有关已完成 WR 的信息（状态、大小等）。
- DMA, Direct Memory Access 直接内存访问：允许硬件绕过 CPU 将数据块直接移入和移出内存。
- Fetch & Add 获取和增加：指示远程 QP 读取 64 位值，并将其替换为 QP 中提供的 64 位值和添加的数据值之和。
- GUID, Globally Unique IDentifier 全局唯一标识符：一个 64 位数字，用于唯一标识子网中的设备或组件。
- GID, Global IDentifier：
  * 一个 128 位标识符，用于标识网络适配器上的端口、路由器上的端口或多播组。
  * GID 是一个有效的 128 位 IPv6 地址（RFC 2373），在 IBA 中定义了额外的属性/限制，以促进高效的发现通信和路由。
- GRH, Global Routing Header：用于跨子网边界传送数据包的数据包标头，也用于传送多播消息。此数据包标头基于 IPv6 协议。
- Network Adapter 网络适配器：一种允许在网络中的计算机之间进行通信的硬件设备。
- Host 主机：运行操作系统的计算机平台，可以控制一个或多个网络适配器。
- IB：InfiniBand。
- Join Operation：IB 端口必须通过向 SA 发送请求来显式加入组播组，以接收组播数据包。
- lkey：注册 MR 时收到的编号，由 WR 在本地使用，以标识内存区域及相关权限。
- LID, Local IDentifier 本地标识符：子网管理器分配给终端节点的 16 位地址。每个 LID 在其子网中都是唯一的。
- LLE, Low Latency Ethernet 低延迟以太网：基于 CEE（融合增强型以太网）的 RDMA 服务允许通过以太网进行 IB 传输。
- MGID, Multicast Group ID 组播组 ID：
  * 由 MDID 标识的 IB 组播组，由 SM 管理。
  * SM 将 MLID 与每个 MGID 相关联，并在交换矩阵中显式编程 IB 交换机，以确保加入组播组的所有端口都能接收数据包。
- MR, Memory Region 内存区域：一组连续的内存缓冲区，这些缓冲区已经注册了访问权限。需要为网络适配器注册这些缓冲区才能使用它们。在注册过程中，将创建一个 `lkey` 和 `rkey` 并将其与创建的内存区域相关联。
- MTU, Maximum Transfer Unit：可以从端口发送/接收的数据包负载（不包含标头）的最大大小。
- MW, Memory Window：
  * 一种已分配的资源，在绑定到现有的 Memory Registration 中的指定区域后启用远程访问。
  * 每个内存窗口都有一个关联的窗口句柄、一组访问权限和当前 `rkey`。
- Outstanding Work Request 未完成的工作请求：已发布到工作队列且未轮询其完成情况的 WR。
- pkey, Partition key 分区键：
  * `pkey` 标识端口所属分区。
  * `pkey` 大致类似于以太网中的 VLAN ID。
  * 用于指向端口的分区键标中的条目。
  * 子网管理器（SM）为每个端口分配至少一个 `pkey`。
- PD, Protection Domain 保护域：组件只能相互交换的对象。AH 与 QP 交互；MR 与 WQ 交互。
- QP, Queue Pair 队列对：
  * 一对独立的 WQ（发送队列和接收队列）打包在一起放在一个对象中，用于在网络节点之间传输数据。
  * Posts 用于启动数据的发送和接收。
  * QP 分为三种类型：
    * UD, Unreliable Datagram 不可靠数据报：一种 QP 传输服务类型，其中消息可以是一个数据包长度，并且每个 UD QP 都可以发送/接收来自子网中另一个 UD QP 的消息，消息可能会丢失，并且无法保证顺序。UD QP 是唯一支持组播消息的类型。UD 数据包的消息大小限制为路径 MTU。
    * UC, Unreliable Connection 不可靠连接：基于面向连接的协议的 QP 传输服务类型，其中 QP 与另一个 QP 相关联。QP 不执行可靠的协议，消息可能会丢失。
    * RC, Reliable Connection 可靠连接：基于面向连接协议的 QP 传输服务类型。QP 与另一个 QP 相关联。消息以可靠的方式发送（就信息的正确性和顺序而言）。
- RDMA, Remote Direct Memory Access 远程直接内存访问：在没有远程 CPU 参与的情况下访问远端的内存。
- RDMA_CM, Remote Direct Memory Access Communication Manager：
  * 用于设置可靠、连接和不可靠的数据报数据传输 API。
  * 提供了一个 RDMA 传输中立接口来建立连接。
  * 该 API 基于套接字，但适用于基于 QP 的语义：通信必须通过特定的 RDMA 设备进行，并且数据传输是基于消息的。


## RDMA 特性

- **Remote**: 远程服务器之间的数据交换
- **Direct**: 数据交换过程中无需 CPU 或操作系统内核干预
- **Memory**: 数据在应用程序的虚拟内存之间传输
- **Access**: 支持 send/receive，read/write 以及原子操作

RDMA 可以做到零拷贝、绕过 CPU 和操作系统内核，并且消息是基于事务的。

## 参考

[1] https://www.doc.ic.ac.uk/~jgiceva/teaching/ssc18-rdma.pdf
