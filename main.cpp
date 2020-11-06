#include <iostream>
#include <string>

#include "process.hpp"

int main(int argc, char** argv) {

  ProcessResult result = qx({"./test_application.sh"});

  if (result.error) {
    std::cout << "Process failed" << std::endl;
    return 1;
  }

  std::cout << "Result is\n\n" << result.output << std::endl;

  return 0;
}
