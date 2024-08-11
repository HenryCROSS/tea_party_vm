#include <iostream>
#include "repl/repl.hpp"
#include "scanner/scanner.hpp"
#include "vm/vm.hpp"
#include "parser/parser.hpp"

using namespace std;

void test2(const std::string& filename) {
  auto tokens_opt = TPV::scan_file(filename);
  if (tokens_opt) {
    TPV::Parser parser{};
    TPV::VM vm{};

    TPV::Test_Fn::print_tokens(*tokens_opt);
    parser.load_tokens(*tokens_opt);
    auto result = parser.parse();
    parser.print_bytecodes();
    if(result.err_msg.empty()){
      vm.load_bytes(result.bytecodes);
      vm.eval_all();
      vm.print_regs();
      vm.print_str_table();
    }else {
      for (auto&& i : result.err_msg) {
        std::cout << i << "\n";
      }
      // vm.print_regs();
    }
  } else {
    std::cerr << "Failed to scan file" << std::endl;
  }
}

int main(int argc, char** argv) {
  // TPV::VM mv = TPV::VM();
  // // load $0, 131102
  // std::vector<uint8_t> instruction0 = {1, 0};
  // auto number = TPV::float32_to_bytes(31.234);
  // instruction0.insert(instruction0.end(), number.begin(), number.end());
  // mv.load_bytes(instruction0);

  // std::vector<uint8_t> instruction1 = {1, 1};
  // instruction1.insert(instruction1.end(), number.begin(), number.end());
  // mv.load_bytes(instruction1);

  // std::vector<uint8_t> add = {3, 2, 0, 1};
  // mv.load_bytes(add);

  // mv.eval_all();

  // mv.print_regs();

  if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " -repl | -c <filename1> [filename2] [...]" << std::endl;
        return 1;
    }

    std::string option = argv[1];

    if (option == "-repl") {
        TPV::repl();
    } else if (option == "-c") {
        if (argc < 3) {
            std::cerr << "Usage: " << argv[0] << " -c <filename1> [filename2] [...]" << std::endl;
            return 1;
        }
        for (int i = 2; i < argc; ++i) {
            const char* filename = argv[i];
            test2(filename);
        }
    } else {
        std::cerr << "Unknown option: " << option << std::endl;
        std::cerr << "Usage: " << argv[0] << " -repl | -c <filename1> [filename2] [...]" << std::endl;
        return 1;
    }

    return 0;
}
