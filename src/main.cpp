#include <iostream>
#include "utils.hpp"
#include "vm/vm.hpp"

using namespace std;

int main(int argc, char** argv) {
  TPV::VM mv = TPV::VM();
  // load $0, 131102
  std::vector<uint8_t> instruction0 = {1, 0};
  auto number = TPV::float32_to_bytes(31.234);
  instruction0.insert(instruction0.end(), number.begin(), number.end());
  mv.load_bytes(instruction0);

  std::vector<uint8_t> instruction1 = {1, 1};
  instruction1.insert(instruction1.end(), number.begin(), number.end());
  mv.load_bytes(instruction1);

  std::vector<uint8_t> add = {6, 2, 0, 1};
  mv.load_bytes(add);


  mv.eval_all();

  mv.print_regs();
  return 0;
}
