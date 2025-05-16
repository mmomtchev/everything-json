#include "jsonAsync.h"

// Process the micro task queue
// (this is a process.nextTick re-implemented in C++)
void JSON::ProcessRunQueue(uv_async_t *handle) {
  const auto start(high_resolution_clock::now());

  auto instance = static_cast<InstanceData *>(handle->data);
  while (!instance->runQueue.empty() && CanRun(start)) {
    // An operation that has finished will have its state
    // deleted by args going out of scope
    auto args = instance->runQueue.front();
    ToObjectAsync(args, start);
    instance->runQueue.pop();
  }

  if (!instance->runQueue.empty()) {
    // More work, ask libuv to call us back after
    // one full event loop iteration
    uv_async_send(handle);
  } else {
    // No more work, do not block the process exit
    uv_unref(reinterpret_cast<uv_handle_t *>(handle));
  }

}

void JSON::ProcessExternalMemory(Napi::Env env) {
  auto instance = env.GetInstanceData<InstanceData>();
  std::lock_guard guard{instance->lock};
  if (instance->pendingExternalMemoryAdjustment != 0) {
    Napi::MemoryManagement::AdjustExternalMemory(env, instance->pendingExternalMemoryAdjustment);
    instance->pendingExternalMemoryAdjustment = 0;
  }
}
