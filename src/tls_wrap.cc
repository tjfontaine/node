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

#include "node.h"
#include "node_buffer.h"
#include "req_wrap.h"
#include "handle_wrap.h"
#include "stream_wrap.h"
#include "tls_wrap.h"
#include "node_wrap.h"

#include "node_crypto.h"
#include "node_crypto_bio.h"

#include <stdlib.h>


namespace node {

using v8::Arguments;
using v8::Context;
using v8::Function;
using v8::FunctionTemplate;
using v8::Handle;
using v8::HandleScope;
using v8::Integer;
using v8::Local;
using v8::Null;
using v8::Object;
using v8::Persistent;
using v8::PropertyAttribute;
using v8::String;
using v8::TryCatch;
using v8::Undefined;
using v8::Value;

using node::crypto::SecureContext;

static Persistent<Function> tlsConstructor;

Local<Object> AddressToJS(const sockaddr* addr);


Local<Object> TLSWrap::Instantiate() {
  // If this assert fire then process.binding('tls_wrap') hasn't been
  // called yet.
  assert(tlsConstructor.IsEmpty() == false);

  HandleScope scope(node_isolate);
  Local<Object> obj = tlsConstructor->NewInstance();

  return scope.Close(obj);
}


void TLSWrap::Initialize(Handle<Object> target) {
  HandleWrap::Initialize(target);
  StreamWrap::Initialize(target);

  HandleScope scope(node_isolate);

  Local<FunctionTemplate> t = FunctionTemplate::New(New);
  t->SetClassName(String::NewSymbol("TLS"));

  t->InstanceTemplate()->SetInternalFieldCount(1);

  enum PropertyAttribute attributes =
      static_cast<PropertyAttribute>(v8::ReadOnly | v8::DontDelete);
  t->InstanceTemplate()->SetAccessor(String::New("fd"),
                                     StreamWrap::GetFD,
                                     NULL,
                                     Handle<Value>(),
                                     v8::DEFAULT,
                                     attributes);

  NODE_SET_PROTOTYPE_METHOD(t, "close", StreamWrap::Close);

  NODE_SET_PROTOTYPE_METHOD(t, "ref", HandleWrap::Ref);
  NODE_SET_PROTOTYPE_METHOD(t, "unref", HandleWrap::Unref);

  NODE_SET_PROTOTYPE_METHOD(t, "readStart", StreamWrap::ReadStart);
  NODE_SET_PROTOTYPE_METHOD(t, "readStop", StreamWrap::ReadStop);
  NODE_SET_PROTOTYPE_METHOD(t, "shutdown", StreamWrap::Shutdown);

  NODE_SET_PROTOTYPE_METHOD(t, "writeBuffer", StreamWrap::WriteBuffer);
  NODE_SET_PROTOTYPE_METHOD(t, "writeAsciiString", StreamWrap::WriteAsciiString);
  NODE_SET_PROTOTYPE_METHOD(t, "writeUtf8String", StreamWrap::WriteUtf8String);
  NODE_SET_PROTOTYPE_METHOD(t, "writeUcs2String", StreamWrap::WriteUcs2String);
  NODE_SET_PROTOTYPE_METHOD(t, "writev", StreamWrap::Writev);

  tlsConstructor = Persistent<Function>::New(node_isolate, t->GetFunction());

  target->Set(String::NewSymbol("TLS"), tlsConstructor);
}


TLSWrap* TLSWrap::Unwrap(Local<Object> obj) {
  assert(!obj.IsEmpty());
  assert(obj->InternalFieldCount() > 0);
  return static_cast<TLSWrap*>(obj->GetAlignedPointerFromInternalField(0));
}


uv_stream_t* TLSWrap::UVHandle() {
  return GetStream();
}


Handle<Value> TLSWrap::New(const Arguments& args) {
  // This constructor should not be exposed to public javascript.
  // Therefore we assert that we are not trying to call this as a
  // normal function.
  assert(args.IsConstructCall());

  HandleScope scope(node_isolate);

  uv_stream_t* stream = HandleToStream(args[0].As<Object>());
  assert(stream != NULL);

  SecureContext* sc = ObjectWrap::Unwrap<SecureContext>(args[1]->ToObject());
  SSL* context = SSL_new(sc->ctx_);

  TLSWrap* wrap = new TLSWrap(args.This(), stream, context);
  assert(wrap);

  return scope.Close(args.This());
}


TLSWrap::TLSWrap(Handle<Object> object, uv_stream_t* h, SSL* ssl)
    : StreamWrap(object, h),
      ssl_(ssl),
      bio_read_(BIO_new(NodeBIO::GetMethod())),
      bio_write_(BIO_new(NodeBIO::GetMethod())) {
  UpdateWriteQueueSize();

  SSL_set_app_data(ssl_, this);

  SSL_set_bio(ssl_, bio_read_, bio_write_);

  // SSL_set_accept_state(ssl_);
  SSL_set_connect_state(ssl_);
}


TLSWrap::~TLSWrap() {
  assert(object_.IsEmpty());
  SSL_free(ssl_);
}


uv_buf_t TLSWrap::Alloc(size_t suggested_size) {
  uv_buf_t buf;
  buf.base = (char*)malloc(suggested_size);
  buf.len = suggested_size;
  return buf;
}


void free_cb(char* buf, void* hint) {
  free(buf);
}


Local<Object> TLSWrap::Shrink(uv_buf_t buf, size_t nread) {
  buf.base = (char*)realloc(buf.base, nread);
  Buffer *b = Buffer::New(buf.base, nread, free_cb, NULL);
  return b->handle_->ToObject();
}


void TLSWrap::AfterRead(uv_buf_t buf, size_t nread, uv_handle_type pending) {
  BIO_write(bio_read_, buf.base, nread);

  uv_buf_t rbuf;
  rbuf.len = BIO_ctrl_pending(bio_write_);
  rbuf.base = (char*)malloc(rbuf.len);

  size_t read = SSL_read(ssl_, rbuf.base, rbuf.len);

  StreamWrap::AfterRead(rbuf, read, pending);
}


size_t TLSWrap::BeforeWrite(char* data, size_t len, uv_buf_t* buf) {
  size_t written = SSL_write(ssl_, data, len);

  size_t towrite = BIO_ctrl_wpending(bio_read_);
  char *wbuf = (char*)malloc(towrite);

  size_t nread = BIO_read(bio_write_, wbuf, towrite);

  buf->base = wbuf;
  buf->len = nread;

  return nread;
}


}  // namespace node

NODE_MODULE(node_tls_wrap, node::TLSWrap::Initialize)
