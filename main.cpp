#include <iostream>
#include <string>
#include <vector>

#include "executor.hpp"

void foo(std::string value) {
  std::cout << "Returned value" << value << std::endl;
}
// Callback
std::function<std::string(std::string)> callback_fn([](std::string value) {
  foo(value);
  return value;
});

int main(int argc, char** argv) {
  ProcessExecutor executor{};

  executor.setEventCallback(callback_fn);
  executor.request("./test_application.sh", {"arg1", "arg2"});

  return 0;
}
