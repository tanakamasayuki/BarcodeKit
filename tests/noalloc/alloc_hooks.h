// Counting replacements for malloc/free and new/delete.
//
// In a header rather than in the .ino because the Arduino preprocessor injects
// its generated prototypes into the top of a sketch, which lands inside an
// `extern "C"` block and gives setup()/loop() C linkage. Headers next to the
// sketch are left alone.
//
// The replacements forward to glibc's real implementations so the rest of the
// sketch keeps working; this is a host-only test, which is where the suite
// runs anyway.

#pragma once

#include <stddef.h>
#include <stdlib.h>

namespace alloc_hooks {

// volatile: the counters are read either side of a call the compiler can see
// through, and we do not want the reads folded away.
inline volatile unsigned long &allocCount() {
  static volatile unsigned long value = 0;
  return value;
}

inline volatile unsigned long &freeCount() {
  static volatile unsigned long value = 0;
  return value;
}

}  // namespace alloc_hooks

extern "C" void *__libc_malloc(size_t);
extern "C" void *__libc_calloc(size_t, size_t);
extern "C" void *__libc_realloc(void *, size_t);
extern "C" void __libc_free(void *);

extern "C" void *malloc(size_t n) {
  alloc_hooks::allocCount()++;
  return __libc_malloc(n);
}

extern "C" void *calloc(size_t n, size_t m) {
  alloc_hooks::allocCount()++;
  return __libc_calloc(n, m);
}

extern "C" void *realloc(void *p, size_t n) {
  alloc_hooks::allocCount()++;
  return __libc_realloc(p, n);
}

extern "C" void free(void *p) {
  alloc_hooks::freeCount()++;
  __libc_free(p);
}

inline void *operator new(size_t n) {
  alloc_hooks::allocCount()++;
  return __libc_malloc(n);
}

inline void *operator new[](size_t n) {
  alloc_hooks::allocCount()++;
  return __libc_malloc(n);
}

inline void operator delete(void *p) noexcept {
  alloc_hooks::freeCount()++;
  __libc_free(p);
}

inline void operator delete[](void *p) noexcept {
  alloc_hooks::freeCount()++;
  __libc_free(p);
}

inline void operator delete(void *p, size_t) noexcept {
  alloc_hooks::freeCount()++;
  __libc_free(p);
}

inline void operator delete[](void *p, size_t) noexcept {
  alloc_hooks::freeCount()++;
  __libc_free(p);
}
