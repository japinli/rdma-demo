project('rdma-demo', 'c',
  version: '0.0.1',
)

server_srcs = files('src/server.c')
client_srcs = files('src/client.c')
rdma_server_srcs = files('src/rdma_server.c')

rdmacm_dep = dependency('librdmacm', required: false)
if not rdmacm_dep.found()
  rdmacm_dep = cc.find_library('rdmacm',
    has_headers: 'rdma/rsocket.h',
    required: true)
endif

ibverbs_dep = dependency('libibverbs', required: false)
if not ibverbs_dep.found()
  ibverbs_dep = cc.find_library('ibverb',
    has_headers: 'infiniband/verbs.h',
    required: true)
endif

deps = [rdmacm_dep]

executable('rdma-server', server_srcs,
  install: true,
  dependencies: deps)
executable('rdma-client', client_srcs,
  install: true,
  dependencies: deps)
executable('rdma-ib-server', rdma_server_srcs,
  install: true,
  dependencies: ibverbs_dep)
executable('rt-server', files('src/rt_server.c'),
  install: true,
  dependencies: ibverbs_dep)
executable('rt-client', files('src/rt_client.c'),
  install: true,
  dependencies: ibverbs_dep)
