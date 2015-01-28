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

#include "allocator_umem.h"

#include <assert.h>

UmemCache::UmemCache() {
  for (size_t i = 0; i < UMC_TYPE_MAX; i++) {
    umem_cache_[i] = NULL;
  }
}

UmemCache::~UmemCache() {
  for (size_t i = 0; i < UMC_TYPE_MAX; i++)
    if (umem_cache_[i] != NULL)
      umem_cache_destroy(this->umem_cache_[i]);
}

void* UmemCache::Allocate(UMC_TYPES tid,
                          size_t type_size,
                          size_t over_size) {
  umem_cache_t* cache = umem_cache_[tid];

  if (tid != UMC_TYPE_Oversize && cache == NULL) {
    if (tid == UMC_TYPE_FSReqWrapOver)
      over_size = PATH_MAX;

    if (tid == UMC_TYPE_WriteWrapOver)
      over_size = 16 + 16384;

    cache = umem_cache_create(Allocator::UMC2Name(tid), type_size + over_size,
                              0, NULL, NULL, NULL,
                              NULL, NULL, 0);

    this->umem_cache_[tid] = cache;
  }

  if (tid != UMC_TYPE_Oversize)
    return umem_cache_alloc(cache, UMEM_DEFAULT);
  else
    return malloc(type_size + over_size);
}


void UmemCache::Destroy(UMC_TYPES tid, void* ptr) {
  if (tid != UMC_TYPE_Oversize) {
    umem_cache_t* cache = this->umem_cache_[tid];
    assert(cache != NULL);
    umem_cache_free(cache, ptr);
  } else {
    free(ptr);
  }
}
