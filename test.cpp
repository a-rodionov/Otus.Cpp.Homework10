#include <sys/file.h>
#include <sstream>
#include "Storage.h"
#include "ConsoleOutput.h"
#include "FileOutput.h"
#include "CommandProcessor.h"
#include "ThreadPool.h"

#define BOOST_TEST_MODULE test_main

#include <boost/test/unit_test.hpp>
#include <boost/test/included/unit_test.hpp>


BOOST_AUTO_TEST_SUITE(test_suite_internals)

BOOST_AUTO_TEST_CASE(infix_iterator)
{
  std::list<int> data{0,1,2,3};
  std::string result{"0, 1, 2, 3"};
  std::ostringstream oss;

  std::copy(std::cbegin(data),
            std::cend(data),
            infix_ostream_iterator<decltype(data)::value_type>{oss, ", "});
  BOOST_CHECK_EQUAL(oss.str(), result);
}

BOOST_AUTO_TEST_CASE(observable)
{
  class TestObservable : public Observable<IOutput>
  {
  public:
    auto GetSubscribers() const {
      return subscribers;
    }
    auto GetSubscribersCount() const {
      return subscribers.size();
    }
  };

  TestObservable testObservable;
  auto consoleOutput = std::make_shared<ConsoleOutput>(std::cout);
  bool isSubscriberFound{false};

  BOOST_CHECK_EQUAL(0, testObservable.GetSubscribersCount());
  
  testObservable.Subscribe(consoleOutput);
  BOOST_CHECK_EQUAL(1, testObservable.GetSubscribersCount());

  auto subscribers = testObservable.GetSubscribers();
  for(auto subscr = std::cbegin(subscribers); subscr != std::cend(subscribers); ++subscr) {
    auto subscriber_locked = subscr->lock();
    if(subscriber_locked.get() == consoleOutput.get()) {
      isSubscriberFound = true;
      break;
    }
  }
  BOOST_CHECK_EQUAL(true, isSubscriberFound);

  testObservable.Unsubscribe(consoleOutput);
  BOOST_CHECK_EQUAL(0, testObservable.GetSubscribersCount());
}

BOOST_AUTO_TEST_SUITE_END()



struct initialized_command_processor
{
  initialized_command_processor()
  {
    commandProcessor = std::make_unique<CommandProcessor>();
    storage = std::make_shared<Storage>(3);
    consoleOutput = std::make_shared<ConsoleOutput>(oss);

    storage->Subscribe(consoleOutput);
    commandProcessor->Subscribe(storage);
  }

  std::unique_ptr<CommandProcessor> commandProcessor;
  std::shared_ptr<Storage> storage;
  std::shared_ptr<ConsoleOutput> consoleOutput;
  std::ostringstream oss;
};

BOOST_FIXTURE_TEST_SUITE(fixture_test_suite_parser, initialized_command_processor)

BOOST_AUTO_TEST_CASE(flush_incomplete_block_by_end)
{
  std::string testData{"cmd1\n"
                      "cmd2\n"
                      "cmd3\n"
                      "cmd4\n"
                      "cmd5\n"};
  std::string result{
    "bulk: cmd1, cmd2, cmd3\n"
    "bulk: cmd4, cmd5\n"
  };
  std::istringstream iss(testData);
  
  commandProcessor->Process(iss);
  consoleOutput->StopWorkers();

  BOOST_CHECK_EQUAL(oss.str(), result);
}

BOOST_AUTO_TEST_CASE(new_block_size)
{
  std::string testData{"cmd1\n"
                      "cmd2\n"
                      "cmd3\n"
                      "{\n"
                      "cmd4\n"
                      "\n"
                      "cmd6\n"
                      "cmd7\n"
                      "}\n"
                      "cmd8\n"
                      "cmd9\n"
                      "cmd10\n"
                      "cmd11\n"};
  std::string result{
    "bulk: cmd1, cmd2, cmd3\n"
    "bulk: cmd4, , cmd6, cmd7\n"
    "bulk: cmd8, cmd9, cmd10\n"
    "bulk: cmd11\n"
  };
  std::istringstream iss(testData);

  commandProcessor->Process(iss);
  consoleOutput->StopWorkers();

  BOOST_CHECK_EQUAL(oss.str(), result);
}

BOOST_AUTO_TEST_CASE(flush_incomplete_block_by_new_block_size)
{
  std::string testData{"cmd1\n"
                      "cmd2\n"
                      "{\n"
                      "cmd3\n"
                      "cmd4\n"
                      "\n"
                      "cmd6\n"
                      "}\n"};
  std::string result{
    "bulk: cmd1, cmd2\n"
    "bulk: cmd3, cmd4, , cmd6\n"
  };
  std::istringstream iss(testData);

  commandProcessor->Process(iss);
  consoleOutput->StopWorkers();

  BOOST_CHECK_EQUAL(oss.str(), result);
}

BOOST_AUTO_TEST_CASE(flush_incomplete_block_by_closing_brace)
{
  std::string testData{"cmd1\n"
                      "cmd2\n"
                      "}\n"
                      "cmd3\n"
                      "cmd4\n"
                      "\n"
                      "cmd6\n"
                      "}\n"};
  std::string result{
    "bulk: cmd1, cmd2\n"
    "bulk: cmd3, cmd4, \n"
    "bulk: cmd6\n"
  };
  std::istringstream iss(testData);

  commandProcessor->Process(iss);
  consoleOutput->StopWorkers();

  BOOST_CHECK_EQUAL(oss.str(), result);
}

BOOST_AUTO_TEST_CASE(ignore_nested_new_block_size)
{
  std::string testData{"{\n"
                      "cmd1\n"
                      "cmd2\n"
                      "{\n"
                      "cmd3\n"
                      "cmd4\n"
                      "}\n"
                      "\n"
                      "cmd6\n"
                      "}\n"};
  std::string result{
    "bulk: cmd1, cmd2, cmd3, cmd4, , cmd6\n"
  };
  std::istringstream iss(testData);

  commandProcessor->Process(iss);
  consoleOutput->StopWorkers();
  
  BOOST_CHECK_EQUAL(oss.str(), result);
}

BOOST_AUTO_TEST_CASE(incomplete_new_block_size)
{
  std::string testData{"cmd1\n"
                      "cmd2\n"
                      "cmd3\n"
                      "{\n"
                      "cmd4\n"
                      "cmd5\n"
                      "cmd6\n"
                      "cmd7\n"};
  std::string result{
    "bulk: cmd1, cmd2, cmd3\n"
  };
  std::istringstream iss(testData);

  commandProcessor->Process(iss);
  consoleOutput->StopWorkers();
  
  BOOST_CHECK_EQUAL(oss.str(), result);
}

BOOST_AUTO_TEST_CASE(command_after_brace_on_same_line_1)
{
  std::string testData{"cmd1\n"
                      "cmd2\n"
                      "{cmd_in_wrong_place\n"
                      "cmd3\n"
                      "cmd4\n"
                      "\n"
                      "cmd6\n"
                      "}\n"};
  std::string result{
    "bulk: cmd1, cmd2, {cmd_in_wrong_place\n"
    "bulk: cmd3, cmd4, \n"
    "bulk: cmd6\n"
  };
  std::istringstream iss(testData);

  commandProcessor->Process(iss);
  consoleOutput->StopWorkers();

  BOOST_CHECK_EQUAL(oss.str(), result);
}

BOOST_AUTO_TEST_CASE(command_after_brace_on_same_line_2)
{
  std::string testData{"cmd1\n"
                      "cmd2\n"
                      "cmd3\n"
                      "{\n"
                      "cmd4\n"
                      "\n"
                      "cmd6\n"
                      "cmd7\n"
                      "}cmd_in_wrong_place\n"
                      "cmd8\n"};
  std::string result{
    "bulk: cmd1, cmd2, cmd3\n"
  };
  std::istringstream iss(testData);

  commandProcessor->Process(iss);
  consoleOutput->StopWorkers();

  BOOST_CHECK_EQUAL(oss.str(), result);
}

BOOST_AUTO_TEST_CASE(brace_after_command_on_same_line)
{
  std::string testData{"cmd1\n"
                      "cmd2{\n"
                      "cmd3\n"
                      "cmd4\n"
                      "\n"};
  std::string result{
    "bulk: cmd1, cmd2{, cmd3\n"
    "bulk: cmd4, \n"
  };
  std::istringstream iss(testData);

  commandProcessor->Process(iss);
  consoleOutput->StopWorkers();

  BOOST_CHECK_EQUAL(oss.str(), result);
}

BOOST_AUTO_TEST_SUITE_END()



BOOST_AUTO_TEST_SUITE(test_suite_file_output)

BOOST_AUTO_TEST_CASE(file_output_simple)
{
  FileOutputThreadHandler fileOutput;
  std::string result;
  std::string goodResult{"bulk: cmd1, cmd2, cmd3"};
  std::list<std::string> testData{"cmd1", "cmd2", "cmd3"};
  size_t timestamp {123};
  unsigned short counter {0};
  auto filename = MakeFilename(timestamp, std::this_thread::get_id(), counter);

  std::remove(filename.c_str());  

  fileOutput(std::make_pair(timestamp, testData));
  
  std::ifstream ifs{filename.c_str(), std::ifstream::in};
  BOOST_CHECK_EQUAL(false, ifs.fail());

  std::getline(ifs, result);
  BOOST_CHECK_EQUAL(goodResult, result);

  std::getline(ifs, result);
  BOOST_CHECK_EQUAL(true, ifs.eof());
  goodResult.clear();
  BOOST_CHECK_EQUAL(goodResult, result);

  std::remove(filename.c_str());
}

BOOST_AUTO_TEST_CASE(file_output_to_locked_file)
{
  FileOutputThreadHandler fileOutput;
  std::list<std::string> testData{"cmd1", "cmd2", "cmd3"};
  size_t timestamp {123};
  unsigned short counter {0};
  auto filename = MakeFilename(timestamp, std::this_thread::get_id(), counter);

  std::remove(filename.c_str());

  auto file_handler = open(filename.c_str(), O_CREAT);
  BOOST_REQUIRE_EQUAL(true, -1 != file_handler);
  BOOST_REQUIRE_EQUAL(true, -1 != flock( file_handler, LOCK_EX | LOCK_NB ));
  
  BOOST_CHECK_THROW(fileOutput(std::make_pair(timestamp, testData)), std::runtime_error);
  
  flock(file_handler, LOCK_UN | LOCK_NB);
  close(file_handler);
  std::remove(filename.c_str());
}

BOOST_AUTO_TEST_SUITE_END()



using namespace std::chrono_literals;

class string_concat_thread_worker
{
public:

  string_concat_thread_worker()
    : calls_count{0} {}

  explicit string_concat_thread_worker(const std::string& initial_data)
    : concatenated_str{initial_data}, calls_count{0} {}

  void operator()(const std::string& str) {
    if(0 == calls_count) {
      thread_id = std::this_thread::get_id();
    }
    concatenated_str += str;
    ++calls_count;
    std::this_thread::sleep_for(200ms);
  }

  auto GetConcatenatedString() const {
    return concatenated_str;
  }

  auto GetCallsCount() const {
    return calls_count;
  }

  auto GetThreadId() const {
    if(0 == calls_count)
      throw std::runtime_error("Worker hasn't been called yet.");
    return thread_id;
  }

private:

  std::string concatenated_str;
  std::size_t calls_count;
  std::thread::id thread_id;
};

BOOST_AUTO_TEST_SUITE(test_suite_thread_pool)

BOOST_AUTO_TEST_CASE(adding_worker_threads)
{
  ThreadPool<std::string, string_concat_thread_worker> thread_pool;
  thread_pool.AddWorker();
  thread_pool.AddWorker();
  thread_pool.AddWorker();

  BOOST_REQUIRE_EQUAL(3, thread_pool.WorkersCount());

  auto thread_handlers = thread_pool.StopWorkers();
  BOOST_REQUIRE_EQUAL(3, thread_handlers.size());
}

BOOST_AUTO_TEST_CASE(verify_multithreaded_execution)
{
  ThreadPool<std::string, string_concat_thread_worker> thread_pool;
  thread_pool.PushMessage("1st part.");
  thread_pool.PushMessage("2nd part.");
  thread_pool.PushMessage("3rd part.");
  thread_pool.AddWorker();
  thread_pool.AddWorker();
  auto thread_handlers = thread_pool.StopWorkers();
  auto first_thread_handler = std::cbegin(thread_handlers);
  auto second_thread_handler = std::next(first_thread_handler);

  BOOST_REQUIRE((*first_thread_handler)->GetThreadId() != (*second_thread_handler)->GetThreadId());
  BOOST_REQUIRE(std::this_thread::get_id() != (*first_thread_handler)->GetThreadId());
  BOOST_REQUIRE(std::this_thread::get_id() != (*second_thread_handler)->GetThreadId());
}

BOOST_AUTO_TEST_CASE(pass_worker_initial_data)
{
  ThreadPool<std::string, string_concat_thread_worker> thread_pool;
  thread_pool.AddWorker("Worker's initial data.");
  thread_pool.PushMessage("1st part.");
  thread_pool.PushMessage("2nd part.");
  auto thread_handlers = thread_pool.StopWorkers();

  BOOST_REQUIRE_EQUAL(2, thread_handlers.front()->GetCallsCount());
  BOOST_REQUIRE_EQUAL("Worker's initial data.1st part.2nd part.", thread_handlers.front()->GetConcatenatedString());
}

BOOST_AUTO_TEST_CASE(pushing_messages_before_start)
{
  ThreadPool<std::string, string_concat_thread_worker> thread_pool;

  thread_pool.PushMessage("1st part.");

  thread_pool.AddWorker();  
  thread_pool.PushMessage("2nd part.");
  auto thread_handlers = thread_pool.StopWorkers();

  BOOST_REQUIRE_EQUAL(2, thread_handlers.front()->GetCallsCount());
  BOOST_REQUIRE_EQUAL("1st part.2nd part.", thread_handlers.front()->GetConcatenatedString());
}

BOOST_AUTO_TEST_CASE(pushing_messages_after_stop)
{
  ThreadPool<std::string, string_concat_thread_worker> thread_pool;
  thread_pool.AddWorker();
  thread_pool.PushMessage("1st part.");
  thread_pool.PushMessage("2nd part.");

  auto thread_handlers = thread_pool.StopWorkers();

  thread_pool.PushMessage("Data won't be processed.");

  BOOST_REQUIRE_EQUAL(2, thread_handlers.front()->GetCallsCount());
  BOOST_REQUIRE_EQUAL("1st part.2nd part.", thread_handlers.front()->GetConcatenatedString());
}

BOOST_AUTO_TEST_SUITE_END()



BOOST_AUTO_TEST_SUITE(test_suite_multithread_file_output)

BOOST_AUTO_TEST_CASE(verify_statistics)
{
  std::string testData{"cmd1\ncmd2\ncmd3\n"
                      "{\ncmd4\n\ncmd6\ncmd7\n}\n"
                      "cmd9\ncmd8\ncmd10\n"
                      "cmd11\ncmd12\ncmd13\n"
                      "cmd14\n"};
  std::istringstream iss(testData);
  std::ostringstream oss;

  auto commandProcessor = std::make_unique<CommandProcessor>();
  auto storage = std::make_shared<Storage>(3);
  auto fileOutput = std::make_shared<FileOutput>(2);

  storage->Subscribe(fileOutput);
  commandProcessor->Subscribe(storage);

  commandProcessor->Process(iss);

  auto thread_handlers = fileOutput->StopWorkers();

  auto main_statisctics = storage->GetStatisctics();
  decltype(main_statisctics) threads_statisctics;
  for(const auto& handler : thread_handlers) {
    auto statistic = handler->GetStatisctics();
    threads_statisctics.commands += statistic.commands;
    threads_statisctics.blocks += statistic.blocks;

    auto filenames = handler->GetProcessedFilenames();
    for(const auto& filename : filenames) {
      std::remove(filename.c_str());
    }
  }
  BOOST_REQUIRE_EQUAL(main_statisctics.commands, threads_statisctics.commands);
  BOOST_REQUIRE_EQUAL(main_statisctics.blocks, threads_statisctics.blocks);

  BOOST_REQUIRE(main_statisctics.commands != thread_handlers.front()->GetStatisctics().commands);
  BOOST_REQUIRE(main_statisctics.blocks != thread_handlers.front()->GetStatisctics().blocks);
}

BOOST_AUTO_TEST_CASE(verify_unique_filenames)
{
  std::string testData{"cmd1\ncmd2\ncmd3\n"
                      "{\ncmd4\n\ncmd6\ncmd7\n}\n"
                      "cmd9\ncmd8\ncmd10\n"
                      "cmd11\ncmd12\ncmd13\n"
                      "cmd14\ncmd15\ncmd16\n"
                      "cmd17\ncmd18\ncmd19\n"
                      "cmd20\ncmd21\ncmd22\n"
                      "cmd23\n"};
  std::istringstream iss(testData);
  std::ostringstream oss;

  auto commandProcessor = std::make_unique<CommandProcessor>();
  auto storage = std::make_shared<Storage>(3);
  auto fileOutput = std::make_shared<FileOutput>(3);

  storage->Subscribe(fileOutput);
  commandProcessor->Subscribe(storage);

  commandProcessor->Process(iss);

  auto thread_handlers = fileOutput->StopWorkers();

  decltype(std::declval<FileOutputThreadHandler>().GetProcessedFilenames()) filenames;
  for(const auto& handler : thread_handlers) {
    auto thread_filenames = handler->GetProcessedFilenames();
    std::copy(std::cbegin(thread_filenames), std::cend(thread_filenames), std::back_inserter(filenames));
  }
  for(const auto& filename : filenames) {
    std::remove(filename.c_str());
  }

  std::sort(std::begin(filenames), std::end(filenames));
  BOOST_REQUIRE(std::cend(filenames) == std::adjacent_find(std::cbegin(filenames), std::cend(filenames)));
}

BOOST_AUTO_TEST_SUITE_END()
