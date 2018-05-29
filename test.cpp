#include <sys/file.h>
#include <sstream>
#include "Storage.h"
#include "ConsoleOutput.h"
#include "FileOutput.h"
#include "CommandProcessor.h"

#define BOOST_TEST_MODULE test_main

#include <boost/test/unit_test.hpp>
#include <boost/test/included/unit_test.hpp>

/*
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

BOOST_AUTO_TEST_CASE(make_filename)
{
  size_t timestamp = 123;
  BOOST_CHECK_EQUAL("bulk123.log", FileOutput::MakeFilename(timestamp));
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
  std::shared_ptr<IOutput> consoleOutput;
  std::ostringstream oss;
};

BOOST_FIXTURE_TEST_SUITE(fixture_test_suite_bulk, initialized_command_processor)

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

  BOOST_CHECK_EQUAL(oss.str(), result);
}

BOOST_AUTO_TEST_CASE(file_output)
{
  FileOutput fileOutput;
  std::string result;
  std::string goodResult{"bulk: cmd1, cmd2, cmd3"};
  std::list<std::string> testData{"cmd1", "cmd2", "cmd3"};
  size_t timestamp = 123;
  auto filename = FileOutput::MakeFilename(timestamp);

  std::remove(filename.c_str());  

  fileOutput.Output(timestamp, testData);
  
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
  FileOutput fileOutput;
  std::string result;
  std::string goodResult{"bulk: cmd1, cmd2, cmd3"};
  std::list<std::string> testData{"cmd1", "cmd2", "cmd3"};
  size_t timestamp = 123;
  auto filename = FileOutput::MakeFilename(timestamp);

  std::remove(filename.c_str());

  auto file_handler = open(filename.c_str(), O_CREAT);
  BOOST_REQUIRE_EQUAL(true, -1 != file_handler);
  BOOST_REQUIRE_EQUAL(true, -1 != flock( file_handler, LOCK_EX | LOCK_NB ));
  
  BOOST_CHECK_THROW(fileOutput.Output(timestamp, testData), std::runtime_error);
  
  flock(file_handler, LOCK_UN | LOCK_NB);
  close(file_handler);
  std::remove(filename.c_str());
}

BOOST_AUTO_TEST_SUITE_END()
*/