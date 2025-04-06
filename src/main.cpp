#include "FileHandler.h"
#include <iostream>

int main(int argc, char *argv[]) {
  if (argc != 3) {
    std::cerr << "Usage: " << argv[0]
              << "<technology_file_name>.json <test_name>.json" << std::endl;
    return 1;
  }

  try {
    JSONHandler technologyFile(argv[1]), testFile(argv[2]);
    std::cout << "Files exist and have correct extension" << std::endl;
  } catch (const std::exception &e) {
    std::cerr << "Error: " << e.what() << std::endl;
    return 1;
  }

  return 0;
}
