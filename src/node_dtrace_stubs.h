// Copyright Joyent, Inc. and other Node contributors.
//
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the
// "Software"), to deal in the Software without restriction, including
// without limitation the rights to use, copy, modify, merge, publish,
// distribute, sublicense, and/or sell copies of the Software, and to permit
// persons to whom the Software is furnished to do so, subject to the
// following conditions:
//
// The above copyright notice and this permission notice shall be included
// in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
// OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN
// NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
// DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
// OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
// USE OR OTHER DEALINGS IN THE SOFTWARE.

#ifndef NODE_DTRACE_STUBS_H
#define NODE_DTRACE_STUBS_H

#ifdef HAVE_DTRACE
#include "node_dtrace.h"
#include <string.h>
#include "node_provider.h"
#elif HAVE_ETW
#include "node_dtrace.h"
#include <string.h>
#include "node_win32_etw_provider.h"
#include "node_win32_etw_provider-inl.h"
#elif HAVE_SYSTEMTAP
#include <string.h>
#include <node.h>
#include <v8.h>
#include <sys/sdt.h>
#include "node_provider.h"
#include "node_dtrace.h"
#else
#define NODE_HTTP_SERVER_REQUEST(arg0, arg1)
#define NODE_HTTP_SERVER_REQUEST_ENABLED() (0)
#define NODE_HTTP_SERVER_RESPONSE(arg0)
#define NODE_HTTP_SERVER_RESPONSE_ENABLED() (0)
#define NODE_HTTP_CLIENT_REQUEST(arg0, arg1)
#define NODE_HTTP_CLIENT_REQUEST_ENABLED() (0)
#define NODE_HTTP_CLIENT_RESPONSE(arg0)
#define NODE_HTTP_CLIENT_RESPONSE_ENABLED() (0)
#define NODE_NET_SERVER_CONNECTION(arg0)
#define NODE_NET_SERVER_CONNECTION_ENABLED() (0)
#define NODE_NET_STREAM_END(arg0)
#define NODE_NET_STREAM_END_ENABLED() (0)
#define NODE_NET_SOCKET_READ(arg0, arg1)
#define NODE_NET_SOCKET_READ_ENABLED() (0)
#define NODE_NET_SOCKET_WRITE(arg0, arg1)
#define NODE_NET_SOCKET_WRITE_ENABLED() (0)
#define NODE_GC_START(arg0, arg1)
#define NODE_GC_DONE(arg0, arg1)
#define NODE_BUFFER_ALLOC(arg0, arg1)
#define NODE_BUFFER_FREE(arg0, arg1)
#endif

#define NODE_BTYPE_SLAB 1
#define NODE_BTYPE_FAST 2
#define NODE_BTYPE_SLOW 3

#endif
