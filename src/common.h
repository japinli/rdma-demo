#ifndef COMMON_H
#define COMMON_H

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#define rs_accept(s, a, l)  use_rdma ? raccept(s, a, l)  : accept(s, a, l)
#define rs_bind(s, a, l)    use_rdma ? rbind(s, a, l)    : bind(s, a, l)
#define rs_close(s)         use_rdma ? rclose(s)         : close(s)
#define rs_connect(s, a, l) use_rdma ? rconnect(s, a, l) : connect(s, a, l)
#define rs_listen(s, b)     use_rdma ? rlisten(s, b)     : listen(s, b)
#define rs_recv(s, b, l, f) use_rdma ? rrecv(s, b, l, f) : recv(s, b, l, f)
#define rs_send(s, b, l, f) use_rdma ? rsend(s, b, l, f) : send(s, b, l, f)
#define rs_socket(f, t, p)  use_rdma ? rsocket(f, t, p)  : socket(f, t, p)

#define rs_poll(f, n, t)    use_rdma ? rpoll(f, n, t)    : poll(f, n, t)
#define rs_fcntl(s, c, p)   use_rdma ? rfcntl(s, c, p)   : fcntl(s, c, p)

#define rs_getsockopt(s, l, n, v, o) \
	use_rdma ? rgetsockopt(s, l, n, v, o) : getsockopt(s, l, n, v, o)
#define rs_setsockopt(s, l, n, v, o) \
	use_rdma ? rsetsockopt(s, l, n, v, o) : setsockopt(s, l, n, v, o)

static const char *
get_program(const char *argv0)
{
	const char *slash = strrchr(argv0, '/');
	return slash ? slash + 1 : argv0;
}

static void
write_stderr(const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	va_end(ap);
}

static void
do_advice(const char *progname)
{
	write_stderr("Try \"%s --help\" for more information.\n", progname);
}

#endif
