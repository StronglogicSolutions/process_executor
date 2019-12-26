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

// Rubbish config
std::vector<char> cv{'a', 'b', 'c'};

int main(int argc, char** argv) {
  ProcessExecutor executor{&cv};

  executor.setEventCallback(callback_fn);
  executor.request("/data/c/learncpp/executor/test_application.sh");

  return 0;
}
