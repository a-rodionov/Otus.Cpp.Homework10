#include <algorithm>
#include <limits>
#include "Storage.h"
#include "ConsoleOutput.h"
#include "FileOutput.h"
#include "CommandProcessor.h"



int main(int argc, char const* argv[])
{
  try
  {
    unsigned long long block_size;
    try
    {
      if(2 != argc)
        throw std::invalid_argument("");

      std::string digit_str{argv[1]};
      if(!std::all_of(std::cbegin(digit_str),
                      std::cend(digit_str),
                      [](unsigned char symbol) { return std::isdigit(symbol); } ))
        throw std::invalid_argument("");

      block_size = std::stoull(digit_str);
      if(0 == block_size)
        throw std::invalid_argument("");
    }
    catch(...)
    {
      std::string error_msg = "The programm must be started with only one parameter. It must be a digit from 1 to "
                              + std::to_string(std::numeric_limits<decltype(block_size)>::max())
                              + " in decimal base.";
      throw std::invalid_argument(error_msg);
    }

    auto commandProcessor = std::make_unique<CommandProcessor>();
    auto storage = std::make_shared<Storage>(block_size);
    auto consoleOutput = std::make_shared<ConsoleOutput>(std::cout);
    auto fileOutput = std::make_shared<FileOutput>(2);

    storage->Subscribe(consoleOutput);
    storage->Subscribe(fileOutput);
    commandProcessor->Subscribe(storage);

    commandProcessor->Process(std::cin);

    auto console_hadlers = consoleOutput->StopWorkers();
    auto file_hadlers = fileOutput->StopWorkers();

    std::cout << "main поток - " << commandProcessor->GetProcessedLines() << " строк, "
              << storage->GetStatisctics() << std::endl;

    for(const auto& handler : console_hadlers) {
      auto statistic = handler->GetStatisctics();
      std::cout << "log поток - " << statistic << std::endl;
    }

    auto i{1};
    for(const auto& handler : file_hadlers) {
      auto statistic = handler->GetStatisctics();
      std::cout << "file" << i++ << " поток - " << statistic << std::endl;
    }
  }
  catch (const std::exception& e)
  {
    std::cerr << e.what() << std::endl;
  }
  return 0;
}