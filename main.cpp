#include <iostream>
#include <string>

#include "process.hpp"

int main(int argc, char** argv)
{

  kiq::process process = kiq::process({"./test_application.sh"});
  if (process.has_work())
    process.do_work();

  if (process.has_error())
  {
    std::cerr << "Process failed with error: " << process.get_error() << std::endl;
    return 1;
  }

  kiq::ProcessResult result  = process.get_value();

  std::cout << "Result is\n\n" << result.output << std::endl;

  return 0;
}
