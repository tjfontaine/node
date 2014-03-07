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

#ifndef SRC_ASYNC_WRAP_INL_H_
#define SRC_ASYNC_WRAP_INL_H_

#include "async-wrap.h"
#include "base-object.h"
#include "base-object-inl.h"
#include "env.h"
#include "env-inl.h"
#include "util.h"
#include "util-inl.h"

#include "v8.h"
#include <assert.h>

namespace node {

inline AsyncWrap::AsyncWrap(Environment* env,
                            v8::Handle<v8::Object> object,
                            ProviderType provider)
    : BaseObject(env, object),
      provider_type_(provider) {

  v8::TryCatch try_catch;
  try_catch.SetVerbose(true);

  Fire(EVENT_CREATE);
}


inline AsyncWrap::~AsyncWrap() {
}


inline void AsyncWrap::Fire(AsyncWrap::EventType event) {
  if ((env()->watched_providers() & provider_type()) == 0)
    return;

  if ((env()->watched_events() & event) == 0) {
    return;

  if (provider_type() == PROVIDER_TIMERWRAP)
    return;

  v8::Local<v8::Value> val_v[] = {
    v8::Integer::NewFromUnsigned(provider_type(), env()->isolate()),
    v8::Integer::NewFromUnsigned(event),
    object().As<v8::Value>()
  };

  env()->async_listener_handler_function()->Call(env()->process_object(),
                                                 ARRAY_SIZE(val_v),
                                                 val_v);
}


inline AsyncWrap::ProviderType AsyncWrap::provider_type() const {
  return provider_type_;
}


inline v8::Handle<v8::Value> AsyncWrap::MakeCallback(
    const v8::Handle<v8::Function> cb,
    int argc,
    v8::Handle<v8::Value>* argv) {
  assert(env()->context() == env()->isolate()->GetCurrentContext());

  v8::Local<v8::Object> context = object();
  v8::Local<v8::Object> process = env()->process_object();
  v8::Local<v8::Value> domain_v;
  v8::Local<v8::Object> domain;
  bool has_domain = false;

  v8::TryCatch try_catch;
  try_catch.SetVerbose(true);

  env()->set_provider_type(provider_type());

  Fire(EVENT_BEFORE);

  if (try_catch.HasCaught())
    return v8::Undefined(env()->isolate());

  if (env()->using_domains()) {
    domain_v = context->Get(env()->domain_string());
    has_domain = domain_v->IsObject();
  }

  if (has_domain) {
    domain = domain_v.As<v8::Object>();

    if (domain->Get(env()->disposed_string())->IsTrue())
      return Undefined(env()->isolate());

    v8::Local<v8::Function> enter =
      domain->Get(env()->enter_string()).As<v8::Function>();
    assert(enter->IsFunction());
    enter->Call(domain, 0, NULL);

    if (try_catch.HasCaught())
      return Undefined(env()->isolate());
  }

  v8::Local<v8::Value> ret = cb->Call(context, argc, argv);

  if (try_catch.HasCaught())
    return Undefined(env()->isolate());

  if (has_domain) {
    v8::Local<v8::Function> exit =
      domain->Get(env()->exit_string()).As<v8::Function>();
    assert(exit->IsFunction());
    exit->Call(domain, 0, NULL);

    if (try_catch.HasCaught())
      return Undefined(env()->isolate());
  }

  Fire(EVENT_AFTER);

  if (try_catch.HasCaught())
    return Undefined(env()->isolate());

  Environment::TickInfo* tick_info = env()->tick_info();

  if (tick_info->in_tick())
    return ret;

  if (tick_info->length() == 0) {
    tick_info->set_index(0);
    return ret;
  }

  tick_info->set_in_tick(true);

  env()->tick_callback_function()->Call(process, 0, NULL);

  tick_info->set_in_tick(false);

  if (try_catch.HasCaught()) {
    tick_info->set_last_threw(true);
    return Undefined(env()->isolate());
  }

  env()->set_provider_type(PROVIDER_NONE);

  return ret;
}


inline v8::Handle<v8::Value> AsyncWrap::MakeCallback(
    const v8::Handle<v8::String> symbol,
    int argc,
    v8::Handle<v8::Value>* argv) {
  v8::Local<v8::Value> cb_v = object()->Get(symbol);
  v8::Local<v8::Function> cb = cb_v.As<v8::Function>();
  assert(cb->IsFunction());

  return MakeCallback(cb, argc, argv);
}


inline v8::Handle<v8::Value> AsyncWrap::MakeCallback(
    uint32_t index,
    int argc,
    v8::Handle<v8::Value>* argv) {
  v8::Local<v8::Value> cb_v = object()->Get(index);
  v8::Local<v8::Function> cb = cb_v.As<v8::Function>();
  assert(cb->IsFunction());

  return MakeCallback(cb, argc, argv);
}

}  // namespace node

#endif  // SRC_ASYNC_WRAP_INL_H_
