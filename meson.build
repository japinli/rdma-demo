project('rdma-demo', 'c',
  version: '0.0.1',
)

server_srcs = files('src/server.c')
client_srcs = files('src/client.c')

rdmacm_dep = dependency('librdmacm', required: false)
if not rdmacm_dep.found()
  rdmacm_dep = cc.find_library('rdmacm',
    has_headers: 'rdma/rsocket.h',
    required: true)
endif

deps = [rdmacm_dep]

executable('rdma-server', server_srcs,
  install: true,
  dependencies: deps)
executable('rdma-client', client_srcs,
  install: true,
  dependencies: deps)
