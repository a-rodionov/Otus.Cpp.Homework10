#pragma once

#include <iostream>
#include "StorageObservable.h"

class CommandProcessor : public StorageObservable
{
public:

  void Process(std::istream& in) {
    for(std::string command; std::getline(in, command);) {
      if(!command.empty()) {
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
      }
      Push(command);
    }
    if(0 == open_brace_count) {
      Flush();
    }
  }

private:

  std::size_t open_brace_count{0};

};
