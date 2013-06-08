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

#ifndef TLS_WRAP_H_
#define TLS_WRAP_H_
#include "stream_wrap.h"
#include "node_crypto.h"

namespace node {

class TLSWrap : public StreamWrap {
 public:
  static v8::Local<v8::Object> Instantiate();
  static TLSWrap* Unwrap(v8::Local<v8::Object> obj);
  static void Initialize(v8::Handle<v8::Object> target);

  virtual uv_buf_t Alloc(size_t suggested_size);
  virtual v8::Local<v8::Object> Shrink(uv_buf_t buf, size_t nread);

  virtual void AfterRead(uv_buf_t buf, size_t nread, uv_handle_type pending);
  virtual size_t BeforeWrite(char* data, size_t len, uv_buf_t* buf);

  uv_stream_t* UVHandle();

 private:
  TLSWrap(v8::Handle<v8::Object> object, uv_stream_t* h, SSL* ctx);
  ~TLSWrap();

  static v8::Handle<v8::Value> New(const v8::Arguments& args);

  SSL* ssl_;
  BIO* bio_read_;
  BIO* bio_write_;
};


}  // namespace node


#endif  // TCP_WRAP_H_
