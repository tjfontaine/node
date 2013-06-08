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

#ifndef NODE_WRAP_H
#define NODE_WRAP_H

#include "pipe_wrap.h"
#include "tty_wrap.h"
#include "tcp_wrap.h"
#include "udp_wrap.h"

#include "v8.h"
#include "uv.h"

namespace node {

inline uv_stream_t* HandleToStream(v8::Local<v8::Object> obj) {
  uv_stream_t* stream = NULL;
  Local<Value> wrapType = obj->Get(String::NewSymbol("wrapType"));
  if (wrapType->Equals(String::NewSymbol("pipe"))) {
    stream = reinterpret_cast<uv_stream_t*>(PipeWrap::Unwrap(obj
        ->Get(String::NewSymbol("handle")).As<v8::Object>())->UVHandle());
  } else if (wrapType->Equals(String::NewSymbol("tty"))) {
    stream = reinterpret_cast<uv_stream_t*>(TTYWrap::Unwrap(obj
        ->Get(String::NewSymbol("handle")).As<v8::Object>())->UVHandle());
  } else if (wrapType->Equals(String::NewSymbol("tcp"))) {
    stream = reinterpret_cast<uv_stream_t*>(TCPWrap::Unwrap(obj
        ->Get(String::NewSymbol("handle")).As<v8::Object>())->UVHandle());
  } else if (wrapType->Equals(String::NewSymbol("udp"))) {
    stream = reinterpret_cast<uv_stream_t*>(UDPWrap::Unwrap(obj
        ->Get(String::NewSymbol("handle")).As<v8::Object>())->UVHandle());
  }
  return stream;
}

}

#endif
