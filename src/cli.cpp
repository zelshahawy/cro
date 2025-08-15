// src/cli.cpp
#include "cro/cli.hpp"
#include "cro/tui.hpp"
#include "cro/db.hpp"

#include <cro/version.hpp>
#include <filesystem>
#include <cstdlib>
#include <iostream>
#include <string>
#include <string_view>

using namespace std::string_view_literals;

namespace cro
{

  // ----- helpers --------------------------------------------------------------

  static void print_help()
  {
    std::cout << "Cro — your terminal gym tracker\n"
                 "Commands:\n"
                 "  tui         Launch the TUI\n"
                 "  --version   Show version\n"
                 "  --help      Show this help\n"
                 "  quit/exit   Exit (REPL mode)\n";
  }

  static std::string default_db_path()
  {
#ifdef _WIN32
    const char *appdata = std::getenv("APPDATA");
    std::filesystem::path base = appdata ? appdata : ".";
    return (base / "cro" / "cro.db").string();
#elif __APPLE__
    const char *home = std::getenv("HOME");
    std::filesystem::path base = home ? home : ".";
    return (base / "Library" / "Application Support" / "cro" / "cro.db").string();
#else
    const char *home = std::getenv("HOME");
    std::filesystem::path base = home ? home : ".";
    return (base / ".local" / "share" / "cro" / "cro.db").string();
#endif
  }

  static cro::db::Handle init_db()
  {
    auto path = default_db_path();
    std::filesystem::create_directories(std::filesystem::path(path).parent_path());
    auto h = cro::db::open(path);
    cro::db::migrate(h.raw);
    return h; // move out; keeps DB open while in scope
  }

  // ----- CLI ------------------------------------------------------------------

  int run_cli(int argc, char **argv)
  {
    if (argc > 1)
    {
      const std::string_view arg = argv[1];

      if (arg == "--version"sv || arg == "-v"sv)
      {
        std::cout << CRO_VERSION_STRING << " (commit " << CRO_GIT_COMMIT << ")\n";
        return 0;
      }
      if (arg == "--help"sv || arg == "-h"sv)
      {
        print_help();
        return 0;
      }
      if (arg == "tui"sv)
      {
        auto db = init_db();
        return cro::run_tui(db.raw);
      }
    }

    // REPL mode (optional)
    auto db = init_db();
    print_help();
    std::string line;
    while (true)
    {
      std::cout << "> " << std::flush;
      if (!std::getline(std::cin, line))
        break;
      if (line == "quit" || line == "exit")
        break;
      if (line == "help")
      {
        print_help();
        continue;
      }
      if (line == "tui")
      {
        (void)cro::run_tui(db.raw);
        continue;
      }
      std::cout << "unrecognized: " << line << "\n";
    }
    return 0;
  }

} // namespace cro
