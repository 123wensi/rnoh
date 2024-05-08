#include <glog/logging.h>
#include <napi/native_api.h>
#include <uv.h>
#include <atomic>

#include "NapiTaskRunner.h"

namespace rnoh {

NapiTaskRunner::NapiTaskRunner(napi_env env, ExceptionHandler exceptionHandler)
    : env(env), exceptionHandler(std::move(exceptionHandler)) {
  // NOTE: let's hope the JS runtime doesn't move between system threads...
  threadId = std::this_thread::get_id();
  auto loop = getLoop();
  asyncHandle = new uv_async_t;
  asyncHandle->data = static_cast<void*>(this);
  uv_async_init(loop, asyncHandle, [](auto handle) {
    auto runner = static_cast<NapiTaskRunner*>(handle->data);

    // https://nodejs.org/api/n-api.html#napi_handle_scope
    // "For any invocations of code outside the execution of a native method
    // (...) the module is required to create a scope before invoking any
    // functions that can result in the creation of JavaScript values"
    napi_handle_scope scope;
    auto result = napi_open_handle_scope(runner->env, &scope);
    if (result != napi_ok) {
      LOG(ERROR) << "Failed to open handle scope";
      return;
    }

    std::queue<Task> tasksQueue;
    {
      std::unique_lock<std::mutex> lock(runner->tasksMutex);
      std::swap(tasksQueue, runner->tasksQueue);
    }
    while (!tasksQueue.empty()) {
      auto task = std::move(tasksQueue.front());
      tasksQueue.pop();
      try {
        task();
      } catch (std::exception const& e) {
        runner->exceptionHandler(std::current_exception());
      }
    }

    result = napi_close_handle_scope(runner->env, scope);
    if (result != napi_ok) {
      LOG(ERROR) << "Failed to close handle scope";
      return;
    }
  });
}

NapiTaskRunner::~NapiTaskRunner() {
  running->store(false);
  cv.notify_all();
  uv_close(reinterpret_cast<uv_handle_t*>(asyncHandle),
    [] (uv_handle_t* handle) {
      delete (uv_async_t*) handle;
    }
  );
}

void NapiTaskRunner::runAsyncTask(Task&& task) {
  std::unique_lock<std::mutex> lock(tasksMutex);
  tasksQueue.push(task);
  uv_async_send(asyncHandle);
}

void NapiTaskRunner::runSyncTask(Task&& task) {
  if (isOnCurrentThread()) {
    task();
    return;
  }
  std::unique_lock<std::mutex> lock(tasksMutex);
  std::atomic_bool done{false};
  tasksQueue.push([this, &done, task = std::move(task)]() {
    task();
    done = true;
    cv.notify_all();
  });
  uv_async_send(asyncHandle);
  cv.wait(lock, [running = this->running, &done] {
    return !(running->load()) || done.load();
  });
}

bool NapiTaskRunner::isOnCurrentThread() const {
  return threadId == std::this_thread::get_id();
}

void NapiTaskRunner::setExceptionHandler(ExceptionHandler handler) {
  exceptionHandler = std::move(handler);
}

uv_loop_t* NapiTaskRunner::getLoop() const {
  uv_loop_t* loop = nullptr;
  napi_get_uv_event_loop(env, &loop);
  return loop;
}

} // namespace rnoh
