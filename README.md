# RDMA Demo

This project is a demo about Remote Direct Memory Access (RDMA) using rsocket API.

## Requires

- librdmacm

## Build

``` bash
git clone https://github.com/japinli/rdma-demo.git
cd rdma-demo
meson build
cd build
meson compile
```

## Notes

The `rpoll()` is not fully compatible with `poll()`.  For example, the `rpoll()`
doesn't respect the `timeout` parameter.
