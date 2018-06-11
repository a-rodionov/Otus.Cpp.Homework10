#pragma once

#include <iostream>
#include <vector>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <boost/asio.hpp>
#include <boost/log/trivial.hpp>

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
    io_service.post([this, task] () {
      try {
        task();
      }
      catch (std::exception& exc) {
        BOOST_LOG_TRIVIAL(error) << exc.what();
        std::lock_guard<std::mutex> lk(exceptions_mutex);
        exceptions.push(std::current_exception());
      }
    });
  }

  std::exception_ptr GetLastException() {
    std::exception_ptr exc;
    std::lock_guard<std::mutex> lk(exceptions_mutex);
    if (!exceptions.empty()) {
      exc = exceptions.front();
      exceptions.pop();
    }
    return exc;
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

  std::queue<std::exception_ptr> exceptions;
  std::mutex exceptions_mutex;
};
