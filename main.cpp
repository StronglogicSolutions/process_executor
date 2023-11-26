#include <iostream>
#include <string>

#include "process.hpp"

int main(int argc, char** argv)
{

  kiq::proc_wrap_t   process = kiq::qx({"./test_application.sh"});
  kiq::ProcessResult result  = process.result;
  if (result.error)
    throw std::runtime_error{"Process failed"};

  std::cout << "Result is\n\n" << result.output << std::endl;

  return 0;
}
