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

#ifndef SRC_ALLOCATOR_H_
#define SRC_ALLOCATOR_H_

#include <stdlib.h>

class Allocator {
  public:
    enum UMC_TYPES {
      UMC_TYPE_Certificate = 0,
      UMC_TYPE_CipherBase,
      UMC_TYPE_Connection,
      UMC_TYPE_ContextifyContext,
      UMC_TYPE_ContextifyScript,
      UMC_TYPE_DiffieHellman,
      UMC_TYPE_ECDH,
      UMC_TYPE_FSEventWrap,
      UMC_TYPE_FSReqWrap,
      UMC_TYPE_FSReqWrapOver,
      UMC_TYPE_GetAddrInfoReqWrap,
      UMC_TYPE_GetNameInfoReqWrap,
      UMC_TYPE_GetHostByAddrWrap,
      UMC_TYPE_Hmac,
      UMC_TYPE_Hash,
      UMC_TYPE_Oversize,
      UMC_TYPE_PBKDF2Request,
      UMC_TYPE_Parser,
      UMC_TYPE_PipeWrap,
      UMC_TYPE_PipeConnectWrap,
      UMC_TYPE_ProcessWrap,
      UMC_TYPE_QueryAWrap,
      UMC_TYPE_QueryAaaaWrap,
      UMC_TYPE_QueryCnameWrap,
      UMC_TYPE_QueryMxWrap,
      UMC_TYPE_QueryNsWrap,
      UMC_TYPE_QueryTxtWrap,
      UMC_TYPE_QuerySrvWrap,
      UMC_TYPE_QueryNaptrWrap,
      UMC_TYPE_QuerySoaWrap,
      UMC_TYPE_RandomBytesRequest,
      UMC_TYPE_SecureContext,
      UMC_TYPE_SendWrap,
      UMC_TYPE_ShutdownWrap,
      UMC_TYPE_SignalWrap,
      UMC_TYPE_Sign,
      UMC_TYPE_StatWatcher,
      UMC_TYPE_StreamWrap,
      UMC_TYPE_TimerWrap,
      UMC_TYPE_TCPWrap,
      UMC_TYPE_TCPConnectWrap,
      UMC_TYPE_TLSCallbacks,
      UMC_TYPE_TTYWrap,
      UMC_TYPE_UDPWrap,
      UMC_TYPE_Verify,
      UMC_TYPE_WriteItem,
      UMC_TYPE_WriteWrap,
      UMC_TYPE_WriteWrapOver,
      UMC_TYPE_ZCtx,
      UMC_TYPE_MAX,
    };

    virtual ~Allocator() {}

    virtual void* Allocate(UMC_TYPES tid,
                           size_t type_size,
                           size_t over_size = 0);
    virtual void Destroy(UMC_TYPES tid, void* ptr);

    static char* UMC2Name(UMC_TYPES tid) {
#pragma GCC diagnostic ignored "-Wwrite-strings"
#pragma GCC diagnostic push
      static char* UMC_TYPE_NAMES[] = {
        "Certificate",
        "CipherBase",
        "Connection",
        "ContextifyContext",
        "ContextifyScript",
        "DiffieHellman",
        "ECDH",
        "FSEventWrap",
        "FSReqWrap",
        "FSReqWrapOver",
        "GetAddrInfoReqWrap",
        "GetNameInfoReqWrap",
        "GetHostByAddrWrap",
        "Hmac",
        "Hash",
        "Oversize",
        "PBKDF2Request",
        "Parser",
        "PipeWrap",
        "PipeConnectWrap",
        "ProcessWrap",
        "QueryAWrap",
        "QueryAaaaWrap",
        "QueryCnameWrap",
        "QueryMxWrap",
        "QueryNsWrap",
        "QueryTxtWrap",
        "QuerySrvWrap",
        "QueryNaptrWrap",
        "QuerySoaWrap",
        "RandomBytesRequest",
        "SecureContext",
        "SendWrap",
        "ShutdownWrap",
        "SignalWrap",
        "Sign",
        "StatWatcher",
        "StreamWrap",
        "TimerWrap",
        "TCPWrap",
        "TCPConnectWrap",
        "TLSCallbacks",
        "TTYWrap",
        "UDPWrap",
        "Verify",
        "WriteItem",
        "WriteWrap",
        "WriteWrapOver",
        "ZCtx",
        NULL,
      };
#pragma GCC diagnostic pop

      return UMC_TYPE_NAMES[tid];
    }
};

extern Allocator* ALLOCATOR;

#define NODE_UMC_NEWOP \
  void* operator new(size_t s, void* f) { return f; } \
  void* operator new(size_t size) { assert(0); } \
  void operator delete(void* ptr) { assert(0); } \
  void operator delete(void* ptr, char* storage) { assert(0); }

#define NODE_UMC_DOALLOC(T) \
  void* storage = ALLOCATOR->Allocate(Allocator::UMC_TYPE_ ## T, sizeof(T)); \

#define NODE_UMC_ALLOCATE(T) \
static T* Allocate(Environment* env, v8::Local<v8::Object> holder) { \
  NODE_UMC_DOALLOC(T) \
  return new(storage) T(env, holder); \
}

#define NODE_UMC_ALLOCATE_AW(T) \
static T* Allocate(Environment* env, \
                   v8::Local<v8::Object> holder, \
                   AsyncWrap* parent) { \
  void* storage = ALLOCATOR->Allocate(Allocator::UMC_TYPE_ ## T, sizeof(T)); \
  return new(storage) T(env, holder, parent); \
}

#define NODE_UMC_DESTROYV(T) \
virtual void Destroy() { \
  this->~T(); \
  ALLOCATOR->Destroy(Allocator::UMC_TYPE_ ## T, this); \
}

#endif  // SRC_ALLOCATOR_H_
