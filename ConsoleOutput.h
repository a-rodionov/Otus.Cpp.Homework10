#pragma once

#include <iostream>
#include "IOutput.h"

class ConsoleOutput : public IOutput
{
  using IOutput::Output;

public:

  explicit ConsoleOutput(std::ostream& out)
    : out{out} {}

  void Output(const std::size_t timestamp, const std::list<std::string>& data) override {
    Output(out, data);
  }

private:

  std::ostream& out;
};
