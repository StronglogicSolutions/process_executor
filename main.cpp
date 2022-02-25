#include <iostream>
#include <string>

#include "process.hpp"

int main(int argc, char** argv)
{

  kiq::ProcessResult result = kiq::qx({"./test_application.sh"});

  if (result.error)
    throw std::runtime_error{"Process failed"};

  std::cout << "Result is\n\n" << result.output << std::endl;

  return 0;
}
