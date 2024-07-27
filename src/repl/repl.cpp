#include <cstdio>
#include <iostream>
#include <string>

#include "repl.hpp"
#include "../vm/vm.hpp"

namespace TPV {
void repl() {
  TPV::VM mv = TPV::VM();
  
  printf("Entering REPL mode...\n");
  std::string input;
  while (true) {
    printf(">>> ");
    std::getline(std::cin, input);
    if (input == "exit") {
      break;
    }
    else {
      printf("Unknown Command\n");
      printf("Command List:\n");
      printf("  exit\n");
    }
  }
}
}  // namespace TPV