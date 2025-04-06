#pragma once

#include <filesystem>
#include <string>

class JSONHandler {
public:
  explicit JSONHandler(const std::string file) {
    auto filePath = std::filesystem::path(file);

    if (!std::filesystem::exists(filePath)) {
      throw std::invalid_argument("File does not exist.");
    }

    if (filePath.extension() != ".json") {
      throw std::invalid_argument("File must have a .json extension.");
    }

    this->filePath = filePath;
  }

  // TODO: implement content reading
  std::string getFileContent() const { return ""; }

  std::filesystem::path getFilePath() const { return filePath; }

private:
  std::filesystem::path filePath;
};
