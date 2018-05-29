#pragma once

#include <thread>
#include <condition_variable>
#include <atomic>
#include <queue>

template<typename MessageType, typename ThreadHandler>
class ThreadPool {

public:

  ThreadPool()
    : done{false} {}

  ~ThreadPool() {
    StopWorkers();
  }

  template<typename ... Args>
  void AddWorker(Args&& ... args) {
    std::lock_guard<std::mutex> lk(threads_mutex);
    thread_handlers.push_back(std::make_shared<ThreadHandler>(std::forward<Args>(args)...));
    threads.push_back(std::thread(&ThreadPool::WorkerThread, this, thread_handlers.back()));
  }

  std::list<std::shared_ptr<ThreadHandler>> StopWorkers() {
    std::lock_guard<std::mutex> lk(threads_mutex);
    if(threads.empty())
      return thread_handlers;
    done = true;
    queue_event.notify_all();
    for(auto& thread : threads) {
      if(thread.joinable())
        thread.join();
    }
    return thread_handlers;
  }

  void PushMessage(const MessageType& message) {
    std::lock_guard<std::mutex> lk(queue_mutex);
    messages.push(message);
    queue_event.notify_one();
  }

  void PushMessage(MessageType&& message) {
    std::lock_guard<std::mutex> lk(queue_mutex);
    messages.push(std::move(message));
    queue_event.notify_one();
  }

private:

  void WorkerThread(const std::shared_ptr<ThreadHandler>& thread_handler) {
    while(true) {
      std::unique_lock<std::mutex> lk(queue_mutex);
      queue_event.wait(lk, [&](){ return (!messages.empty() || done); });
      if(messages.empty())
        break;
      auto message = messages.front();
      messages.pop();
      lk.unlock();
      (*thread_handler)(message);
    }
  }

  std::vector<std::thread> threads;
  std::list<std::shared_ptr<ThreadHandler>> thread_handlers;
  std::mutex threads_mutex;
  std::atomic_bool done;

  std::queue<MessageType> messages;
  std::mutex queue_mutex;
  std::condition_variable queue_event;
};
