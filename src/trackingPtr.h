#pragma once

#include <memory>
#include <napi.h>

namespace Napi {

/**
 * An std::shared_ptr derivative that can track memory
 * through the V8 GC
 */

template <typename T, typename... ARGS> inline std::shared_ptr<T> MakeTracking(Env env, int64_t size, ARGS &&...args) {
  MemoryManagement::AdjustExternalMemory(env, size + sizeof(T));
  return std::shared_ptr<T>{new T(std::forward<ARGS>(args)...), [env, size](void *p) {
                              MemoryManagement::AdjustExternalMemory(env, -(size + sizeof(T)));
                              delete static_cast<T *>(p);
                            }};
}
template <typename T> inline std::shared_ptr<T> MakeTracking(Env) { return std::shared_ptr<T>{new T}; }
} // namespace Napi
