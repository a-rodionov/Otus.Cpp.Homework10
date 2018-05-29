#pragma once

#include <iostream>
#include "IOutput.h"
#include "ThreadPool.h"
#include "Statistics.h"

class ConsoleOutputThreadHandler : public BaseStatistics
{

public:

  explicit ConsoleOutputThreadHandler(std::ostream& out)
    : out{out} {}

  void operator()(const std::list<std::string>& data) {
    Output(out, data);
    ++statistics.blocks;
    statistics.commands += data.size();
  }

private:

  std::ostream& out;
};

class ConsoleOutput : public IOutput, public ThreadPool<std::list<std::string>, ConsoleOutputThreadHandler>
{

public:

  explicit ConsoleOutput(std::ostream& out, size_t threads_count = 1)
    : out{out}
  {
    for(decltype(threads_count) i{0}; i < threads_count; ++i)
      AddWorker();
  }

  void AddWorker() {
    ThreadPool::AddWorker(out);
  }

  void Output(const std::size_t timestamp, const std::list<std::string>& data) override {
    PushMessage(data);
  }

private:

  std::ostream& out;
};
