# Tracing

  Stability: 2 - Unstable

This is a description of the tracing points exported for use with tools like
[DTrace](http://dtrace.org),
[ETW](http://msdn.microsoft.com/en-us/magazine/cc163437.aspx), and
[SystemTap](sourceware.org/systemtap/). If your version of node was compiled
with these enabled you don't need to take any further action in your code to
use them.

## Brief examples

This is not meant to be documentation on how to use a specific tracing
platform. For information on how to use these tools please refer to their
respective documentation.

### DTrace

The following is a simple example that will work on Illumos, OS X, and FreeBSD.
It will print the remote IP address, http method, and url for a given request
as it comes in.

```
# dtrace -qn 'node*::http-server-request {
  printf("%s -> %s %s\n", copyinstr(arg5), copyinstr(arg6), copyinstr(arg3));
}'
GET -> /allison?limit=5&offset=5 127.0.0.1
GET -> /uter?limit=5&offset=5 127.0.0.1
GET -> /wendell 127.0.0.1
```

### SystemTap

The same example from DTrace, but using SystemTap

```
# stap -e 'probe process("node").mark("http__server__request") {
  printf("%s -> %s %s\n", user_string($arg5), user_string($arg6), user_string($arg3));
}'
GET -> /uter?limit=5&offset=5 127.0.0.1
GET -> /allison?limit=5 127.0.0.1
GET -> /uter?limit=5 127.0.0.1
```

Also we distribute a `node.stp` file in `${PREFIX}/share/systemtap/tapset/`,
which maps `$arg0..N` to names, and defines a `probestr` which is a
representation of each trace point.

```
# stap -e 'probe node_http_server_request { println(probestr); }'
http__server__request(remote=192.168.198.1, port=64913, method=GET, url=/, fd=11)
http__server__request(remote=192.168.198.1, port=64913, method=GET, url=/favicon.ico, fd=11)
```

### ETW

Here is a quick example that records all trace points and translates the output
to CSV.

```
:: Start collecting events into output.etl
logman start node -ow -p NodeJS-ETW-provider -ets -o output.etl
:: Do stuff with node. All node processes are traced.
node myscript.js
:: Stop collecting events.
logman stop node -ets
:: Convert the log from binary format to csv. You can also do this while the
:: collector is running.
tracerpt output.etl -o output.csv -of csv
```

There are lots of options for logman and tracerpt to configure what you collect
and report and how it is formatted.

## Translators

Some trace points (probes) have structs pased as arguments, on supported
platforms (like [Illumos](http://illumos.org)) software is able to
automatically translate the dereference of the members. However on other
platforms (OS X, FreeBSD, Linux/SystemTap) the software is unable automatically
translate them for you. To handle that case many trace points include the
members of the structs as additional arguments.

### node_dtrace_connection_t

 * `int fd`
  - File descriptor of the underlying socket (-1 if unset or Win32)
 * `int port`
  - Remote port of connected client
 * `char *remote`
  - IP address of connected client
 * `int buffered`
  - The number of bytes waiting to be sent from javascript to libuv

### node_dtrace_http_client_request_t

 * `char *url`
  - The URL for the outgoing request
 * `char *method`
  - The METHOD used for the outgoing request

### node_dtrace_http_server_request_t

 * `char *url`
  - The URL of the incoming request
 * `char *method`
  - The METHOD of the incoming request

## Trace points

### gc-start(type, flags)

Fired by [GCPrologueCallback](http://izs.me/v8-docs/namespacev8.html#ab1b8c05dd6d75d0fbed8902b63580ed1)

 * [GCType](http://izs.me/v8-docs/namespacev8.html#ac109d6f27e0c0f9ef4e98bcf7a806cf2) type
 * [GCCallbackFlags](http://izs.me/v8-docs/namespacev8.html#a247c37a849f4d6c293b9b16e94e1944b) flags

### gc-done(type, flags)

Fired by [GCEpilogueCallback](http://izs.me/v8-docs/namespacev8.html#a220aa1e29ecc75f460d8dbd765b66435)

 * [GCType](http://izs.me/v8-docs/namespacev8.html#ac109d6f27e0c0f9ef4e98bcf7a806cf2) type
 * [GCCallbackFlags](http://izs.me/v8-docs/namespacev8.html#a247c37a849f4d6c293b9b16e94e1944b) flags

### socket-read(conn, nbytes, remote, port, fd)

 * [node_dtrace_connection_t conn](#tracing_node_dtrace_connection_t)
 * `int nbytes`
  - number of bytes read
 * `char *remote`
  - IP address of remote client
 * `int port`
  - Port number of connected client
 * `int fd`
  - File descriptor of the underlying socket (-1 if unset or Win32)

### socket-write(conn, nbytes, remote, port, fd)

 * [node_dtrace_connection_t conn](#tracing_node_dtrace_connection_t)
 * `int nbytes`
  - number of bytes read
 * `char *remote`
  - IP address of remote client
 * `int port`
  - Port number of connected client
 * `int fd`
  - File descriptor of the underlying socket (-1 if unset or Win32)

### net-server-connection(conn, remote, port, fd)

Fired before ['connection'](net.html#event_connection_) is emitted

 * [node_dtrace_connection_t conn](#tracing_node_dtrace_connection_t)
 * `char *remote`
  - IP address of remote client
 * `int port`
  - Port number of connected client
 * `int fd`
  - File descriptor of the underlying socket (-1 if unset or Win32)

### http-server-request(req, conn, remote, port, method, url, fd)

Fired before ['request'](http.html#event_request_) is emitted

 * [node_dtrace_http server_request_t req](#tracing_node_dtrace_http_server_request_t)
 * [node_dtrace_connection_t conn](#tracing_node_dtrace_connection_t)
 * `char *remote`
  - IP address of remote client
 * `int port`
  - Port number of connected client
 * `char *method`
  - METHOD of incoming request
 * `char *url`
  - URL of incoming request
 * `int fd`
  - File descriptor of the underlying socket (-1 if unset or Win32)

### http-server-response(conn, remote, port, fd)

Fired before ['finish'](stream.html#event_finish_) is emitted

 * [node_dtrace_connection_t conn](#tracing_node_dtrace_connection_t)
 * `char *remote`
  - IP address of remote client
 * `int port`
  - Port number of connected client
 * `int fd`
  - File descriptor of the underlying socket (-1 if unset or Win32)

### http-client-request(req, conn, remote, port, method, url, fd)

Fired before ['finish'](stream.html#event_finish_) is emitted

 * [node_dtrace_http server_request_t req](#tracing_node_dtrace_http_server_request_t)
 * [node_dtrace_connection_t conn](#tracing_node_dtrace_connection_t)
 * `char *remote`
  - IP address of remote client
 * `int port`
  - Port number of connected client
 * `char *method`
  - METHOD of incoming request
 * `char *url`
  - URL of incoming request
 * `int fd`
  - File descriptor of the underlying socket (-1 if unset or Win32)

### http-client-response(conn, remote, port, fd)

Fired before ['response'](http.html#event_response_) is emitted

 * [node_dtrace_connection_t conn](#tracing_node_dtrace_connection_t)
 * `char *remote`
  - IP address of remote client
 * `int port`
  - Port number of connected client
 * `int fd`
  - File descriptor of the underlying socket (-1 if unset or Win32)
