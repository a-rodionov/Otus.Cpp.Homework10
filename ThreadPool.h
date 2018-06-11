#pragma once

#include <iostream>
#include <vector>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <boost/asio.hpp>

class ThreadPool {

public:

  ThreadPool() = default;

  ~ThreadPool() {
    JoinWorkers();
  }

  ThreadPool(const ThreadPool&) = delete;
  ThreadPool& operator=(const ThreadPool&) = delete;
  ThreadPool(ThreadPool&&) = delete;
  ThreadPool& operator=(ThreadPool&&) = delete;

  auto AddWorker() {
    std::lock_guard<std::mutex> lk(threads_mutex);
    if(threads.empty()) {
      work = std::make_shared<boost::asio::io_service::work>(io_service);
    }
    threads.push_back(std::thread([this] () { io_service.run(); }));
    return threads.back().get_id();
  }

  void StopWorkers() {
    std::lock_guard<std::mutex> lk(threads_mutex);
    JoinWorkers();
    threads.clear();
  }

  auto WorkersCount() const {
    std::lock_guard<std::mutex> lk(threads_mutex);
    return threads.size();
  }

  template<typename Task>
  void AddTask(Task&& task) {
    io_service.post(std::forward<Task>(task));
  }

private:

  void JoinWorkers() {
    if(threads.empty()) {
      return;
    }

    work.reset();
    for(auto& thread : threads) {
      if(thread.joinable()) {
        thread.join();
      }
    }
  }

  boost::asio::io_service io_service;
  std::shared_ptr<boost::asio::io_service::work> work;
  std::vector<std::thread> threads;
  mutable std::mutex threads_mutex;

  std::atomic_bool is_workers_stopping{false};
  std::condition_variable_any workers_stopped_event;
};
