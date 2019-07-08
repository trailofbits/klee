/*
 * Copyright (c) 2019 Trail of Bits, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include "runtime/Native/Intrinsics.h"
namespace {


extern "C" {
long strtol_intercept(addr_t nptr, addr_t endptr, int base, Memory *memory);
addr_t malloc_intercept( Memory *memory, uint64_t size);
bool free_intercept( Memory *memory, addr_t ptr);
addr_t calloc_intercept( Memory *memory, uint64_t size);
addr_t realloc_intercept( Memory *memory, addr_t ptr,  uint64_t size);
size_t malloc_size( Memory *memory, addr_t ptr);
}  // extern C

template <typename ABI>
static Memory *Intercept_strtol(Memory *memory, State *state,
                                const ABI &intercept) {
  addr_t nptr = 0;
  addr_t endptr = 0;
  int base = 0;

  if (!intercept.TryGetArgs(memory, state, &nptr, &endptr, &base)) {
    STRACE_ERROR(libc_strtol, "Couldn't get args");
    exit(1);
  }

  long number = strtol_intercept(nptr, endptr, base, memory);

  exit(0);
}


static constexpr addr_t kBadAddr = ~0ULL;

static constexpr addr_t kReallocInternalPtr = ~0ULL - 1ULL;
static constexpr addr_t kReallocTooBig = ~0ULL - 2ULL;
static constexpr addr_t kReallocInvalidPtr = ~0ULL - 3ULL;
static constexpr addr_t kReallocFreedPtr = ~0ULL - 4ULL;

static constexpr addr_t kMallocTooBig = ~0ULL - 1ULL;

template <typename ABI>
static Memory *Intercept_malloc(Memory *memory, State *state,
                                const ABI &intercept) {
  addr_t alloc_size = 0;
  if (!intercept.TryGetArgs(memory, state, &alloc_size)) {
    STRACE_ERROR(malloc, "Couldn't get args");
    return intercept.SetReturn(memory, state, 0);
  }

  if (!alloc_size) {
    STRACE_SUCCESS(libc_malloc, "size=0, ptr=0");
    return intercept.SetReturn(memory, state, 0);
  }

  const auto ptr = malloc_intercept(memory, alloc_size);
  if (ptr == kBadAddr) {
    STRACE_ERROR(libc_malloc, "Falling back to real malloc for size=%" PRIxADDR,
                 alloc_size);
    return memory;

  } else if (ptr == kMallocTooBig) {
    STRACE_ERROR(libc_malloc, "Malloc for size=%" PRIxADDR " too big",
                 alloc_size);
    return memory;

  } else {
    STRACE_SUCCESS(libc_malloc, "size=%" PRIdADDR ", ptr=%" PRIxADDR,
                   alloc_size, ptr);
    return intercept.SetReturn(memory, state, ptr);
  }
}

template <typename ABI>
static Memory *Intercept_free(Memory *memory, State *state,
                              const ABI &intercept) {
  addr_t address = 0;
  if (!intercept.TryGetArgs(memory, state, &address)) {
    STRACE_ERROR(libc_free, "Couldn't get args");
    return intercept.SetReturn(memory, state, 0);
  }

  if (!free_intercept(memory, address)) {
    STRACE_ERROR(libc_free, "Falling back to real free for addr=%" PRIxADDR,
                 address);
    return memory;
  }

  return intercept.SetReturn(memory, state, 0);
}

template <typename ABI>
static Memory *Intercept_calloc(Memory *memory, State *state,
                                const ABI &intercept) {
  addr_t num = 0;
  addr_t size = 0;
  if (!intercept.TryGetArgs(memory, state, &num, &size)) {
    STRACE_ERROR(libc_calloc, "Couldn't get args");
    return intercept.SetReturn(memory, state, 0);
  }

  addr_t alloc_size = num * size;
  if (!alloc_size) {
    STRACE_SUCCESS(libc_calloc, "num=%" PRIxADDR ", size=%" PRIxADDR ", ptr=0", num, size);
    return intercept.SetReturn(memory, state, 0);
  }

  const auto ptr = calloc_intercept(memory, alloc_size);
  if (ptr == kBadAddr) {
    STRACE_ERROR(libc_calloc, "Falling back to real calloc for num=%" PRIxADDR
                 ", size=%" PRIxADDR, num, size);
    return memory;

  } else if (ptr == kMallocTooBig) {
    STRACE_ERROR(libc_calloc, "Calloc for size=%" PRIxADDR " too big",
                 alloc_size);
    return memory;

  } else {
    STRACE_SUCCESS(libc_calloc, "num=%" PRIdADDR ", size=%" PRIdADDR ", ptr=%" PRIxADDR,
                   num, size, ptr);
    return intercept.SetReturn(memory, state, ptr);
  }
}

template <typename ABI>
static Memory *Intercept_realloc(Memory *memory, State *state,
                                 const ABI &intercept) {
  addr_t ptr;
  size_t alloc_size;
  if (!intercept.TryGetArgs(memory, state, &ptr, &alloc_size)) {
    STRACE_ERROR(libc_realloc, "Couldn't get args");
    return intercept.SetReturn(memory, state, 0);
  }

  addr_t new_ptr = 0;
  if (!ptr) {
    new_ptr = malloc_intercept(memory, alloc_size);
  } else {
    new_ptr = realloc_intercept(memory, ptr, alloc_size);
  }

  if (new_ptr == kBadAddr) {
    STRACE_ERROR(libc_realloc, "Falling back to real realloc for ptr=%" PRIxADDR
                 ", size=%" PRIxADDR, ptr, alloc_size);
    return memory;

  } else if (kReallocInternalPtr == new_ptr) {
    STRACE_ERROR(libc_realloc, "Can't realloc displaced malloc addr=" PRIxADDR, ptr);
    klee_abort();

  } else if (kReallocTooBig == new_ptr) {
    STRACE_ERROR(libc_realloc, "Realloc size=%" PRIxADDR " too big", alloc_size);
    klee_abort();

  } else if (kReallocInvalidPtr == new_ptr) {
    STRACE_ERROR(libc_realloc, "Realloc on untracked addr=%" PRIxADDR, ptr);
    klee_abort();

  } else if (kReallocFreedPtr == new_ptr) {
    STRACE_ERROR(libc_realloc, "Realloc on freed addr=%" PRIxADDR, ptr);
    klee_abort();

  } else {
    STRACE_SUCCESS(libc_realloc, "Realloc of ptr=%" PRIxADDR " to %" PRIxADDR,
                   ptr, new_ptr);
    return intercept.SetReturn(memory, state, new_ptr);
  }
}

template <typename ABI>
static Memory *Intercept_memalign(Memory *memory, State *state,
                                  const ABI &intercept) {
  size_t alignment;
  size_t size;
  if (!intercept.TryGetArgs(memory, state, &alignment, &size)) {
    STRACE_ERROR(libc_memalign, "Couldn't get args");
    return intercept.SetReturn(0, state, 0);
  }
  const auto ptr = malloc_intercept(memory, size);
  if (ptr == kBadAddr) {
    STRACE_ERROR(libc_memalign, "Falling back to real memalign for align=%"
                 PRIxADDR ", size=%" PRIxADDR, alignment, size);
    return memory;
  }
  return intercept.SetReturn(memory, state, ptr);
}


template <typename ABI>
static Memory *Intercept_malloc_usable_size(Memory *memory, State *state,
                                            const ABI &intercept) {
  addr_t ptr;
  if (!intercept.TryGetArgs(memory, state, &ptr)) {
    STRACE_ERROR(read, "Couldn't get args");
    return intercept.SetReturn(memory, state, 0);
  }

  const auto size = malloc_size(memory, ptr);
  if (!size) {
    STRACE_ERROR(
        libc_malloc_usable_size, "Falling back to real malloc_usable_size for ptr=%"
        PRIxADDR, ptr);
    return memory;
  }
  return intercept.SetReturn(memory, state, size);
}

}  // namespace
