# Linux kernel stub for Onload network APIs

Implement a subset of Onload extensions to be able to run Onload
applications on a Linux kernel.

This library is a stub: it exports the symbols to be able to run
unmodified binaries, but it does not necessarily implement the
original behavior.

## Supported Features

### MSG\_ONEPKT

Linux recv, recvfrom, recvmsg, recvmmsg all support passing this flag,
for both TCP and UDP sockets.

### SO\_TIMESTAMPING: raw hardware timestamps

Support requesting `SOF_TIMESTAMPING_RAW_HARDWARE` requests even on
devices that do not implement this feature.

Intercept setsockopt `SO_TIMESTAMPING` requests to rewrite as a
request for software timestamps.

Intercept recvmsg to convert software timestamps to appear to be
hardware timestamps.

Intercept getsockopt `SO_TIMESTAMPING` requests to convert the
response to return the flags as originally passed to setsockopt.

## Background

[Onload](https://github.com/Xilinx-CNS/onload) is a high performance
hybrid userspace network stack. It presents the Linux kernel API, but
with optimized implementations of some operations.

Onload extends the Linux API with custom
[extensions](https://github.com/Xilinx-CNS/onload/blob/master/src/include/onload/extensions.h).
Applications that depends on these will not be able to run on bare
Linux. This library implements some of these extensions.

## Contributing

See [`CONTRIBUTING.md`](CONTRIBUTING.md) for details.

## License

GPLv2; see [`LICENSE`](LICENSE) for details.

