#pragma once

#include <fstream>
#include "IOutput.h"

class FileOutput : public IOutput
{
  using IOutput::Output;

public:

  static std::string MakeFilename(std::size_t timestamp) {
    std::string filename = "bulk" + std::to_string(timestamp) + ".log";
    return filename;
  }

  void Output(std::size_t timestamp, const std::list<std::string>& data) override {
    std::ofstream ofs{MakeFilename(timestamp).c_str(), std::ofstream::out | std::ofstream::trunc};
    if(ofs.fail())
      throw std::runtime_error("FileOutput::Output. Can't open file for output.");
    Output(ofs, data);
    auto is_failed = ofs.fail();
    ofs.close();
    if(is_failed)
      throw std::runtime_error("FileOutput::Output. Failed to write to file.");
  }

};
