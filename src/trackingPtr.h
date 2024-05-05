#pragma once

#include <napi.h>

namespace Napi {
/**
 * An std::shared_ptr derivative that can track memory
 * through the V8 GC
 */
template <typename T> class TrackingPtr : public std::shared_ptr<T> {
  Env env_;
  int64_t size_;

public:
  inline TrackingPtr(Env env, int64_t size, T *ptr) : std::shared_ptr<T>{ptr}, env_{env}, size_(size) {
    MemoryManagement::AdjustExternalMemory(env_, sizeof(T) + size_);
  }
  inline TrackingPtr(Env env, T *ptr) : std::shared_ptr<T>{ptr}, env_{env}, size_(0) {
    MemoryManagement::AdjustExternalMemory(env_, sizeof(T));
  }
  inline TrackingPtr() : std::shared_ptr<T>{}, env_{nullptr}, size_(0) {}
  virtual ~TrackingPtr() {
    if (this->get() != nullptr) {
      MemoryManagement::AdjustExternalMemory(env_, -(sizeof(T) + size_));
    }
  };
  virtual TrackingPtr &operator=(T *ptr) {
    if (this->get() != nullptr) {
      MemoryManagement::AdjustExternalMemory(env_, -(sizeof(T) + size_));
    }
    size_ = 0;
    MemoryManagement::AdjustExternalMemory(env_, sizeof(T));
    return *this;
  };
};
template <typename T, typename... ARGS> inline TrackingPtr<T> MakeTracking(Env env, int64_t size, ARGS &&...args) {
  return TrackingPtr<T>{env, size, new T(std::forward<ARGS>(args)...)};
}
template <typename T> inline TrackingPtr<T> MakeTracking(Env env) { return TrackingPtr<T>{env, new T}; }
} // namespace Napi
