#include <iostream>
#include <string>

#include "process.hpp"

int main(int argc, char** argv)
{
  kiq::args_t args            = {"./test_application.sh"};
  int         timeout         = 30;
  bool        kill_on_timeout = true;
  bool        handle_process  = false;

  kiq::process process(args, timeout, kill_on_timeout, handle_process);
  if (process.has_work())
    process.do_work();

  if (process.has_error())
  {
    std::cerr << "Process failed with error: " << process.get_error() << std::endl;
    return 1;
  }

  kiq::proc_result_t result  = process.get();

  std::cout << "Process ended with exit code " << process.exit_code() <<
              "\n\nSTDOUT: \n"                 << result.output       << std::endl;

  return 0;
}
