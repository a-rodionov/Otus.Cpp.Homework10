#pragma once

#include <iostream>
#include "StorageObservable.h"

class CommandProcessor : public StorageObservable
{

public:

  void Process(std::istream& in) {
    for(std::string command; std::getline(in, command);) {
      ++processedLines;
      if("{" == command) {
        if(0 == open_brace_count++) {
          BlockStart();
        }
        continue;
      }
      if("}" == command) {
        if(0 == open_brace_count){
          BlockEnd();
        }
        else if(0 == --open_brace_count) {
          BlockEnd();
        }
        continue;
      }
      Push(command);
    }
    if(0 == open_brace_count) {
      Flush();
    }
  }

  auto GetProcessedLines() const {
    return processedLines;
  }

private:

  std::size_t open_brace_count{0};
  std::size_t processedLines{0};

};
