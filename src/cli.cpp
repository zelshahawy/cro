#include "cro/cli.hpp"
#include <iostream>
#include <string>
#include <string_view>

#include <cro/version.hpp> // generated at configure time

using namespace std::string_view_literals;

namespace cro {

static void print_help() {
  std::cout << "Cro — your terminal gym tracker\n"
               "Commands:\n"
               "  help        Show this help\n"
               "  quit/exit   Exit\n";
}

int run_cli(int argc, char **argv) {
  if (argc > 1) {
    std::string_view arg = argv[1];
    if (arg == "--version"sv || arg == "-v"sv) {
      std::cout << CRO_VERSION_STRING << " (commit " << CRO_GIT_COMMIT << ")\n";
      return 0;
    }
    if (arg == "--help"sv || arg == "-h"sv) {
      print_help();
      return 0;
    }
  }

  print_help();
  std::string line;
  while (true) {
    std::cout << "> " << std::flush;
    if (!std::getline(std::cin, line))
      break;
    if (line == "quit" || line == "exit")
      break;
    if (line == "help") {
      print_help();
      continue;
    }

    std::cout << "unrecognized: " << line << "\n";
  }
  return 0;
}

} // namespace cro
