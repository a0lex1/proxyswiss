## ProxySwiss - proxy manipulation swiss knife

Allows you to:

- do port forwarding
- start proxy server (HTTPS and SOCKS5)
- chain proxy servers

```
Usage:
 proxyswiss proxy <inProxy> [proxy-chain]
OR
 proxyswiss tunnel <tunIn> <tunOut> [proxy-chain]

 inProxy     => proxy-server-type://[uname:pwd@]ip:port
 tunIn       => ip:port
 tunOut      => host:port
 proxy-chain => proxy-client-type://[uname:pwd@]host:port [, ...]

  proxy-server-type => socks5, https
  proxy-client-type => socks5

 IPv6 addresses are enclosed in square brackets:
   [2001:db8:a0b:12f0::1]
 ip:port requires IP address
 host:port allows both host name and IP address

Examples:

 proxyswiss tunnel 127.0.0.1:5500 1.2.3.4:5555

 proxyswiss tunnel 127.0.0.1:5500 example.com:80
   socks5://admin:123@4.5.6.7:1080

 proxyswiss proxy socks5://0.0.0.0:1080 socks5://proxy1.com:1080
   socks5://login:pass@proxy2.com:1080

```

## Building

Please build with Visual Studio and CMake. You'll need boost.
Linux is not available yet (TODO).
