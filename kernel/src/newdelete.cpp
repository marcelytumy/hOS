#include <cstddef>
#include <new>

// Minimal global new/delete operators for freestanding kernel
// No allocator yet; new will halt to avoid silent misuse.

extern "C" void hcf(); // from main.cpp

void *operator new(std::size_t) {
  hcf();
  return nullptr;
}

void *operator new[](std::size_t) {
  hcf();
  return nullptr;
}

void *operator new(std::size_t, std::align_val_t) {
  hcf();
  return nullptr;
}

void *operator new[](std::size_t, std::align_val_t) {
  hcf();
  return nullptr;
}

void operator delete(void *) noexcept {}
void operator delete[](void *) noexcept {}

void operator delete(void *, std::size_t) noexcept {}
void operator delete[](void *, std::size_t) noexcept {}

void operator delete(void *, std::align_val_t) noexcept {}
void operator delete[](void *, std::align_val_t) noexcept {}

void operator delete(void *, std::size_t, std::align_val_t) noexcept {}
void operator delete[](void *, std::size_t, std::align_val_t) noexcept {}

// Itanium C++ ABI guard variables for thread-safe local statics
extern "C" int __cxa_guard_acquire(long long *guard) {
  if (*guard)
    return 0;
  *guard = 1;
  return 1;
}
extern "C" void __cxa_guard_release(long long * /*guard*/) {}
extern "C" void __cxa_guard_abort(long long * /*guard*/) {}
